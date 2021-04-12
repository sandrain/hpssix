/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#include <config.h>

#include <stdlib.h>

#include "hpssix-tmpdb.h"

static int tmpdb_prepare_stmt(hpssix_tmpdb_t *self)
{
    int ret = 0;
    uint32_t i = 0;
    sqlite3_stmt **stmt = NULL;

    stmt = calloc(sizeof(*stmt), self->n_sql);
    if (!stmt)
        return ENOMEM;

    for (i = 0; i < self->n_sql; i++) {
        ret = sqlite3_prepare(self->dbconn, self->sqlstr[i], -1,
                              &stmt[i], NULL);
        if (ret != SQLITE_OK) {
            free(stmt);
            return EIO;
        }
    }

    self->stmt = stmt;

    return 0;
}

int hpssix_tmpdb_open(hpssix_tmpdb_t *self, int initdb)
{
    int ret = 0;
    sqlite3 *dbconn = NULL;

    if (!self)
        return EINVAL;

    if (initdb && !self->schema)
        return EINVAL;

    ret = sqlite3_open(self->dbpath, &dbconn);
    if (ret)
        return EIO;

    self->dbconn = dbconn;

    if (initdb) {
        ret = hpssix_tmpdb_exec_simple_sql(self, self->schema);
        if (ret)
            goto out_close;
    }

    ret = tmpdb_prepare_stmt(self);
    if (ret)
        goto out_close;

    return 0;

out_close:
    sqlite3_close(dbconn);
    return ret;
}

int hpssix_tmpdb_close(hpssix_tmpdb_t *self)
{
    if (self) {
        if (self->stmt)
            free(self->stmt);

        if (self->dbconn)
            sqlite3_close(self->dbconn);
    }

    return 0;
}

