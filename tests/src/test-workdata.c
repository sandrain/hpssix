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
#include <assert.h>

#include <hpssix.h>

static uint64_t count = 100;

static hpssix_workdata_t wd;

int main(int argc, char **argv)
{
    int ret = 0;
    uint64_t i = 0;
    char name[64] = { 0, };

    if (argc == 2)
        count = strtoull(argv[1], 0, 0);

    assert(0 == hpssix_workdata_create(&wd, "test.db"));

    sprintf(name, "file_XXXXXX");

    for (i = 0; i < count; i++) {
        ret = mkstemp(name);
        if (ret < 0) {
            perror("mkstemp");
            break;
        }

        ret = hpssix_workdata_append(&wd, i, name);
        assert(ret == 0);
    }

    assert(0 == hpssix_workdata_close(&wd));

    return 0;
}
