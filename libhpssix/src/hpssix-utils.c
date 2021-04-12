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
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <mntent.h>
#include <fcntl.h>
#include <limits.h>

#include "hpssix-utils.h"

static const char *hpssix_work_root = LOCALSTATEDIR;

static const char *hpssix_dirs[] = { "data" };

/**
 * @brief
 *
 * @param path
 *
 * @return errno on failure, 0 on success.
 */
static int create_directory(const char *path)
{
    int ret = 0;
    struct stat sb = { 0, };

    ret = stat(path, &sb);
    if (ret < 0) {
        if (errno != ENOENT)
            return errno;

        ret = mkdir(path, 0700);
        if (ret < 0)
            return errno;
    }
    else if (!S_ISDIR(sb.st_mode))
        ret = ENOTDIR;

    return ret;
}

static inline void dirname_today(char *buf)
{
    time_t now_time = { 0, };
    struct tm now = { 0, };
    int year = 0;
    int mm = 0;
    int dd = 0;

    if (!buf)
        return;

    now_time = time(NULL);
    localtime_r(&now_time, &now);

    year = now.tm_year + 1900;
    mm = now.tm_mon + 1;
    dd = now.tm_mday;

    sprintf(buf, "%s/data/%d/%d%02d%02d",
                 hpssix_work_root, year, year, mm, dd);
}

/* directory structure:
 *
 * For storing the master db (mdb):
 *
 * ${localstatedir}/hpssix/hpssix.mdb.db
 * 
 * For storing intermediate files, e.g., 20181010, 1540559332 (unix timestamp):
 *
 * ${localstatedir}/hpssix/data/2018/20181010/scanner.1540559332.csv
 * ${localstatedir}/hpssix/data/2018/20181010/builder.1540559332.db
 */
int hpssix_create_work_directories(void)
{
    int i = 0;
    int ret = 0;
    char *pos = NULL;
    char path[PATH_MAX] = { 0, };

    ret = create_directory(hpssix_work_root);
    if (ret)
        goto out;

    for (i = 0; i < sizeof(hpssix_dirs)/sizeof(char *); i++) {
        sprintf(path, "%s/%s", hpssix_work_root, hpssix_dirs[i]);

        ret = create_directory(path);
        if (ret)
            goto out;
    }

    /* create work directories for today */

    dirname_today(path);

    pos = strrchr(path, '/');
    *pos = '\0';

    ret = create_directory(path);
    if (ret)
        goto out;

    *pos = '/';
    ret = create_directory(path);

out:
    return ret;
}

const char *hpssix_get_work_root(void)
{
    return hpssix_work_root;
}

int hpssix_get_datadir_today(char *pathbuf, uint64_t buflen)
{
    int ret = 0;
    struct stat sb = { 0, };

    if (!pathbuf)
        return EINVAL;

    if (buflen < strlen(hpssix_work_root) + 20)
        return ENOSPC;

    dirname_today(pathbuf);

    ret = stat(pathbuf, &sb);
    if (ret < 0 && errno == ENOENT)
        ret = hpssix_create_work_directories();

    return ret;
}

char *hpssix_find_hpss_mountpoint(void)
{
    struct mntent *ent = NULL;
    FILE *fp = NULL;
    int found = 0;
    char *mountpoint = NULL;

    fp = setmntent("/proc/mounts", "r");
    if (!fp)
        goto out;

    while (NULL != (ent = getmntent(fp))) {
        if (0 == strcmp(ent->mnt_type, "fuse.hpssfs")) {
            found = 1;
            mountpoint = strdup(ent->mnt_dir);
            if (!mountpoint)
                goto out;
            break;
        }
    }

    endmntent(fp);
out:
    return mountpoint;
}

