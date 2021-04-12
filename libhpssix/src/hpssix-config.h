/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_CONFIG_H
#define __HPSSIX_CONFIG_H

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

struct _hpssix_config {
    char *db_host;
    char *db_user;
    char *db_password;
    char *db_database;
    uint16_t db_port;

    char *tag_prefix;

    char *workdir;
    char *hpss_mountpoint;
    char *db2path;
    char *db2user;
    char *db2password;

    char *tika_host;
    uint16_t tika_port;

    uint32_t extractor_nthreads;
    uint64_t extractor_maxfilesize;

    char *scanner_host;
    char *builder_host;
    char *extractor_host;

    uint64_t rpc_timeout;
    uint64_t first_oid;

    struct tm schedule;
};

typedef struct _hpssix_config hpssix_config_t;

/**
 * @brief read the sysconf file (e.g., /etc/hpssix.conf) and populate the given
 * hpssix_config_t structure.
 *
 * @param hpssix should be allocated by the caller.
 *
 * @return 0 on success, errno otherwise.
 */
int hpssix_config_read_sysconf(hpssix_config_t *config);

/**
 * @brief
 *
 * @param hpssix
 *
 * @return
 */
void hpssix_config_free(hpssix_config_t *config);

/**
 * @brief
 *
 * @param hpssix
 * @param stream
 *
 * @return
 */
int hpssix_config_dump(hpssix_config_t *config, FILE *stream);

struct _hpssix_extfilter {
    uint64_t n_exts;
    const char *exts[0];
};

typedef struct _hpssix_extfilter hpssix_extfilter_t;

/**
 * @brief
 *
 * @return
 */
hpssix_extfilter_t *hpssix_extfilter_get(void);

/**
 * @brief
 *
 * @param filter
 */
void hpssix_extfilter_free(hpssix_extfilter_t *filter);

#endif  /* HPSSIX_CONFIG_H */

