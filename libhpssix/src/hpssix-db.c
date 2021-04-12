/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#include <config.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libpq-fe.h>

#include "hpssix-db.h"

/**
 * @brief this will be generated from hpssix-db-schema.sql.
 */
extern const char *hpssix_db_schema_sqlstr;

static inline
void db_handle_postgres_error(hpssix_db_t *self, PGresult *result,
                              ExecStatusType status)
{
    char *status_str = PQresStatus(status);
    char *error_str = PQresultErrorMessage(result);

    hpssix_log_printf(self->logfp, "[HPSSIXDB] postgresql (%s: %s)\n",
                      status_str, error_str);
}

static void db_psql_print_error(FILE *fp, PGresult *result)
{
    ExecStatusType status = PQresultStatus(result);

    fprintf(fp, "PostgreSQL (status=%s, error=%s)\n",
            PQresStatus(status), PQresultErrorMessage(result));
}

PGresult *hpssix_db_psql_query(hpssix_db_t *self, const char *format, ...)
{
    char *qstr = NULL;
    PGresult *result = NULL;
    va_list argv;

    va_start(argv, format);
    vasprintf(&qstr, format, argv);
    va_end(argv);

    if (!qstr)
        return NULL;

    result = PQexec(self->dbconn, qstr);
    free(qstr);

    if (!result) {
        ExecStatusType status = PQresultStatus(result);

        fprintf(stderr, "Error: %s\n", PQresultErrorMessage(result));
        fprintf(stderr, "Status: %s\n", PQresStatus(status));
    }

    return result;
}

int hpssix_db_connect(hpssix_db_t *self, hpssix_config_t *config)
{
    char uri[512] = { 0, };
    PGconn *conn = NULL;
    int conn_status = -1;

    if (!self) {
        errno = EINVAL;
        return -1;
    }

    sprintf(uri, "postgresql://%s:%s@%s:%d/%s",
            config->db_user,
            config->db_password,
            config->db_host,
            config->db_port,
            config->db_database);

    conn = PQconnectdb(uri);

    conn_status = PQstatus(conn);
    if (conn_status != CONNECTION_OK) {
        errno = EIO;
        return -1;
    }

    self->dbconn = conn;
    self->logfp = stderr;

    return 0;
}

int hpssix_db_disconnect(hpssix_db_t *self)
{
    if (self) {
        if (self->dbconn)
            PQfinish(self->dbconn);
    }

    return 0;
}

int hpssix_db_initialize(hpssix_db_t *self)
{
    return hpssix_db_psql_exec(self, hpssix_db_schema_sqlstr);
}

int hpssix_db_populate_xattr_keys(hpssix_db_t *self, hpssix_xattr_t **xattrs,
                                  uint64_t count)
{
    int ret = 0;
    uint64_t i = 0;
    uint64_t j = 0;
    char *key = NULL;
    char *escaped_key = NULL;
    hpssix_xattr_t *current = NULL;
    PGresult *res = NULL;

    for (i = 0; i < count; i++) {
        current = xattrs[i];

        for (j = 0; j < current->n_tags; j++) {
            key = current->tags[j].key;
            escaped_key = PQescapeLiteral(self->dbconn, key, strlen(key));
            if (!escaped_key)
                return ENOMEM;

            res = hpssix_db_psql_query(self, "select * from hpssix_attr_key_pop(%s);",
                                escaped_key);

            PQfreemem(escaped_key);

            if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                PQclear(res);
                return EIO;
            }

            key = PQgetvalue(res, 0, 0);
            current->tags[j].kid = strtoul(key, NULL, 0);
            PQclear(res);
        }
    }

out:
    return ret;
}

int hpssix_db_delete_xattrs(hpssix_db_t *self, hpssix_xattr_t **xattrs,
                            uint64_t count)
{
    int ret = 0;
    uint64_t i = 0;
    uint64_t oid = 0;
    PGresult *res = NULL;

    for (i = 0; i < count; i++) {
        oid = xattrs[i]->oid;

        res = hpssix_db_psql_query(self, "delete from hpssix_attr_val\n"
                                  "where oid=%lu;\n", oid);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            ret = EIO;
            break;
        }
    }

    return ret;
}

int hpssix_db_index_tsv(hpssix_db_t *self, uint64_t object_id,
                        const char *meta, const char *text)
{
    int ret = 0;
    PGresult *res = NULL;
    char *escaped_meta = NULL;
    char *escaped_text = NULL;

    if (!meta)
        return EINVAL;

    escaped_meta = PQescapeLiteral(self->dbconn, meta, strlen(meta));
    if (!escaped_meta)
        return ENOMEM;

    if (text) {
        escaped_text = PQescapeLiteral(self->dbconn, text, strlen(text));
        if (!escaped_text)
            return ENOMEM;
    }
    else
        escaped_text = "''";

    res = hpssix_db_psql_query(self,
                               "INSERT INTO hpssix_attr_document\n"
                               "  (oid, meta, text) VALUES (%lu, %s, %s)\n"
                               "ON CONFLICT (oid)\n"
                               "DO UPDATE SET\n"
                               "  meta = EXCLUDED.meta, text = EXCLUDED.text;",
                               object_id, escaped_meta, escaped_text);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_psql_print_error(stderr, res);
        ret = EIO;
    }

    if (text)
        PQfreemem(escaped_text);

    PQfreemem(escaped_meta);
    PQclear(res);

    return ret;
}

int hpssix_db_copy(hpssix_db_t *self, hpssix_db_copy_t *copy)
{
    int ret = 0;
    FILE *fp = NULL;
    uint64_t n_processed = 0;
    char linebuf[LINE_MAX] = { 0, };
    char rbuf[LINE_MAX] = { 0, };
    char *copy_input = NULL;
    hpssix_db_copy_line_processor_t line_func = NULL;

    if (!self || !copy || !copy->input_csv || !copy->stmt_copy)
        return EINVAL;

    fp = fopen(copy->input_csv, "r");
    if (!fp)
        return errno;

    if (copy->atomic)
        hpssix_db_begin_transaction(self);

    if (copy->stmt_init) {
        ret = hpssix_db_psql_exec(self, copy->stmt_init);
        if (ret)
            goto out_rollback;
    }

    ret = hpssix_db_copy_init(self, copy->stmt_copy);
    if (ret)
        goto out_rollback;

    line_func = copy->line_processor;

    while (fgets(linebuf, LINE_MAX-1, fp) != NULL) {
        if (is_line_empty(linebuf))
            continue;

        copy_input = linebuf;

        if (line_func) {
            ret = line_func(linebuf, rbuf, copy->line_processor_data);
            if (ret < 0)
                goto out_rollback;

            if (1 == ret)
                copy_input = rbuf;
        }

        ret = hpssix_db_copy_put(self, copy_input);
        if (ret)
            goto out_rollback;

        n_processed++;
    }
    if (ferror(fp)) {   /* file i/o error */
        ret = errno;
        goto out_rollback;
    }

    ret = hpssix_db_copy_end(self, 0);
    if (ret)
        goto out_rollback;

    if (copy->stmt_fini) {
        ret = hpssix_db_psql_exec(self, copy->stmt_fini);
        if (ret)
            goto out_rollback;
    }

    if (copy->atomic) {
        ret = hpssix_db_end_transaction(self);
        if (ret)
            goto out_rollback;
    }

    copy->n_processed = n_processed;

    goto out_close;

out_rollback:
    if (n_processed)
        hpssix_db_copy_end(self, 1); /* abort the copy */

    if (copy->atomic)
        hpssix_db_rollback(self);

out_close:
    fclose(fp);

    return ret;
}

