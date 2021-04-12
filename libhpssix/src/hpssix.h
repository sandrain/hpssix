/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_H
#define __HPSSIX_H
#include <config.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "hpssix-utils.h"
#include "hpssix-config.h"
#include "hpssix-tmpdb.h"
#include "hpssix-db.h"
#include "hpssix-mdb.h"
#include "hpssix-workdata.h"

struct _hpssix_context {
    hpssix_config_t *config;
    struct tm local_tm;
};

typedef struct _hpssix_context hpssix_context_t;

/**
 * @brief
 *
 * @param context
 *
 * @return
 */
int hpssix_context_init(hpssix_context_t *context, hpssix_config_t *config);

/**
 * @brief
 *
 * @param context
 * @param buf
 * @param len
 *
 * @return
 */
int hpssix_context_get_workdir(hpssix_context_t *context, char *buf,
                               size_t len);

/**
 * @brief
 *
 * @param fp
 * @param fmt
 * @param ...
 */
void hpssix_log_printf(FILE *fp, const char *fmt, ...);

#endif  /* HPSSIX_H */

