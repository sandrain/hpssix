/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_UTILS_H
#define __HPSSIX_UTILS_H
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "hpssix-config.h"

/**
 * @brief
 *
 * @return
 */
const char *hpssix_get_work_root(void);

/**
 * @brief
 *
 * @param pathbuf
 * @param buflen
 */
int hpssix_get_datadir_today(char *pathbuf, uint64_t buflen);

/**
 * @brief
 *
 * @return
 */
int hpssix_create_work_directories(void);

/**
 * @brief
 *
 * @return hpss mountpoint, the string is allocated by strdup(3) and should be
 * freed by caller.
 */
char *hpssix_find_hpss_mountpoint(void);

/**
 * @brief
 *
 * @param path
 *
 * @return
 */
static inline char *hpssix_path_basename(const char *path)
{
    char *exe = NULL;

    if (path) {
        char *tmp = strdup(path);
        exe = basename(tmp);
        free(tmp);
    }

    return exe;
}

/**
 * @brief
 *
 * @param line
 *
 * @return
 */
static inline int is_line_empty(const char *line)
{
    int i = 0;

    if (!line)
        return 1;

    if (line[0] == '#')
        return 1;

    for (i = 0; i < strlen(line); i++)
        if (!isspace(line[i]))
            return 0;

    return 1;
}

/**
 * @brief
 *
 * @param start
 * @param end
 *
 * @return
 */
static inline double timediff_sec(struct timeval *start, struct timeval *end)
{
    double sec = .0F;

    if (!start || !end)
        goto out;

    sec = 1.0F*(end->tv_sec - start->tv_sec);
    sec += 1.0F*(end->tv_usec - start->tv_usec)/1e6;

out:
    return sec;
}

#endif /* __HPSSIX_UTILS_H */
