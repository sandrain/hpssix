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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mntent.h>
#include <hpssix.h>

static hpssix_db_t db;
static hpssix_config_t config;

static char *filename;
static int print_meta;
static int print_text;
static int auto_prefix;

static char sqlbuf[8192];

static PGconn *dbconn;

static char *find_hpss_mountpoint(void)
{
    struct mntent *ent = NULL;
    FILE *fp = NULL;
    int found = 0;
    char *mountpoint = NULL;

    fp = setmntent("/proc/mounts", "r");
    if (!fp) {
        perror("cannot read mounted file information");
        goto out;
    }

    while (NULL != (ent = getmntent(fp))) {
        if (0 == strcmp(ent->mnt_type, "fuse.hpssfs")) {
            found = 1;
            mountpoint = strdup(ent->mnt_dir);
            if (!mountpoint)
                perror("cannot allocate memory");
            break;
        }
    }

    endmntent(fp);

    if (!found)
        fprintf(stderr, "cannot find the hpss mountpoint\n");
out:
    return mountpoint;
}

static inline char *get_sql(const char *path)
{
    char *escaped_path = PQescapeLiteral(dbconn, path, strlen(path));

    if (!escaped_path) {
        fprintf(stderr, "cannot process the pathname %s\n", path);
        return NULL;
    }

    sprintf(sqlbuf, "select meta, text from hpssix_attr_document "
                    "where oid=(select oid from hpssix_file where "
                    "path=%s limit 1)", escaped_path);

    return sqlbuf;
}

static inline void print_result(PGresult *res)
{
    int rows = PQntuples(res);

    printf("## file: %s\n", filename);

    if (rows == 0) {
        printf("## no document metadata found.\n");
        return;
    }

    if (print_meta) {
        printf("\n## document metadata\n");
        printf("%s\n", PQgetvalue(res, 0, 0));
    }

    if (print_text) {
        printf("\n## fulltext\n");
        printf("%s", PQgetvalue(res, 0, 1));
    }
}

static int do_meta(void)
{
    int ret = 0;
    char *mountpoint = NULL;
    char *path = NULL;
    char *sql = NULL;
    size_t len = 0;
    PGresult *res = NULL;

    mountpoint = hpssix_find_hpss_mountpoint();
    if (!mountpoint) {
        ret = ENOENT;
        goto out;
    }

    len = strlen(mountpoint);

    if (0 != strncmp(filename, mountpoint, strlen(mountpoint))) {
        fprintf(stderr, "%s is not a file inside hpss (%s).\n",
                        filename, mountpoint);
        return EINVAL;
    }

    free(mountpoint);

    path = &filename[len];
    sql = get_sql(path);
    if (!sql) {
        ret = EIO;
        goto out;
    }

    res = hpssix_db_psql_query(&db, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "db query failed.\n");
        ret = EIO;
        goto out;
    }

    print_result(res);

out:
    return ret;
}

/*
 * main program
 */
static char *program;

static struct option const long_opts[] = {
    { "auto-prefix", 0, 0, 'a' },
    { "help", 0, 0, 'h' },
    { "meta", 0, 0, 'm' },
    { "text", 0, 0, 't' },
    { 0, 0, 0, 0 },
};

static const char *short_opts = "ahmt";

static const char *usage_str =
"\n"
"Usage: %s [options] ... <pathname>\n"
"\n"
"  -a, --auto-prefix       Automatically prefix the HPSS mountpoint.\n"
"  -h, --help              Print this help message.\n"
"  -m, --meta              Only print the document metadata.\n"
"  -t, --text              Only print the content.\n"
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
    char *path = NULL;
    char *hpssmnt = NULL;

    program = hpssix_path_basename(argv[0]);

    while ((ch = getopt_long(argc, argv,
                             short_opts, long_opts, &optidx)) >= 0) {
        switch (ch) {
        case 'a':
            auto_prefix = 1;
            break;

        case 'm':
            print_meta = 1;
            break;

        case 't':
            print_text = 1;
            break;

        case 'h':
        default:
            usage(0);
            break;
        }
    }

    if (argc - optind != 1)
        usage(1);

    path = argv[optind];

    if (auto_prefix) {
        char *pos = NULL;

        hpssmnt = find_hpss_mountpoint();
        if (!hpssmnt)
            goto out;

        filename = malloc(strlen(hpssmnt)+strlen(path));
        if (!filename)
            goto out;

        pos = filename;

        pos += sprintf(pos, "%s", hpssmnt);
        if (path[0] != '/')
            pos += sprintf(pos, "/");

        pos += sprintf(pos, "%s", path);

        free(hpssmnt);
    }
    else
        filename = realpath(path, NULL);    /* should be freed */

    if (!filename) {
        fprintf(stderr, "cannot resolve the path %s (%s)\n", strerror(errno));
        goto out;
    }

    if (print_meta + print_text == 0)
        print_meta = print_text = 1;

    ret = hpssix_config_read_sysconf(&config);
    if (ret) {
        fprintf(stderr, "failed to read the configuration file.\n");
        goto out_free;
    }

    ret = hpssix_db_connect(&db, &config);
    if (ret) {
        fprintf(stderr, "failed to connect to the database.\n");
        goto out_free;
    }

    dbconn = db.dbconn;

    ret = do_meta();

    hpssix_db_disconnect(&db);
out_free:
    if (filename)
        free(filename);
out:
    return ret;
}
