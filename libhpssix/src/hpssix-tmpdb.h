/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_TMPDB_H
#define __HPSSIX_TMPDB_H

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sqlite3.h>

struct _hpssix_tmpdb {
    const char *dbpath;
    const char *schema;

    sqlite3 *dbconn;

    uint32_t n_sql;
    const char **sqlstr;
    sqlite3_stmt **stmt;
};

typedef struct _hpssix_tmpdb hpssix_tmpdb_t;

/**
 * @brief
 *
 * @param self
 * @param initdb
 *
 * @return
 */
int hpssix_tmpdb_open(hpssix_tmpdb_t *self, int initdb);

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
int hpssix_tmpdb_close(hpssix_tmpdb_t *self);

/**
 * @brief
 *
 * @param self
 * @param index
 *
 * @return
 */
static inline
sqlite3_stmt *hpssix_tmpdb_stmt_get(hpssix_tmpdb_t *self, int index)
{
    sqlite3_stmt *stmt = NULL;

    if (!self || !self->dbconn || !self->stmt) {
        errno = EINVAL;
        goto out;
    }

    if (index >= self->n_sql) {
        errno = EINVAL;
        goto out;
    }

    stmt = self->stmt[index];
out:
    return stmt;
}

/**
 * @brief
 *
 * @param self
 * @param sql
 *
 * @return
 */
static inline
int hpssix_tmpdb_exec_simple_sql(hpssix_tmpdb_t *self, const char *sql)
{
    int ret = -EINVAL;

    if (!self || !self->dbconn)
        return ret;

    ret = sqlite3_exec(self->dbconn, sql, 0, 0, 0);

    return ret == SQLITE_OK ? 0 : ret;
}

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_tmpdb_begin_transaction(hpssix_tmpdb_t *self)
{
    return hpssix_tmpdb_exec_simple_sql(self, "begin transaction;");
}

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_tmpdb_end_transaction(hpssix_tmpdb_t *self)
{
    return hpssix_tmpdb_exec_simple_sql(self, "end transaction;");
}

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_tmpdb_rollback_transaction(hpssix_tmpdb_t *self)
{
    return hpssix_tmpdb_exec_simple_sql(self, "rollback transaction;");
}

#endif /* __HPSSIX_TMPDB_H */

