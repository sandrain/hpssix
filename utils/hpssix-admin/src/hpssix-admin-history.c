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
#include <time.h>

#include "hpssix-admin.h"

static uint64_t count = 10;     /* default 10 */
static int verbose;

#define NRECPERFETCH    10
static uint64_t fetched;
static hpssix_work_status_t records[NRECPERFETCH];

static hpssix_mdb_t _mdb;

static inline void print_history_record(uint64_t len)
{
    uint64_t i;
    hpssix_work_status_t *current = NULL;
    struct tm *tm = NULL;
    char timebuf[128] = { 0, };

    for (i = 0; i < len; i++) {
        current = &records[i];
        time_t start = current->scanner_start;

        tm = localtime(&start);
        strftime(timebuf, 128, "%Y-%m-%d %H:%M:%S", tm);

        printf("## task at %s: took %4lu seconds, "
               "%4lu scanned, %4lu deleted, "
               "%4lu indexed, %4lu extracted.\n",
               timebuf,
               current->extractor_end - current->scanner_start,
               current->n_scanned,
               current->n_deleted,
               current->n_indexed,
               current->n_extracted);
    }
}

static int do_history_fetch(hpssix_mdb_t *mdb)
{
    int ret = 0;
    int i = 0;
    uint64_t offset = 0;
    uint64_t len = 0;
    uint64_t to_fetch = NRECPERFETCH;
    uint64_t left = 0;

    while (count > fetched) {
        left = count - fetched;
        if (to_fetch > left)
            to_fetch = left;

        ret = hpssix_mdb_fetch_history(mdb, offset, to_fetch, &len, records);
        if (ret)
            goto out;

        if (0 == len)
            break;

        print_history_record(len);
        offset += len;
        fetched += len;
    }
out:
    return ret;
}

static int do_history(void)
{
    int ret = 0;
    hpssix_mdb_t *mdb = &_mdb;

    ret = hpssix_mdb_open(mdb);
    if (ret) {
        fprintf(stderr, "failed to connect to the log database.\n");
        goto out;
    }

    ret = do_history_fetch(mdb);
    if (ret)
        fprintf(stderr, "failed to fetch history records.\n");
    else
        printf("## %lu records fetched.\n", fetched);

out_close:
    hpssix_mdb_close(mdb);
out:
    return ret;
}

#define HPSSIX_ADMIN_CMD_HISTORY_USAGE \
    "history [options]..\n" \
    "\n" \
    "Available options:\n" \
    "-a, --all            print all history\n" \
    "-c, --count=<N>      print <N> history\n" \
    "-h, --help           print this help message\n" \
    "-v, --verbose        print in detail\n" \
    "\n"

static void history_usage(void)
{
    printf("%s %s", hpssix_admin_program_name, HPSSIX_ADMIN_CMD_HISTORY_USAGE);
}

static const char *short_opts = "ac:hv";

static struct option const long_opts[] = {
    { "all", 0, 0, 'a' },
    { "count", 1, 0, 'c' },
    { "help", 0, 0, 'h' },
    { "verbose", 0, 0, 'v' },
    { 0, 0, 0, 0},
};

static int history_cmd_main(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int optidx = 0;

    while ((ch = getopt_long(argc, argv,
                             short_opts, long_opts, &optidx)) >= 0) {
        switch (ch) {
        case 'a':
            count = 0;
            break;

        case 'c':
            count = strtoul(optarg, NULL, 0);
            break;

        case 'v':
            verbose = 1;
            break;

        case 'h':
        default:
            history_usage();
            goto out;
        }
    }

    if (argc - optind != 0) {
        history_usage();
        goto out;
    }

    if (!count)
        count = NRECPERFETCH;

    ret = do_history();
out:
    return ret;
}

hpssix_admin_cmd_t hpssix_admin_cmd_history = {
    .name = "history",
    .usage_str = HPSSIX_ADMIN_CMD_HISTORY_USAGE,
    .func = history_cmd_main,
};

