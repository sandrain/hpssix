/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_EXTRACTOR_H
#define __HPSSIX_EXTRACTOR_H

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <curl/curl.h>

#include <hpssix.h>

struct _hpssix_extractor_data {
    char *tika_host;
    int tika_port;

    char file[PATH_MAX];
    uint64_t file_size;
    FILE *fp;
    FILE *tmpfp;

    char *meta;
    char *content;
};

typedef struct _hpssix_extractor_data hpssix_extractor_data_t;

/**
 * @brief
 *
 * @param data
 *
 * @return
 */
int hpssix_extractor_get_meta(hpssix_extractor_data_t *data);

/**
 * @brief
 *
 * @param data
 *
 * @return
 */
int hpssix_extractor_get_content(hpssix_extractor_data_t *data);

/**
 * @brief
 *
 * @return
 */
static inline int hpssix_extractor_global_init(void)
{
    curl_global_init(CURL_GLOBAL_ALL);
}

/**
 * @brief
 */
static inline void hpssix_extractor_global_cleanup(void)
{
    curl_global_cleanup();
}

#endif /* __HPSSIX_EXTRACTOR_H */

