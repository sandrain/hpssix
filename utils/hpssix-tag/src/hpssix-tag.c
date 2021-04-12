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
#include <sys/types.h>
#include <sys/stat.h>
#include <mntent.h>
#include <hpssix.h>

#include "hpssix-tag.h"

static hpssix_tag_cmd_t *cmds[] = {
    &hpssix_tag_cmd_set,
    &hpssix_tag_cmd_get,
    &hpssix_tag_cmd_del,
    &hpssix_tag_cmd_list,
};

hpssix_config_t *hpssix_config;
const char *hpssix_mountpoint;

static hpssix_config_t _config;

char *hpssix_tag_program_name;

static const char *usage_str =
"\n"
"Usage: %s <> [options] ..\n"
"\n";

static void usage(int status)
{
    int i = 0;
    int n_cmds = sizeof(cmds)/sizeof(*cmds);

    printf("\nUsage: %s <", hpssix_tag_program_name);

    for (i = 0; i < n_cmds; i++)
        printf("%s%c", cmds[i]->name, i == n_cmds-1 ? '>' : '|');

    printf(" [options] ..\n\n");

    for (i = 0; i < n_cmds; i++)
        printf("%s %s", hpssix_tag_program_name, cmds[i]->usage_str);

    exit(status);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int i = 0;
    char *cmd = NULL;
    hpssix_tag_cmdfunc_t func = NULL;

    hpssix_tag_program_name = hpssix_path_basename(argv[0]);

    if (argc < 2)
        usage(1);

    cmd = argv[1];

    for (i = 0; i < sizeof(cmds)/sizeof(hpssix_tag_cmd_t *); i++) {
        hpssix_tag_cmd_t *current = cmds[i];

        if (0 == strcmp(cmd, current->name)) {
            func = current->func;
            break;
        }
    }

    if (!func)
        usage(1);

    ret = hpssix_config_read_sysconf(&_config);
    if (ret) {
        fprintf(stderr, "failed to read hpssix configuration file (%d:%s)\n",
                        errno, strerror(errno));
        goto out;
    }

    hpssix_mountpoint = hpssix_find_hpss_mountpoint();
    if (!hpssix_mountpoint) {
        fprintf(stderr, "hpssfs is not mounted on this system.\n");
        ret = EINVAL;
        goto out;
    }

    hpssix_config = &_config;

    ret = func(argc - 1, &argv[1]);
out:
    return ret;
}
