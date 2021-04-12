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
#include <errno.h>
#include <limits.h>

#include "hpssix.h"

int hpssix_context_init(hpssix_context_t *context, hpssix_config_t *config)
{
    time_t timeval = 0;
    struct tm *local_tm = NULL;

    if (!context || !config)
        return EINVAL;

    context->config = config;
    local_tm = &context->local_tm;

    timeval = time(NULL);
    local_tm = localtime_r(&timeval, local_tm);
    if (!local_tm)
        return errno;

    return 0;
}

int hpssix_context_get_workdir(hpssix_context_t *context, char *buf,
                               size_t len)
{
    char pathbuf[PATH_MAX] = { 0, };
    size_t written = 0;
    hpssix_config_t *config = NULL;

    if (!context || !context->config)
        return EINVAL;

    config = context->config;

    written = sprintf(pathbuf, "%s", config->workdir);
    written += strftime(&pathbuf[written], sizeof(pathbuf)-written,
                        "/%Y/%Y%m%d/", &context->local_tm);

    if (written >= len)
        return ENOSPC;

    strncpy(buf, pathbuf, len);

    return 0;
}

void hpssix_log_printf(FILE *fp, const char *fmt, ...)
{
}
