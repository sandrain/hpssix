/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_WORKDATA_H
#define __HPSSIX_WORKDATA_H

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "hpssix.h"

struct _hpssix_workdata {
    hpssix_config_t config;
    hpssix_tmpdb_t tmpdb;
};

typedef struct _hpssix_workdata hpssix_workdata_t;

struct _hpssix_workdata_object {
    uint64_t object_id;
    char *path;
};

typedef struct _hpssix_workdata_object hpssix_workdata_object_t;

struct _hpssix_workdata_worklist {
    uint64_t offset;    /* starting from 0 */
    uint64_t count;     /* this becomes the # of elements in @object_list */

    hpssix_workdata_object_t *object_list;
};

typedef struct _hpssix_workdata_worklist hpssix_workdata_worklist_t;

static inline void
hpssix_workdata_cleanup_object_list(hpssix_workdata_object_t *list,
                                    uint64_t count)
{
    uint64_t i = 0;

    if (list) {
        for (i = 0; i < count; i++)
            if (list[i].path)
                free(list[i].path);
        free(list);
    }
}

/**
 * @brief
 *
 * @param self
 * @param name
 *
 * @return
 */
int hpssix_workdata_create(hpssix_workdata_t *self, const char *name);

/**
 * @brief
 *
 * @param self
 * @param name
 * @param flags
 *
 * @return
 */
int hpssix_workdata_open(hpssix_workdata_t *self, const char *name);

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
int hpssix_workdata_close(hpssix_workdata_t *self);

/**
 * @brief
 *
 * @param self
 * @param pid
 * @param path
 *
 * @return
 */
int hpssix_workdata_append(hpssix_workdata_t *self,
                           uint64_t pid, const char *path);

/**
 * @brief
 *
 * @param self
 * @param count
 *
 * @return
 */
int hpssix_workdata_get_total_count(hpssix_workdata_t *self, uint64_t *count);

/**
 * @brief
 *
 * @param self
 * @param items
 *
 * @return
 */
int hpssix_workdata_dispatch(hpssix_workdata_t *self,
                             hpssix_workdata_worklist_t *list);

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_workdata_begin_transaction(hpssix_workdata_t *self)
{
    return hpssix_tmpdb_begin_transaction(&self->tmpdb);
}

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_workdata_end_transaction(hpssix_workdata_t *self)
{
    return hpssix_tmpdb_end_transaction(&self->tmpdb);
}

/**
 * @brief
 *
 * @param self
 *
 * @return
 */
static inline int hpssix_workdata_rollback_transaction(hpssix_workdata_t *self)
{
    return hpssix_tmpdb_rollback_transaction(&self->tmpdb);
}

#endif /* __HPSSIX_WORKDATA_H */

