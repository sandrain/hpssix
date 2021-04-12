/* Copyright (C) 2018 - Hyogi Sim <simh@ornl.gov>
 * 
 * Please refer to COPYING for the license.
 * ---------------------------------------------------------------------------
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <hpssix.h>

#include "testlib.h"

static hpssix_config_t config;
static hpssix_db_t db;

int main(int argc, char **argv)
{
    int ret = 0;
    PGconn *conn = NULL;
    char conninfo[LINE_MAX] = { 0, };

    ret = hpssix_config_read_sysconf(&config);
    if (ret)
        die("hpssix_config_read_sysconf failed\n");

    hpssix_config_dump(&config, stdout);

    sprintf(conninfo, "%s",
            "host=hpss-dev-md-index2 port=5432 user=hpssixdbp password=hpsshpss dbname=hpssixdb");
    conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        return 1;
    }

    PQfinish(conn);

    return 0;

    ret = hpssix_db_connect(&db, &config);
    if (ret)
        die("hpssix_db_connect failed\n");

    /* create tables .. */
    ret = hpssix_db_initialize(&db);
    if (ret)
        die("hpssix_db_initialize failed\n");

    ret = hpssix_db_disconnect(&db);
    if (ret)
        die("hpssix_db_disconnect failed\n");

    hpssix_config_free(&config);

    return 0;
}

