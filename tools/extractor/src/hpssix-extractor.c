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
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <curl/curl.h>
#include <sqlite3.h>

#include <hpssix.h>
#include "hpssix-extractor.h"

static char *tika_host = "localhost";
static int tika_port = 9998;

static uint64_t nthreads;
static pthread_t *threads;
static uint64_t total_objects;

static hpssix_config_t config;
static char *dbpath;

static uint64_t n_extracted;
static int verbose;

static struct timeval start, end;

static inline double timediff(struct timeval *t1, struct timeval *t2)
{
    double usec = (t2->tv_sec - t1->tv_sec)*1e6 + 1.0F*(t2->tv_usec - t1->tv_usec);
    return usec/1e6;
}

static int check_builder_output(void)
{
    int ret = 0;
    struct stat sb = { 0, };

    ret = stat(dbpath, &sb);
    if (ret < 0)
        return errno;

    return sb.st_size > 0 ? 0 : EINVAL;
}

static inline
void get_work_allocation(uint64_t id, uint64_t *offset, uint64_t *count)
{
    uint64_t per_each = total_objects / nthreads;
    uint64_t _offset = 0;
    uint64_t _count = 0;

    _offset = per_each * id;
    _count = per_each;

    if (id == nthreads - 1)
        _count += total_objects % nthreads;

    *offset = _offset;
    *count = _count;
}

static
int do_extract(hpssix_workdata_object_t *object, hpssix_db_t *db, uint64_t id)
{
    int ret = 0;
    struct stat sb = { 0, };
    hpssix_extractor_data_t data = { 0, };
    FILE *fp = NULL;

    sprintf(data.file, "%s%s", config.hpss_mountpoint, object->path);

    ret = stat(data.file, &sb);
    if (ret) {
        printf("[%lu]EE: cannot stat %s (%s)\n", id, data.file, strerror(errno));
        return errno;
    }

    data.file_size = sb.st_size;
    /*
     * need to resolve firewall/connection issue to use the full domain name.
     */
    /*data.tika_host = config.tika_host;*/
    data.tika_host = tika_host;
    data.tika_port = config.tika_port;

    fp = fopen(data.file, "r");
    if (!fp) {
        printf("[%lu]EE: cannot fopen %s (%s)\n",
               id, data.file, strerror(errno));
        return errno;
    }

    data.fp = fp;

    fp = tmpfile();
    if (!fp) {
        printf("[%lu]EE: cannot make tmpfile (%s)\n",
               id, data.file, strerror(errno));
        return errno;
    }

    data.tmpfp = fp;

    ret = hpssix_extractor_get_meta(&data);
    if (ret && verbose) {
        if (ret != EINVAL)
            printf("[%lu]EE: hpssix_extractor_get_meta failed (%d:%s)\n",
                   id, strerror(ret));
        goto out;
    }

    ret = hpssix_extractor_get_content(&data);
    if (ret && verbose)
        printf("[%lu]EE: hpssix_extractor_get_content failed (%d:%s)\n",
               id, strerror(ret));

out:
    fclose(data.tmpfp);
    fclose(data.fp);

    if (data.meta) {
        if (verbose)
            printf("[%lu] extracted from %lu, %s (meta: %s, content: ...)\n",
                    id, object->object_id, data.file, data.meta);

        /* index the data */
        ret = hpssix_db_index_tsv(db, object->object_id,
                                  data.meta, data.content);
        if (ret)
            fprintf(stderr, "hpssix_db_index_tsv failed (%d:%s)\n", ret,
                    strerror(ret));
        else
            n_extracted++;

        free(data.meta);
        free(data.content);
    }

    return ret;
}

