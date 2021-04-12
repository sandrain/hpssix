/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hpssix-mdb.h"

static const char *mdb_name = "hpssix.mdb.db";

static inline hpssix_tmpdb_t *mdb_tmpdb(hpssix_mdb_t *mdb)
{
    return &mdb->tmpdb;
}

/*
 * TODO: create indices.
 */
static const char *mdb_schema =
"-- hpssix-mdb-schema.sql\n"
"\n"
"begin transaction;\n"
"\n"
"drop table if exists hpssix_mdb;\n"
"\n"
"create table hpssix_mdb (\n"
"    id integer primary key,\n"
"    oid_start integer not null,\n"
"    oid_end integer not null,\n"
"    scanner_start integer,\n"
"    builder_start integer,\n"
"    extractor_start integer,\n"
"    extractor_end integer,\n"
"    n_scanned integer,\n"
"    n_deleted integer,\n"
"    n_indexed integer,\n"
"    n_extracted integer,\n"
"    status integer\n"
");\n"
"\n"
"end transaction;\n";

enum {
    HPSSIX_MDB_SQL_RECORD = 0,
    HPSSIX_MDB_SQL_LAST_OID,
    HPSSIX_MDB_SQL_LAST_STAUS,
    HPSSIX_MDB_SQL_FETCH_RECORD,

    N_HPSSIX_MDB_SQLS,
};

static const char *mdb_sqls[N_HPSSIX_MDB_SQLS] =
{
    /* [0] HPSSIX_MDB_SQL_RECORD */
    "insert into hpssix_mdb (oid_start, oid_end,\n"
    "scanner_start, builder_start, extractor_start, extractor_end,\n"
    "n_scanned, n_deleted, n_indexed, n_extracted, status)\n"
    "values (?,?,?,?,?,?,?,?,?,?,?)",

    /* [1] HPSSIX_MDB_SQL_LAST_OID */
    "select max(oid_end) from hpssix_mdb where status=0",

    /* [2] HPSSIX_MDB_SQL_LAST_STAUS */
    "select oid_end, scanner_start from hpssix_mdb\n"
    "where status=0 order by id desc limit 1",

    /* [3] HPSSIX_MDB_SQL_FETCH_RECORD */
    "select id,oid_start,oid_end,\n"
    "scanner_start,builder_start,extractor_start,extractor_end,\n"
    "n_scanned,n_deleted,n_indexed,n_extracted,status\n"
    "from hpssix_mdb order by id desc limit ?,?",
};

int hpssix_mdb_open(hpssix_mdb_t *mdb)
{
    int ret = 0;
    int need_init = 0;
    hpssix_config_t *config = NULL;
    hpssix_tmpdb_t *tmpdb = NULL;
    char dbpath[PATH_MAX] = { 0, };
    struct stat sb = { 0, };

    if (!mdb)
        return EINVAL;

    tmpdb = &mdb->tmpdb;
    config = &mdb->config;

    ret = hpssix_config_read_sysconf(config);
    if (ret)
        return ret;

    sprintf(dbpath, "%s/%s", hpssix_get_work_root(), mdb_name);

    ret = stat(dbpath, &sb);
    if (ret < 0) {
        if (errno != ENOENT)
            goto out;

        need_init = 1;
    }

    memset((void *) tmpdb, 0, sizeof(*tmpdb));

    tmpdb->dbpath = dbpath;
    tmpdb->schema = mdb_schema;
    tmpdb->n_sql = N_HPSSIX_MDB_SQLS;
    tmpdb->sqlstr = mdb_sqls;

    ret = hpssix_tmpdb_open(tmpdb, need_init);

out:
    return ret;
}

int hpssix_mdb_close(hpssix_mdb_t *mdb)
{
    if (mdb)
        hpssix_tmpdb_close(&mdb->tmpdb);

    return 0;
}

int hpssix_mdb_record(hpssix_mdb_t *mdb, hpssix_work_status_t *status)
{
    int ret = 0;
    sqlite3_stmt *stmt = NULL;
    hpssix_tmpdb_t *tmpdb = NULL;

    if (!mdb || !status)
        return EINVAL;

    tmpdb = mdb_tmpdb(mdb);
    stmt = hpssix_tmpdb_stmt_get(tmpdb, HPSSIX_MDB_SQL_RECORD);

    ret = sqlite3_bind_int64(stmt, 1, status->oid_start);
    ret |= sqlite3_bind_int64(stmt, 2, status->oid_end);
    ret |= sqlite3_bind_int64(stmt, 3, status->scanner_start);
    ret |= sqlite3_bind_int64(stmt, 4, status->builder_start);
    ret |= sqlite3_bind_int64(stmt, 5, status->extractor_start);
    ret |= sqlite3_bind_int64(stmt, 6, status->extractor_end);
    ret |= sqlite3_bind_int64(stmt, 7, status->n_scanned);
    ret |= sqlite3_bind_int64(stmt, 8, status->n_deleted);
    ret |= sqlite3_bind_int64(stmt, 9, status->n_indexed);
    ret |= sqlite3_bind_int64(stmt, 10, status->n_extracted);
    ret |= sqlite3_bind_int64(stmt, 11, status->status);
    if (ret) {
        ret = EIO;
        goto out;
    }

    hpssix_tmpdb_begin_transaction(tmpdb);

    do {
        ret = sqlite3_step(stmt);
    } while (ret == SQLITE_BUSY);

    if (ret == SQLITE_DONE) {
        hpssix_tmpdb_end_transaction(tmpdb);
        ret = 0;
    }
    else {
        hpssix_tmpdb_rollback_transaction(tmpdb);
        ret = EIO;
    }

out:
    sqlite3_reset(stmt);

    return ret;
}

