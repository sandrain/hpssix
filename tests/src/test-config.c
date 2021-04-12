/* Copyright (C) 2018 - Hyogi Sim <simh@ornl.gov>
 * 
 * Please refer to COPYING for the license.
 * ---------------------------------------------------------------------------
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hpssix.h>

#include "testlib.h"

static hpssix_config_t config;

int main(int argc, char **argv)
{
    int ret = 0;

    ret = hpssix_config_read_sysconf(&config);
    if (ret)
        die("hpssix_config_read_sysconf failed");

    hpssix_config_dump(&config, stdout);

    hpssix_config_free(&config);

    return 0;
}
