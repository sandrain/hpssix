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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <libgen.h>
#include <getopt.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "hpssix-builder.h"

static hpssix_config_t config;
static hpssix_extfilter_t *filter;

static int initdb;

static char *datadir;
static uint64_t task_id;

static hpssix_builder_t builder;

static int db_initialize(void)
{
    int ret = 0;
    hpssix_db_t db = { 0, };

    ret = hpssix_db_connect(&db, &config);
    if (ret)
        return ret;

    ret = hpssix_db_initialize(&db);
    hpssix_db_disconnect(&db);

    return ret;
}

const char *workdata_sql =
"SELECT o.oid, o.st_mode, o.st_size, f.path\n"
"  FROM (SELECT oid, st_mode, st_size FROM __object WHERE st_size > 0) o\n"
"       NATURAL JOIN __file f;\n";

static int hpssix_builder_create_workdata(hpssix_builder_t *self)
{
    int ret = 0;
    hpssix_workdata_t workdata = { 0, };
    char builder_output[PATH_MAX] = { 0, };
    PGresult *res = NULL;
    int i = 0;
    int rows = 0;
    uint64_t count = 0;

    sprintf(builder_output, "%s/builder.%lu.db", datadir, task_id);

    ret = hpssix_workdata_create(&workdata, builder_output);
    if (ret)
        goto out;

    hpssix_workdata_begin_transaction(&workdata);

    res = hpssix_db_psql_query(self->db, workdata_sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        ret = EIO;
        goto out_finish;
    }

    rows = PQntuples(res);

    for (i = 0; i < rows; i++) {
        uint64_t oid = strtoull(PQgetvalue(res, i, 0), NULL, 0);
        mode_t st_mode = atoi(PQgetvalue(res, i, 1));
        size_t st_size = strtoull(PQgetvalue(res, i, 2), NULL, 0);
        char *path = PQgetvalue(res, i, 3);

        if (!builder_filter_meta_extract(self, path, st_mode, st_size))
            continue;

        ret = hpssix_workdata_append(&workdata, oid, path);
        if (ret)
            break;

        count++;
    }

out_finish:
    if (ret)
        hpssix_workdata_rollback_transaction(&workdata);
    else {
        hpssix_workdata_end_transaction(&workdata);
        self->n_regular_files = count;
    }

out:
    return ret;
}

/*
 * processing new/updated files in the following order:
 *
 * 1) insert/update object (hpssix_object, object id and stat attrs)
 * 2) insert/update filepath (hpssix_file)
 * 3) insert/update xattrs (hpssix_attr_key, hpssix_attr_val)
 * 4) mark deleted files (hpssix_object)
 */
static int do_builder(void)
{
    int ret = 0;
    hpssix_db_t db = { 0, };
    char sql[4096] = { 0, };

    ret = hpssix_db_connect(&db, &config);
    if (ret)
        return ret;


    builder.config = &config;
    builder.db = &db;
    builder.task_id = task_id;
    builder.datadir = datadir;
    builder.filter = filter;

    hpssix_db_begin_transaction(&db);

    ret = hpssix_builder_process_fattr(&builder);
    if (ret) {
        fprintf(stderr, "## failed to process fattr.\n");
        goto out_finish;
    }

    ret = hpssix_builder_process_path(&builder);
    if (ret) {
        fprintf(stderr, "## failed to process path.\n");
        goto out_finish;
    }

    ret = hpssix_builder_process_xattrs(&builder);
    if (ret) {
        fprintf(stderr, "## failed to process xattrs.\n");
        goto out_finish;
    }

    ret = hpssix_builder_process_deleted(&builder);
    if (ret) {
        fprintf(stderr, "## failed to process the deleted files.\n");
        goto out_finish;
    }

    ret = hpssix_builder_create_workdata(&builder);
    if (ret) {
        fprintf(stderr, "## failed to create the workdata.\n");
        goto out_finish;
    }

    printf("## files indexed: %lu\n", builder.n_processed);
    printf("## regular files: %lu\n", builder.n_regular_files);

out_finish:
    if (ret)
        hpssix_db_rollback(&db);
    else
        hpssix_db_end_transaction(&db);

out:
    hpssix_db_disconnect(&db);

    return ret;
}

static char *program;

static struct option const long_opts[] = {
    { "help", 0, 0, 'h' },
    { "init", 0, 0, 'i' },
    { 0, 0, 0, 0 },
};

static const char *short_opts = "hi";

static const char *usage_str =
"\n"
"Usage: %s [options] <datadir> <task id>\n"
"\n"
"-h, --help             print this help message\n"
"-i, --init             initialize the database\n"
"\n";

static void usage(int status)
{
    printf(usage_str, program);
    exit(status);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int optidx = 0;

    program = hpssix_path_basename(argv[0]);

    while ((ch = getopt_long(argc, argv,
                             short_opts, long_opts, &optidx)) >= 0) {
        switch (ch) {
        case 'i':
            initdb = 1;
            break;

        case 'h':
        default:
            usage(0);
            break;
        }
    }

    if (!initdb) {
        if (argc - optind != 2)
            usage(1);

        datadir = strdup(argv[optind++]);
        task_id = strtoull(argv[optind++], NULL, 0);
    }

    ret = hpssix_config_read_sysconf(&config);
    if (ret) {
        fprintf(stderr, "hpssix_config_read_sysconf: %s\n", strerror(errno));
        ret = errno;
        goto out;
    }

    if (initdb) {
        ret = db_initialize();
        goto out;
    }

    filter = hpssix_extfilter_get();
    if (!filter) {
        fprintf(stderr, "failed to load the extfilter: %s\n", strerror(errno));
        goto out;
    }

    ret = do_builder();
    if (ret) {
        fprintf(stderr, "do_builder: %s\n", strerror(ret));
        return errno;
    }

    hpssix_extfilter_free(filter);
out:
    free(datadir);
    return ret;
}
