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
#include <limits.h>

#include "hpssix-workdata.h"

static const char *workdata_schema =
"begin transaction;\n"
"\n"
"drop table if exists hpssix_workdata;\n"
"\n"
"create table hpssix_workdata (\n"
"    id integer primary key,\n"
"    pid integer not null,\n"
"    path text not null\n"
");\n"
"\n"
"end transaction;\n";

enum {
    HPSSIX_WORKDATA_SQL_APPEND = 0,
    HPSSIX_WORKDATA_SQL_TOTALCOUNT,
    HPSSIX_WORKDATA_SQL_FETCH,
    N_HPSSIX_WORKDATA_SQLS,
};

static const char *workdata_sqls[N_HPSSIX_WORKDATA_SQLS] =
{
    /* HPSSIX_WORKDATA_SQL_APPEND */
    "insert into hpssix_workdata (pid,path) values (?,?)",

    /* HPSSIX_WORKDATA_SQL_TOTALCOUNT */
    "select count(id) from hpssix_workdata",

    /* HPSSIX_WORKDATA_SQL_FETCH */
    "select pid,path from hpssix_workdata order by id asc limit ?,?",
};

enum { HPSSIX_WORKDATA_OPEN = 0, HPSSIX_WORKDATA_CREATE = 1 };

static int __hpssix_workdata_open(hpssix_workdata_t *self, const char *name,
                                  int mode)
{
    int ret = 0;
    hpssix_config_t *config = NULL;
    hpssix_tmpdb_t *tmpdb = NULL;

    if (!self || !name)
        return EINVAL;

    tmpdb = &self->tmpdb;
    config = &self->config;

    ret = hpssix_config_read_sysconf(config);
    if (ret)
        return ret;

    memset((void *) tmpdb, 0, sizeof(*tmpdb));

    tmpdb->dbpath = name;
    tmpdb->schema = workdata_schema;
    tmpdb->n_sql = N_HPSSIX_WORKDATA_SQLS;
    tmpdb->sqlstr = workdata_sqls;

    return hpssix_tmpdb_open(tmpdb, mode);
}

int hpssix_workdata_create(hpssix_workdata_t *self, const char *name)
{
    return __hpssix_workdata_open(self, name, HPSSIX_WORKDATA_CREATE);
}

int hpssix_workdata_open(hpssix_workdata_t *self, const char *name)
{
    return __hpssix_workdata_open(self, name, HPSSIX_WORKDATA_OPEN);
}

int hpssix_workdata_close(hpssix_workdata_t *self)
{
    if (self)
        hpssix_tmpdb_close(&self->tmpdb);

    return 0;
}

int hpssix_workdata_append(hpssix_workdata_t *self,
                           uint64_t pid, const char *path)
{
    int ret = 0;
    sqlite3_stmt *stmt = NULL;

    if (!self || !path)
        return EINVAL;

    stmt = hpssix_tmpdb_stmt_get(&self->tmpdb, HPSSIX_WORKDATA_SQL_APPEND);
    ret = sqlite3_bind_int64(stmt, 1, pid);
    ret |= sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
    if (ret) {
        ret = EIO;
        goto out;
    }

    do {
        ret = sqlite3_step(stmt);
    } while (ret == SQLITE_BUSY);

    ret = ret == SQLITE_DONE ? 0 : EIO;
out:
    sqlite3_reset(stmt);

    return ret;
}

int hpssix_workdata_get_total_count(hpssix_workdata_t *self, uint64_t *count)
{
    int ret = 0;
    sqlite3_stmt *stmt = NULL;

    if (!self)
        return EINVAL;

    stmt = hpssix_tmpdb_stmt_get(&self->tmpdb, HPSSIX_WORKDATA_SQL_TOTALCOUNT);

    do {
        ret = sqlite3_step(stmt);
    } while (ret == SQLITE_BUSY);

    if (ret != SQLITE_ROW) {
        ret = EIO;
        goto out;
    }

    *count = sqlite3_column_int64(stmt, 0);

    ret = 0;

out:
    sqlite3_reset(stmt);

    return ret;
}

int hpssix_workdata_dispatch(hpssix_workdata_t *self,
                             hpssix_workdata_worklist_t *list)
{
    int ret = 0;
    uint64_t offset = 0;
    uint64_t count = 0;
    sqlite3_stmt *stmt = NULL;
    hpssix_workdata_object_t *object_list = NULL;

    if (!self || !list)
        return EINVAL;

    offset = list->offset;
    count = list->count;

    stmt = hpssix_tmpdb_stmt_get(&self->tmpdb, HPSSIX_WORKDATA_SQL_FETCH);
    ret = sqlite3_bind_int64(stmt, 1, offset);
    ret |= sqlite3_bind_int64(stmt, 2, count);
    if (ret) {
        ret = EIO;
        goto out;
    }

    do {
        ret = sqlite3_step(stmt);
    } while (ret == SQLITE_BUSY);

    if (ret != SQLITE_ROW) {
        ret = EIO;
        goto out;
    }

    /*
     * FIXME: the actual number of rows might be less than @count
     */
    object_list = calloc(count, sizeof(*object_list));
    if (!object_list) {
        ret = ENOMEM;
        goto out;
    }

    count = 0;

    do {
        hpssix_workdata_object_t *current = &object_list[count];

        current->object_id = sqlite3_column_int64(stmt, 0);
        current->path = strdup(sqlite3_column_text(stmt, 1));
        if (!current->path) {
            ret = ENOMEM;
            hpssix_workdata_cleanup_object_list(object_list, count);
            goto out;
        }

        count++;
    } while (sqlite3_step(stmt) == SQLITE_ROW);

    list->object_list = object_list;
    list->count = count;

    ret = 0;

out:
    sqlite3_reset(stmt);

    return ret;
}

