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

#include "hpssix-search.h"

static char sqlbuf[8192];

/*
 * TODO: escape strings
 */
static int write_numeric_predicate(char *pos, hpssix_search_tag_cond_t *cond)
{
    int ret = 0;

    ret = sprintf(pos,
                  "(kid=(SELECT kid FROM hpssix_attr_key WHERE name='%s')",
                  cond->tag_name);

    if (cond->op == HPSSIX_OP_BETWEEN)
        ret += sprintf(&pos[ret], " AND rval BETWEEN %lf AND %lf)",
                                  cond->tag_name, cond->val1, cond->val2);
    else
        ret += sprintf(&pos[ret], " AND rval %s %lf)",
                                  opstr[cond->op], cond->val1);

    return ret;
}

static int write_string_predicate(char *pos, hpssix_search_tag_cond_t *cond)
{
    int ret = 0;

    ret = sprintf(pos, 
                  "(kid=(SELECT kid FROM hpssix_attr_key WHERE name='%s')",
                  cond->tag_name);
    ret += sprintf(&pos[ret], " AND sval LIKE '%%%%%s%%%%')", cond->tag_val);

    return ret;
}

static inline char *get_sql_tag(hpssix_search_tag_t *search)
{
    uint32_t i = 0;
    char *pos = sqlbuf;
    hpssix_search_tag_cond_t *cond = NULL;

    pos += sprintf(pos, "  tag_oids (oid) AS\n"
                        "    (SELECT oid FROM hpssix_attr_val");

    for (i = 0; i < search->count; i++) {
        cond = &search->conds[i];

        pos += sprintf(pos, i ? " AND " : " WHERE ");
        pos += cond->op >= 0 ? write_numeric_predicate(pos, cond)
                             : write_string_predicate(pos, cond);
    }

    pos += sprintf(pos, ")");

    return sqlbuf;
}

char *hpssix_search_tag_get_sql(hpssix_search_tag_t *search)
{
    char *sql = get_sql_tag(search);

    if (!sql)
        fprintf(stderr, "failed to generate the search query.\n");

    return sql;
}

