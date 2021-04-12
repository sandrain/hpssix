/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIXD_H
#define __HPSSIXD_H

#include <config.h>

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdarg.h>
#include <hpssix.h>

struct _hpssixd_daemon_data {
    hpssix_config_t config;

    const char *name;
    int debug;
};

typedef struct _hpssixd_daemon_data hpssixd_daemon_data_t;

typedef int (*hpssixd_daemon_func_t)(hpssixd_daemon_data_t *);

int hpssixd_daemon_scanner(hpssixd_daemon_data_t *data);

int hpssixd_daemon_builder(hpssixd_daemon_data_t *data);

int hpssixd_daemon_extractor(hpssixd_daemon_data_t *data);

/**
 * @brief daemonize the process. when using the traditional daemonizing method
 * (i.e., @systemd=0), the following signals will be handled by the default
 * handler which just logs the signal number:
 *
 *     signal(SIGSEGV, hpssixd_log_signal);
 *     signal(SIGABRT, hpssixd_log_signal);
 *     signal(SIGILL, hpssixd_log_signal);
 *     signal(SIGTRAP, hpssixd_log_signal);
 *     signal(SIGFPE, hpssixd_log_signal);
 *     signal(SIGBUS, hpssixd_log_signal);
 *     signal(SIGINT, hpssixd_log_signal);
 *     signal(SIGPIPE, SIG_IGN);
 *
 * @param data the daemon data.
 * @param systemd whether to use systemd daemon or traditional one (sysv).
 *
 * @return 0 on success, errno otherwise.
 */
int hpssixd_daemonize(hpssixd_daemon_data_t *data, int systemd);

/**
 * @brief
 *
 * @param data
 *
 * @return
 */
FILE *hpssixd_setup_pidfile(hpssixd_daemon_data_t *data);

static inline int hpssixd_daemon_terminate(hpssixd_daemon_data_t *data)
{
    return 0;
}

#if 0
enum {
    HPSSIXD_LOGI    = 0,
    HPSSIXD_LOGW,
    HPSSIXD_LOGE,
};
#endif

/**
 * @brief
 *
 * @param level
 * @param error
 * @param format
 * @param ...
 *
 * @return
 */
#if 0
int hpssixd_log(hpssixd_daemon_data_t *data, int level, int error,
                const char *format, ...);
#endif

static inline void catch_line_output(char *line, const char *key, uint64_t *val)
{
    if (strncmp(line, key, strlen(key)) == 0) {
        char *pos = strrchr(line, ':');
        *val = strtoull(&pos[2], NULL, 0);
    }
}

enum {
    HPSSIXD_LOG_EMERG   = 0,
    HPSSIXD_LOG_ALERT   = 1,
    HPSSIXD_LOG_CRIT    = 2,
    HPSSIXD_LOG_ERR     = 3,
    HPSSIXD_LOG_WARNING = 4,
    HPSSIXD_LOG_NOTICE  = 5,
    HPSSIXD_LOG_INFO    = 6,
    HPSSIXD_LOG_DEBUG   = 7,
};

void hpssixd_log(int level, const char *fmt, va_list args);

static inline void hpssixd_log_emerg(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_EMERG, fmt, args);
    va_end(args);
}

static inline void hpssixd_log_alert(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_ALERT, fmt, args);
    va_end(args);
}

static inline void hpssixd_log_crit(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_CRIT, fmt, args);
    va_end(args);
}

static inline void hpssixd_log_err(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_ERR, fmt, args);
    va_end(args);
}

static inline void hpssixd_log_warning(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_WARNING, fmt, args);
    va_end(args);
}

static inline void hpssixd_log_notice(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_NOTICE, fmt, args);
    va_end(args);
}

static inline void hpssixd_log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_INFO, fmt, args);
    va_end(args);
}

static inline void hpssixd_log_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hpssixd_log(HPSSIXD_LOG_DEBUG, fmt, args);
    va_end(args);
}

void hpssixd_daemon_close(void);

enum {
    HPSSIXD_ID_SCANNER = 0,
    HPSSIXD_ID_BUILDER = 1,
    HPSSIXD_ID_EXTRACTOR = 2,

    HPSSIXD_TASK_SCAN_HPSS = 0,
    HPSSIXD_TASK_BUILD_IX = 1,
    HPSSIXD_TASK_EXTRACT_META = 2,

    HPSSIXD_RC_SUCCESS = 0,
    HPSSIXD_RC_FAIL = 1,
};

#include "hpssixd-comm.h"

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

#endif /* __HPSSIXD_H */