int hpssix_mdb_get_last_scanned_oid(hpssix_mdb_t *mdb, uint64_t *oid)
{
    int ret = 0;
    sqlite3_stmt *stmt = NULL;

    if (!mdb)
        return EINVAL;

    stmt = hpssix_tmpdb_stmt_get(mdb_tmpdb(mdb), HPSSIX_MDB_SQL_LAST_OID);

    do {
        ret = sqlite3_step(stmt);
    } while (ret == SQLITE_BUSY);

    if (ret == SQLITE_ROW) {
        *oid = sqlite3_column_int64(stmt, 0);
        ret = 0;
    }
    else if (ret == SQLITE_DONE) {
        /* first scan: start from @first_oid in config */
        *oid = mdb->config.first_oid;
        ret = 0;
    }
    else
        ret = EIO;

out:
    sqlite3_reset(stmt);

    return ret;
}

int hpssix_mdb_get_last_scanned_status(hpssix_mdb_t *mdb,
                                       uint64_t *oid, uint64_t *ts)
{
    int ret = 0;
    sqlite3_stmt *stmt = NULL;

    if (!mdb)
        return EINVAL;

    stmt = hpssix_tmpdb_stmt_get(mdb_tmpdb(mdb), HPSSIX_MDB_SQL_LAST_STAUS);

    do {
        ret = sqlite3_step(stmt);
    } while (ret == SQLITE_BUSY);

    if (ret == SQLITE_ROW) {
        *oid = sqlite3_column_int64(stmt, 0);
        *ts = sqlite3_column_int64(stmt, 1);
        ret = 0;
    }
    else if (ret == SQLITE_DONE) {
        *oid = 0;
        *ts = 0;
        ret = 0;
    }
    else
        ret = EIO;

out:
    sqlite3_reset(stmt);

    return ret;
}

static inline int fill_history(sqlite3_stmt *stmt,
                               hpssix_work_status_t *status, uint64_t *len)
{
    int ret = 0;
    int col = 0;
    int i = 0;
    uint64_t count = 0;
    hpssix_work_status_t tmp = { 0, };
    hpssix_work_status_t *current = NULL;

    do {
        tmp.id = sqlite3_column_int64(stmt, col++);
        tmp.oid_start = sqlite3_column_int64(stmt, col++);
        tmp.oid_end = sqlite3_column_int64(stmt, col++);
        tmp.scanner_start = sqlite3_column_int64(stmt, col++);
        tmp.builder_start = sqlite3_column_int64(stmt, col++);
        tmp.extractor_start = sqlite3_column_int64(stmt, col++);
        tmp.extractor_end = sqlite3_column_int64(stmt, col++);
        tmp.n_scanned = sqlite3_column_int64(stmt, col++);
        tmp.n_deleted = sqlite3_column_int64(stmt, col++);
        tmp.n_indexed = sqlite3_column_int64(stmt, col++);
        tmp.n_extracted = sqlite3_column_int64(stmt, col++);
        tmp.status = sqlite3_column_int(stmt, col);

        current = &status[i];
        if (!current) {
            ret = EINVAL;
            goto out;
        }

        *current = tmp;
        count++;
        i++;
        col = 0;

        ret = sqlite3_step(stmt);
    } while (SQLITE_ROW == ret);

    if (ret == SQLITE_DONE)
        ret = 0;
    else
        ret = EIO;

    *len = count;
out:
    return ret;
}

int hpssix_mdb_fetch_history(hpssix_mdb_t *mdb, uint64_t offset, uint64_t len,
                             uint64_t *outlen, hpssix_work_status_t *status)
{
    int ret = 0;
    sqlite3_stmt *stmt = NULL;

    if (!mdb || !status)
        return EINVAL;

    stmt = hpssix_tmpdb_stmt_get(mdb_tmpdb(mdb), HPSSIX_MDB_SQL_FETCH_RECORD);

    ret = sqlite3_bind_int64(stmt, 1, offset);
    ret |= sqlite3_bind_int64(stmt, 2, len);
    if (ret) {
        ret = EIO;
        goto out;
    }

    do {
        ret = sqlite3_step(stmt);
    } while (ret == SQLITE_BUSY);

    if (ret == SQLITE_ROW)
        ret = fill_history(stmt, status, outlen);
    else if (ret == SQLITE_DONE) {      /* no records found */
        ret = 0;
        *outlen = 0;
    }
    else
        ret = EIO;

out:
    sqlite3_reset(stmt);

    return 0;
}