static void *extractor_worker_func(void *arg)
{
    int ret = 0;
    uint64_t id = (unsigned long) arg;
    uint64_t i = 0;
    hpssix_workdata_t wd = { 0, };
    hpssix_workdata_worklist_t list = { 0, };
    hpssix_db_t db = { 0, };

    get_work_allocation(id, &list.offset, &list.count);

    if (verbose)
        printf("[%lu] work (%lu, %lu)\n", id, list.offset, list.count);

    ret = hpssix_workdata_open(&wd, dbpath);
    if (ret) {
        fprintf(stderr, "[%lu]: hpssix_workdata_open failed\n", id);
        goto out;
    }

    ret = hpssix_workdata_dispatch(&wd, &list);
    hpssix_workdata_close(&wd);
    if (ret) {
        fprintf(stderr, "[%lu]: hpssix_workdata_dispatch failed\n", id);
        goto out;
    }

    ret = hpssix_db_connect(&db, &config);
    if (ret) {
        fprintf(stderr, "[%lu]: hpssix_workdata_db_connect failed\n", id);
        goto out;
    }

    for (i = 0; i < list.count; i++) {
        hpssix_workdata_object_t *current = &list.object_list[i];
        char *meta = NULL;
        char *text = NULL;

        if (verbose)
            printf("[%lu]: processing file %lu/%lu (%s)\n",
                   id, i, list.count, current->path);

        ret = do_extract(current, &db, id);
        if (ret) {
            if (ret == ENOENT || ret == EINVAL)
                continue;

            if (verbose)
                fprintf(stderr, "[%lu]E: do_extract failed (%d)\n", id, ret);
        }
    }

out_disconnect:
    hpssix_db_disconnect(&db);
    hpssix_workdata_cleanup_object_list(list.object_list, list.count);
out:
    return (void *) 0;
}

static char *program;

static struct option long_opts[] = {
    { "help", 0, 0, 'h' },
    { "nthreads", 1, 0, 'n' },
    { "verbose", 0, 0, 'v' },
    { 0, 0, 0, 0},
};

static const char *short_opts = "hn:v";

static const char *usage_str = "\n"
"Usage: extractor [options] <input dbfile>\n"
"\n"
"Available options:\n"
"-h, --help             print the help message.\n"
"-n, --nthreads=<NUM>   number of threads to be spawned. this will override\n"
"                       the value in the configuration file.\n"
"-v, --verbose          print noisy output.\n"
"\n";

static void print_usage(int status)
{
    fputs(usage_str, stderr);
    exit(status);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int optidx = 0;
    int ch = 0;
    uint64_t i = 0;
    hpssix_workdata_t wd = { 0, };
    double elapsed = .0F;

    program = hpssix_path_basename(argv[0]);

    while ((ch = getopt_long(argc, argv,
                             short_opts, long_opts, &optidx)) >= 0) {
        switch (ch) {
        case 'n':
            nthreads = strtoull(optarg, 0, 0);
            break;

        case 'v':
            verbose = 1;
            break;

        case 'h':
        default:
            print_usage(1);
            break;
        }
    }

    if (argc - optind != 1)
        print_usage(1);

    dbpath = argv[optind];

    gettimeofday(&start, NULL);

    ret = hpssix_config_read_sysconf(&config);
    if (ret) {
        fprintf(stderr, "hpssix_config_read_sysconf: %s\n", strerror(errno));
        return errno;
    }

    ret = check_builder_output();
    if (ret) {
        fprintf(stderr, "cannot find the builder output\n");
        return EINVAL;
    }

    if (!nthreads)
        nthreads = config.extractor_nthreads;

    ret = hpssix_workdata_open(&wd, dbpath);
    if (ret) {
        fprintf(stderr, "hpssix_workdata_open: %s\n", strerror(ret));
        return errno;
    }

    ret = hpssix_workdata_get_total_count(&wd, &total_objects);
    if (ret) {
        fprintf(stderr, "hpssix_workdata_get_total: %s\n", strerror(ret));
        return errno;
    }

    hpssix_workdata_close(&wd);

    printf("Total work: %lu\n", total_objects);
    if (total_objects == 0) {
        printf("Nothing to extract.. terminating.\n");
        goto out_donothing;
    }

    hpssix_extractor_global_init();
 
    threads = calloc(nthreads, sizeof(*threads));
    if (!threads) {
        perror("## [E] calloc");
        ret = errno;
        goto out;
    }

    for (i = 0; i < nthreads; i++) {
        ret = pthread_create(&threads[i], 0, extractor_worker_func,
                             (void *) (unsigned long) i);
        if (ret) {
            perror("pthread_create failed");
            goto out;
        }
    }

    for (i = 0; i < nthreads; i++) {
        ret = pthread_join(threads[i], NULL);
        if (ret)
            perror("pthread_join");
    }

out:
    hpssix_extractor_global_cleanup();
    free(threads);

    gettimeofday(&end, NULL);
    elapsed = timediff(&start, &end);

out_donothing:
    printf("## files extracted: %d\n", n_extracted);
    printf("## %.3lf seconds\n", elapsed);

    return ret;
}
