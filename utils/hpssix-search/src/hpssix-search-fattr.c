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

#include "hpssix-search.h"

static char sqlbuf[4096];

static const char *fstr[] = {
    "oid",
    "st_mode",
    "st_uid",
    "st_gid",
    "st_size",
    "st_atime",
    "st_mtime",
    "st_ctime",
};

static char *get_sql_fattr(hpssix_search_fattr_t *search)
{
    int ret = 0;
    int i = 0;
    int count = 0;
    char *pos = NULL;
    hpssix_search_fattr_cond_t *cond = NULL;

    if (!search)
        return NULL;

    for (i = 0; i < N_COND_ARG; i++) {
        cond = &search->cond[i];

        if (i == cond->attr)
            count++;
    }

    pos = sqlbuf;

    if (search->removed)
        pos += sprintf(pos,
                       "WITH\n"
                       "  fattr_oids AS\n"
                       "    (SELECT oid FROM hpssix_object WHERE valid=false");
    else
        pos += sprintf(pos,
                       "WITH\n"
                       "  fattr_oids AS\n"
                       "    (SELECT oid FROM hpssix_object WHERE valid=true");

    for (i = 0; i < N_COND_ARG; i++) {
        cond = &search->cond[i];
        if (i != cond->attr)
            continue;

        if (cond->op == HPSSIX_OP_BETWEEN) {
            pos += sprintf(pos, " AND %s BETWEEN %lu AND %lu",
                                fstr[i], cond->val1, cond->val2);
        }
        else if (cond->op == HPSSIX_OP_BITAND) {
            pos += sprintf(pos, " AND %s & %lu > 0", fstr[i], cond->val1);
        }
        else {
            pos += sprintf(pos, " AND %s %s %lu",
                                fstr[i], opstr[cond->op], cond->val1);
        }
    }

    pos += sprintf(pos, ")");

    return sqlbuf;
}

char *hpssix_search_fattr_get_sql(hpssix_search_fattr_t *search)
{
    char *sql = NULL;

    sql = get_sql_fattr(search);
    if (!sql) {
        fprintf(stderr, "failed to generate the search query.\n");
        return NULL;
    }

    return sql;
}

