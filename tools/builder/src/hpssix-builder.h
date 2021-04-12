/* Copyright (C) 2019 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_BUILDER_H
#define __HPSSIX_BUILDER_H

#include <config.h>

#include <stdio.h>
#include <stdint.h>

#include <hpssix.h>

#define CEILING(x,y) (((x) + (y) - 1) / (y))

struct _hpssix_builder {
    hpssix_config_t *config;
    hpssix_db_t *db;
    hpssix_extfilter_t *filter;

    uint64_t task_id;
    char *datadir;

    uint64_t n_processed;
    uint64_t n_regular_files;
};

typedef struct _hpssix_builder hpssix_builder_t;

int hpssix_builder_process_fattr(hpssix_builder_t *self);

int hpssix_builder_process_path(hpssix_builder_t *self);

int hpssix_builder_process_xattrs(hpssix_builder_t *self);

int hpssix_builder_process_deleted(hpssix_builder_t *self);

enum {
    SCANNER_OUTPUT_FATTR = 0,
    SCANNER_OUTPUT_PATH = 1,
    SCANNER_OUTPUT_XATTR = 2,
    SCANNER_OUTPUT_DELETED = 3,
    N_SCANNER_OUTPUT_TYPE = 4,
};

static inline char *hpssix_builder_get_scanner_filename(hpssix_builder_t *self,
                                                        char *buf, int type)
{
    if (!buf)
        return NULL;

    switch (type) {
    case SCANNER_OUTPUT_FATTR:
        sprintf(buf, "%s/scanner.%lu.fattr.csv", self->datadir, self->task_id);
        break;

    case SCANNER_OUTPUT_PATH:
        sprintf(buf, "%s/scanner.%lu.path.csv", self->datadir, self->task_id);
        break;

    case SCANNER_OUTPUT_XATTR:
        sprintf(buf, "%s/scanner.%lu.xattr.csv", self->datadir, self->task_id);
        break;

    case SCANNER_OUTPUT_DELETED:
        sprintf(buf, "%s/scanner.%lu.deleted.csv",
                     self->datadir, self->task_id);
        break;

    default:
        return NULL;
    }

    return buf;
}

/*
 * utility functions, defined in hpssix-builder-utils.c.
 */

int hpssix_builder_parse_fattr(const char *line, struct stat *sb);

hpssix_xattr_t **builder_collect_xattrs(const char *file, uint64_t *out_count);

int builder_filter_meta_extract(hpssix_builder_t *self, const char *path,
                                mode_t mode, size_t st_size);

#endif /* __HPSSIX_BUILDER_H */

