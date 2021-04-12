/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_DB_H
#define __HPSSIX_DB_H

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libpq-fe.h>

#include "hpssix.h"

struct _hpssix_db {
    PGconn *dbconn;
    FILE *logfp;
};

typedef struct _hpssix_db hpssix_db_t;

struct _hpssix_tag {
    uint64_t kid;
    char *key;
    char *val;
    long double rval;
};

typedef struct _hpssix_tag hpssix_tag_t;

struct _hpssix_xattr {
    uint64_t oid;
    uint64_t n_tags;
    hpssix_tag_t tags[0];
};

typedef struct _hpssix_xattr hpssix_xattr_t;

/**
 * @brief
 *
 * @param self
 * @param config
 *
 * @return
 */
int hpssix_db_connect(hpssix_db_t *self, hpssix_config_t *config);

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
int hpssix_db_disconnect(hpssix_db_t *self);

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
int hpssix_db_initialize(hpssix_db_t *self);

/**
* @brief
*
* @param self
* @param str
*
* @return
*/
static inline
char *hpssix_db_escape_literal(hpssix_db_t *self, const char *str)
{
    return PQescapeLiteral(self->dbconn, str, strlen(str));
}

/**
* @brief
*
* @param ptr
*/
static inline void hpssix_db_freemem(void *ptr)
{
    if (ptr)
        PQfreemem(ptr);
}

/**
 * @brief for sending queries to postgres without returning data.
 *
 * @param self
 * @param command
 *
 * @return
 */
static inline int hpssix_db_psql_exec(hpssix_db_t *self, const char *command)
{
    int ret = 0;
    PGresult *res = NULL;
    ExecStatusType status = 0;

    if (!command)
        return EINVAL;

    res = PQexec(self->dbconn, command);
    status = PQresultStatus(res);

    if (status != PGRES_COMMAND_OK) {
        fprintf(stderr, "PostgreSQL (status=%s, error=%s)\n",
                PQresStatus(status), PQresultErrorMessage(res));
        ret = EIO;
    }

    PQclear(res);

    return ret;
}


/**
 * @brief
 *
 * @param self
 * @param format
 * @param ...
 *
 * @return
 */
PGresult *hpssix_db_psql_query(hpssix_db_t *self, const char *format, ...);

static inline int hpssix_db_copy_init(hpssix_db_t *self, const char *command)
{
    int ret = 0;
    PGresult *res = NULL;
    ExecStatusType status = 0;

    if (!command)
        return EINVAL;

    res = PQexec(self->dbconn, command);
    status = PQresultStatus(res);

    if (status != PGRES_COPY_IN) {
        fprintf(stderr, "PostgreSQL (status=%s, error=%s)\n",
                PQresStatus(status), PQresultErrorMessage(res));
        ret = EIO;
    }

    PQclear(res);

    return ret;
}

/**
 * @brief
 *
 * @param self
 * @param str
 *
 * @return 0 on success, @errno otherwise
 */
static inline int hpssix_db_copy_put(hpssix_db_t *self, const char *str)
{
    int ret = 0;

    if (!self || !str)
        return EINVAL;

    /* this returns 1 on successful queueing */
    ret = PQputCopyData(self->dbconn, str, strlen(str));
    if (ret != 1) {
        fprintf(stderr, "PostgreSQL error: %s\n",
                        PQerrorMessage(self->dbconn));
        return EIO;
    }

    return 0;
}

static inline int hpssix_db_copy_end(hpssix_db_t *self, int abort)
{
    int ret = 0;
    PGresult *res = NULL;
    char *msg = NULL;

    if (abort)
        msg = "abort";
    
    PQputCopyEnd(self->dbconn, msg);

    if (abort)
        goto out;

    res = PQgetResult(self->dbconn);
    if (PGRES_COMMAND_OK == PQresultStatus(res))
        ret = 0;
    else {
        fputs(PQerrorMessage(self->dbconn), stderr);
        ret = EIO;
    }

    PQclear(res);

out:
    return ret;
}

/**
 * @brief
 *
 * @param in: a single record line from input_csv
 * @param out: LINE_MAX sized output line
 * @param data: any private data that should be passed together
 *
 * @return 1 when @out is processed/filled, 0 if original @in should be used
 */
typedef int (*hpssix_db_copy_line_processor_t)(const char *in, char *out,
                                               void *data);

struct _hpssix_db_copy {
    int atomic;             /* if set (1), internally use transaction */

    const char *stmt_init;  /* sql stmt before copy execution */
    const char *stmt_copy;  /* sql stmt for copy execution (containing COPY) */
    const char *stmt_fini;  /* sql stmt after copy execution */
    const char *input_csv;  /* input csv file path */

    uint64_t batch_size;    /* default 0, unlimited */
    uint64_t n_processed;   /* number of records processed on success */

    hpssix_db_copy_line_processor_t line_processor;
    void *line_processor_data;
};

typedef struct _hpssix_db_copy hpssix_db_copy_t;

/**
 * @brief
 *
 * @param self
 * @param copy
 *
 * @return 0 on success, @errno otherwise.
 */
int hpssix_db_copy(hpssix_db_t *self, hpssix_db_copy_t *copy);

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_db_begin_transaction(hpssix_db_t *self)
{
    return hpssix_db_psql_exec(self, "BEGIN;");
}

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_db_end_transaction(hpssix_db_t *self)
{
    return hpssix_db_psql_exec(self, "END;");
}

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_db_rollback(hpssix_db_t *self)
{
    return hpssix_db_psql_exec(self, "ABORT;");
}

/**
 * @brief
 *
 * @param self
 * @param xattrs
 * @param count
 *
 * @return
 */
int hpssix_db_populate_xattr_keys(hpssix_db_t *self, hpssix_xattr_t **xattrs,
                                  uint64_t count);

/**
 * @brief
 *
 * @param self
 * @param xattrs
 * @param count
 *
 * @return
 */
int hpssix_db_delete_xattrs(hpssix_db_t *self, hpssix_xattr_t **xattrs,
                            uint64_t count);

/**
 * @brief
 *
 * @param self
 * @param object_id
 * @param meta
 * @param text
 *
 * @return
 */
int hpssix_db_index_tsv(hpssix_db_t *self, uint64_t object_id,
                        const char *meta, const char *text);

#endif  /* HPSSIX_DB_H */

