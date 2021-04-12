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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <hpssix.h>

#include "testlib.h"

static hpssix_config_t config;
static hpssix_db_t db;
static hpssix_workdata_t wd;

static PGconn *dbconn;
static char *testfile;
static struct stat sb;

/*
 * 1 | hpssix.st_dev
 * 2 | hpssix.st_ino
 * 3 | hpssix.st_mode
 * 4 | hpssix.st_nlink
 * 5 | hpssix.st_uid
 * 6 | hpssix.st_gid
 * 7 | hpssix.st_rdev
 * 8 | hpssix.st_size
 * 9 | hpssix.st_blksize
 * 10 | hpssix.st_blocks
 * 11 | hpssix.st_atime
 * 12 | hpssix.st_mtime
 * 13 | hpssix.st_ctime
 * 14 | hpssix.fulltext
*/
enum {
    COLNO_ST_DEV = 1,
    COLNO_ST_INO,
    COLNO_ST_MODE,
    COLNO_ST_NLINK,
    COLNO_ST_UID,
    COLNO_ST_GID,
    COLNO_ST_RDEV,
    COLNO_ST_SIZE,
    COLNO_ST_BLKSIZE,
    COLNO_ST_BLOCKS,
    COLNO_ST_ATIME,
    COLNO_ST_MTIME,
    COLNO_ST_CTIME,
    COLNO_FULLTEXT,
};

static inline
void db_handle_postgres_error(hpssix_db_t *self, PGresult *result,
                              ExecStatusType status)
{
    char *status_str = PQresStatus(status);
    char *error_str = PQresultErrorMessage(result);

    fprintf(stderr, "[HPSSIXDB] postgresql (%s: %s)\n", status_str, error_str);
}

/**
 * @brief for sending queries to postgres without returning data.
 *
 * @param self
 * @param command
 *
 * @return
 */
static inline int db_pqexec(hpssix_db_t *self, const char *command)
{
    int ret = 0;
    PGresult *res = NULL;
    ExecStatusType status = 0;

    if (!command)
        return EINVAL;

    res = PQexec(self->dbconn, command);
    status = PQresultStatus(res);

    if (status != PGRES_COMMAND_OK) {
        db_handle_postgres_error(self, res, status);
        ret = EIO;
    }
    else
        ret = 0;

    PQclear(res);

    return ret;
}

static inline int db_begin_transaction(hpssix_db_t *self)
{
    return db_pqexec(self, "BEGIN;");
}

static inline int db_end_transaction(hpssix_db_t *self)
{
    return db_pqexec(self, "END;");
}

static inline int db_rollback(hpssix_db_t *self)
{
}

PGresult *db_psql_query(hpssix_db_t *self, const char *format, ...)
{
    char *qstr = NULL;
    PGresult *result = NULL;
    va_list argv;
    ExecStatusType status = 0;

    va_start(argv, format);
    vasprintf(&qstr, format, argv);
    va_end(argv);

    if (!qstr)
        return NULL;

    result = PQexec(self->dbconn, qstr);
    free(qstr);

    status = PQresultStatus(result);

    printf("Error: %s\n", PQresultErrorMessage(result));
    printf("Status: %s\n", PQresStatus(status));

    return result;
}

static void db_print_psql_error(FILE *fp, PGresult *result)
{
    ExecStatusType status = PQresultStatus(result);

    fprintf(fp, "PostgreSQL (status=%s, error=%s)\n",
            PQresStatus(status), PQresultErrorMessage(result));
}


static inline
PGresult *db_insert_stat_attr(hpssix_db_t *self, uint64_t oid, struct stat *sb)
{
    return db_psql_query(self,
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 1, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 2, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 3, %u);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 4, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 5, %u);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 6, %u);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 7, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 8, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 9, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 10, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 11, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 12, %lu);"
                         "insert into hpssix_attr_val (oid, kid, ival) "
                         "values (%lu, 13, %lu);",
                         oid, sb->st_dev,
                         oid, sb->st_ino,
                         oid, sb->st_mode,
                         oid, sb->st_nlink,
                         oid, sb->st_uid,
                         oid, sb->st_gid,
                         oid, sb->st_rdev,
                         oid, sb->st_size,
                         oid, sb->st_blksize,
                         oid, sb->st_blocks,
                         oid, sb->st_atime,
                         oid, sb->st_mtime,
                         oid, sb->st_ctime);
}

static
uint64_t db_insert_new_object(hpssix_db_t *self, char *path, struct stat *sb)
{
    PGresult *res = NULL;
    uint64_t oid = 0;
    char *val = NULL;
    char *namepos = NULL;
    char name[NAME_MAX] = { 0, };

    db_begin_transaction(self);

    res = db_psql_query(self, "insert into hpssix_object (object) "
                              "values (%lu) returning oid;", sb->st_ino);
    if (!res || PQntuples(res) != 1) {
        db_print_psql_error(stderr, res);
        goto out;
    }

    val = PQgetvalue(res, 0, 0);
    oid = strtoull(val, 0, 0);

    PQclear(res);

    namepos = strrchr(path, '/');
    strcpy(name, &namepos[1]);

    res = db_psql_query(self, "insert into hpssix_file (oid, path, name) "
                              "values (%lu,'%s','%s');", oid, path, name);
    if (!res) {
        db_print_psql_error(stderr, res);
        oid = 0;
        goto out;
    }

    PQclear(res);

    res = db_insert_stat_attr(self, oid, sb);
    if (!res) {
        db_print_psql_error(stderr, res);
        oid = 0;
    }

    db_end_transaction(self);

out:
    PQclear(res);

    return oid;
}

static int do_test(void)
{
    int ret = 0;
    uint64_t oid = 0;
    char *val = NULL;
    PGresult *res = NULL;

    oid = db_insert_new_object(&db, testfile, &sb);
    printf("pid = %lu\n", oid);

    if (oid == 0) {
        ret = EIO;
        goto out;
    }

    if (!S_ISREG(sb.st_mode)) /* nothing further to do */
        goto out;

    ret = hpssix_workdata_append(&wd, oid, testfile);
    if (ret)
        ret = -EIO;

out:
    return ret;
}

int main(int argc, char **argv)
{
    int ret = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        return 1;
    }

    testfile = argv[1];

    assert(0 == stat(testfile, &sb));

    ret = hpssix_config_read_sysconf(&config);
    if (ret)
        die("hpssix_config_read_sysconf failed");

    hpssix_config_dump(&config, stdout);

    ret = hpssix_db_connect(&db, &config);
    if (ret)
        die("hpssix_db_connect failed");

    /* create tables .. */
    ret = hpssix_db_initialize(&db);
    if (ret)
        die("hpssix_db_initialize failed");

    dbconn = db.dbconn;

    ret = hpssix_workdata_create(&wd, "test.sqlite3.db");
    if (ret)
        die("hpssix_workdata_create failed");

    do_test();

    hpssix_workdata_close(&wd);

    ret = hpssix_db_disconnect(&db);
    if (ret)
        die("hpssix_db_disconnect failed");

    hpssix_config_free(&config);

    return 0;
}

