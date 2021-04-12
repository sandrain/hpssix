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
#include <unistd.h>

#include <hpssix.h>

#include "hpssixd.h"

enum {
    HPSSIXD_CTX_SCANNER = 0,
    HPSSIXD_CTX_BUILDER,
    HPSSIXD_CTX_EXTRACTOR,

    N_HPSSIXD_CTX,
};

static int context = -1;
static int foreground;
static int systemd;

static FILE *pidfp;

static hpssixd_daemon_func_t daemon_funcs[] = {
    &hpssixd_daemon_scanner,
    &hpssixd_daemon_builder,
    &hpssixd_daemon_extractor,
};

static inline hpssixd_daemon_func_t get_daemon_func(int context)
{
    return daemon_funcs[context];
}

static hpssixd_daemon_data_t data;

static char *program;

static struct option const long_opts[] = {
    { "builder", 0, 0, 'b' },
    { "debug", 0, 0, 'd' },
    { "extractor", 0, 0, 'e' },
    { "foreground", 0, 0, 'f' },
    { "help", 0, 0, 'h' },
    { "scanner", 0, 0, 's' },
    { "systemd", 0, 0, 't' },
    { 0, 0, 0, 0 },
};

static const char *short_opts = "bdefhst";

static const char *usage_str =
"\n"
"Usage: %s [options]\n"
"\n"
"-b, --builder      Launch the builder daemon.\n"
"-d, --debug        Print debug output (implies --foreground).\n"
"-e, --extractor    Launch the extractor daemon.\n"
"-f, --foreground   Do not daemonize but run as a foreground process.\n"
"-h, --help         Print this help message\n"
"-s, --scanner      Launch the scanner daemon.\n"
"-t, --systemd      Launch the systemd daemon (default SysV daemon).\n"
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
    hpssixd_daemon_func_t worker = NULL;
    hpssix_config_t *config = &data.config;

    program = hpssix_path_basename(argv[0]);

    while ((ch = getopt_long(argc, argv,
                             short_opts, long_opts, &optidx)) >= 0) {
        switch (ch) {
        case 'b':
            context = HPSSIXD_CTX_BUILDER;
            data.name = "builder";
            break;

        case 'd':
            data.debug = 1;
            foreground = 1;
            break;

        case 'e':
            context = HPSSIXD_CTX_EXTRACTOR;
            data.name = "extractor";
            break;

        case 'f':
            foreground = 1;
            break;

        case 's':
            context = HPSSIXD_CTX_SCANNER;
            data.name = "scanner";
            break;

        case 't':
            systemd = 1;
            break;

        case 'h':
        default:
            usage(0);
            break;
        }
    }

    if (context < 0)
        usage(-1);

    ret = hpssix_config_read_sysconf(config);
    if (ret) {
        fprintf(stderr, "cannot read the configuration file.\n");
        goto out;
    }

    if (foreground) {
        pidfp = hpssixd_setup_pidfile(&data);
        if (!pidfp) {
            fprintf(stderr, "failed to setup pidfile.\n");
            goto out;
        }
    }
    else {
        ret = hpssixd_daemonize(&data, systemd);
        if (ret) {
            fprintf(stderr, "Failed to daemonize the process.. ");
            goto out;
        }
    }

    worker = get_daemon_func(context);
    ret = worker(&data);

out:
    return ret;
}

