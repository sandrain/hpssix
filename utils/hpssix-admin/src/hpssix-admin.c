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

#include "hpssix-admin.h"

static hpssix_admin_cmd_t *cmds[] = {
    &hpssix_admin_cmd_history,
};

/*
 * main program
 */
char *hpssix_admin_program_name;

static void usage(int status)
{
    int i = 0;

    for (i = 0; i < sizeof(cmds)/sizeof(*cmds); i++)
        printf("%s %s", hpssix_admin_program_name, cmds[i]->usage_str);

    exit(status);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int i = 0;
    char *cmd = NULL;

    hpssix_admin_program_name = hpssix_path_basename(argv[0]);

    if (argc < 2)
        usage(1);

    cmd = argv[1];

    for (i = 0; i < sizeof(cmds)/sizeof(hpssix_admin_cmd_t *); i++) {
        hpssix_admin_cmd_t *current = cmds[i];

        if (0 == strcmp(cmd, current->name)) {
            ret = current->func(argc - 1, &argv[1]);
            goto out;
        }
    }

    usage(1);

out:
    return ret;
}

