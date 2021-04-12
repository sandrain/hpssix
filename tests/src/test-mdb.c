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
#include <assert.h>
#include <time.h>
#include <hpssix.h>

#include "testlib.h"

static hpssix_mdb_t mdb;
static hpssix_work_status_t status;

int main(int argc, char **argv)
{
    int ret = 0;
    uint64_t oid = 0;
    uint64_t ts = 0;
    char datebuf[11] = { 0, };

    ret = hpssix_mdb_open(&mdb);
    assert(!ret);

#if 0
    status.type = HPSSIX_WORKTYPE_NEW_INSERT;
    status.oid_start = 1;
    status.oid_end = 30000;
    status.scanner_start = 100;
    status.builder_start = 160;
    status.extractor_start = 190;
    status.extractor_end = 200;
    status.n_scanned = 10000;
    status.n_indexed = 10000;
    status.n_extracted = 100;
    status.status = 0;

    ret = hpssix_mdb_record(&mdb, &status);
    assert(!ret);
#endif

    ret = hpssix_mdb_get_last_scanned_status(&mdb, &oid, &ts);
    assert(!ret);

    ts -= 60*60*24;

    strftime(datebuf, 11, "%F", localtime(&ts));

    printf("last oid=%lu, ts=%lu (%s)\n",
           (unsigned long) oid, (unsigned long) ts, datebuf);


    ret = hpssix_mdb_close(&mdb);

    return ret;
}
