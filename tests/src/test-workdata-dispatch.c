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

static hpssix_workdata_t wd;
static char *dbpath;
static uint64_t offset;
static uint64_t count;
static hpssix_workdata_worklist_t list;

int main(int argc, char **argv)
{
    int ret = 0;
    uint64_t i = 0;
    char name[64] = { 0, };

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <dbpath> <offset> <count>\n", argv[0]);
        return 0;
    }

    dbpath = argv[1];
    offset = strtoull(argv[2], 0, 0);
    count = strtoull(argv[3], 0, 0);

    assert(0 == hpssix_workdata_open(&wd, dbpath));

    /*
     * get the total count
     */
    ret = hpssix_workdata_get_total_count(&wd, &i);
    assert(ret == 0);

    printf("The workdata has %lu records.\n", i);

    /*
     * dispatch test
     */
    list.offset = offset;
    list.count = count;

    ret = hpssix_workdata_dispatch(&wd, &list);
    assert(ret == 0);

    printf("%lu records fetched:\n", list.count);

    for (i = 0; i < list.count; i++) {
        hpssix_workdata_object_t *current = &list.object_list[i];
        printf("oid: %lu, path: %s\n", current->object_id, current->path);
    }

    hpssix_workdata_cleanup_object_list(list.object_list, list.count);

    assert(0 == hpssix_workdata_close(&wd));

    return 0;
}

