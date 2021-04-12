/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_MDB_H
#define __HPSSIX_MDB_H

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "hpssix.h"

struct _hpssix_mdb {
    hpssix_config_t config;
    hpssix_tmpdb_t tmpdb;
};

typedef struct _hpssix_mdb hpssix_mdb_t;

struct _hpssix_work_status {
    uint64_t id;
    uint64_t oid_start;
    uint64_t oid_end;
    uint64_t scanner_start;
    uint64_t builder_start;
    uint64_t extractor_start;
    uint64_t extractor_end;
    uint64_t n_scanned;
    uint64_t n_deleted;
    uint64_t n_indexed;
    uint64_t n_extracted;
    int status;
};

typedef struct _hpssix_work_status hpssix_work_status_t;

/**
 * @brief
 *
 * @param mdb
 *
 * @return
 */
int hpssix_mdb_open(hpssix_mdb_t *mdb);

/**
 * @brief
 *
 * @param mdb
 *
 * @return
 */
int hpssix_mdb_close(hpssix_mdb_t *mdb);

/**
 * @brief
 *
 * @param mdb
 * @param oid
 *
 * @return
 */
int hpssix_mdb_record(hpssix_mdb_t *mdb, hpssix_work_status_t *status);

/**
 * @brief
 *
 * @param mdb
 *
 * @return
 */
int hpssix_mdb_get_last_scanned_oid(hpssix_mdb_t *mdb, uint64_t *oid);

/**
 * @brief
 *
 * @param mdb
 * @param oid
 * @param ts
 *
 * @return
 */
int hpssix_mdb_get_last_scanned_status(hpssix_mdb_t *mdb,
                                       uint64_t *oid, uint64_t *ts);

/**
 * @brief
 *
 * @param mdb
 * @param offset
 * @param len
 * @param outlen
 * @param status
 *
 * @return
 */
int hpssix_mdb_fetch_history(hpssix_mdb_t *mdb, uint64_t offset, uint64_t len,
                             uint64_t *outlen, hpssix_work_status_t *status);

#endif /* __HPSSIX_MDB_H */

