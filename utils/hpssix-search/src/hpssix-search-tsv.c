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

static int verbose;
static int print_meta;
static int print_text;

static char sqlbuf[8192];

static uint64_t default_limit = 100;

#if 0
static char *sqlstr_with_text =
"with ksearch (oid, meta, tsv, text) as ( "
"select oid, meta, tsv, text from ( "
"select oid, meta, tsv, text "
"from hpssix_attr_document, plainto_tsquery('%s') as q "
"where (tsv @@ q)) t1) "
"select k.oid,f.path,k.meta,k.text from "
"ksearch k natural join hpssix_file f "
"order by ts_rank_cd(k.tsv, plainto_tsquery('%s')) desc limit %lu";
#endif

static char *sqlstr =
"  tsv_oids (oid, tsv) AS\n"
"    (SELECT oid, tsv FROM (SELECT oid, tsv FROM hpssix_attr_document, "
"PLAINTO_TSQUERY('%s') AS q WHERE (tsv @@ q)) t1)";

char *hpssix_search_tsv_get_sql(hpssix_search_tsv_t *search)
{
    sprintf(sqlbuf, sqlstr, search->keyword);
    return sqlbuf;
}

