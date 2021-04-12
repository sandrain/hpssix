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
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <execinfo.h>

#include "hpssixd.h"

static hpssixd_daemon_data_t *daemon_data;
static int use_syslog;
static FILE *pidfp;
static char daemon_name[20];

static inline void generate_pidfile_path(char *path, hpssix_config_t *config,
                                         const char *name)
{
    sprintf(path, "%s/pid/%s.pid", config->workdir, name);
}

FILE *hpssixd_setup_pidfile(hpssixd_daemon_data_t *data)
{
    int ret = 0;
    int fd = -1;
    pid_t pid = 0;
    FILE *fp = NULL;
    char path[PATH_MAX] = { 0, };
    hpssix_config_t *config = NULL;

    if (!data) {
        errno = EINVAL;
        goto out;
    }

    daemon_data = data;

    config = &data->config;
    pid = getpid();

    generate_pidfile_path(path, config, data->name);

    fp = fopen(path, "a+");
    if (!fp) {
        fprintf(stderr, "failed to open pid file %s", path);
        goto out;
    }

    fd = fileno(fp);
    ret = lockf(fd, F_TLOCK, 0);
    if (ret) {
        fprintf(stderr, "failed to lock pid file %s", path);
        goto out_close;
    }

    ret = ftruncate(fd, 0);
    if (ret) {
        fprintf(stderr, "failed to truncate pid file %s", path);
        goto out_close;
    }

    ret = fprintf(fp, "%d\n", pid);
    if (ret <= 0) {
        fprintf(stderr, "failed to write pid to pid file %s", path);
        goto out_close;
    }

    ret = fflush(fp);
    if (ret) {
        fprintf(stderr, "failed to flush pid file %s", path);
        goto out_close;
    }

    return fp;

out_close:
    fclose(fp);
    fp = NULL;
out:
    return fp;
}

static const char *systemd_log_prefix[] = {
    "<0>",
    "<1>",
    "<2>",
    "<3>",
    "<4>",
    "<5>",
    "<6>",
    "<7>",
};

static const int syslog_level[] = {
    LOG_EMERG,
    LOG_ALERT,
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG,
};

void hpssixd_log(int level, const char *fmt, va_list args) 
{
    if (level < HPSSIXD_LOG_EMERG || level > HPSSIXD_LOG_DEBUG)
        level = HPSSIXD_LOG_DEBUG;

    if (use_syslog)
        vsyslog(syslog_level[level], fmt, args);
    else {
        fprintf(stderr, "<%d> ", level);
        vfprintf(stderr, fmt, args);
    }
}

static inline void hpssixd_dump_backtrace(void)
{
    int i = 0;
    int size = 0;
    void *bt[64] = { NULL, };
    char **btstr = NULL;

    size = backtrace(bt, 64);
    btstr = backtrace_symbols(bt, size);

    for (i = 0; i < size; i++)
        hpssixd_log_err("%s", btstr[i]);

    hpssixd_log_err("terminating the process..");

    exit(1);
}

static void hpssixd_dump_status(int signum)
{
    hpssixd_log_info("received signal: %s (%d)", strsignal(signum), signum);

    if (signum == SIGSEGV || signum == SIGBUS || signum == SIGFPE)
        hpssixd_dump_backtrace();

    /* TODO: print more useful status */
}

static int hpssixd_setup_signals(void)
{
    int ret = 0;

    signal(SIGSEGV, hpssixd_dump_status);
    signal(SIGABRT, hpssixd_dump_status);
    signal(SIGILL, hpssixd_dump_status);
    signal(SIGTRAP, hpssixd_dump_status);
    signal(SIGFPE, hpssixd_dump_status);
    signal(SIGBUS, hpssixd_dump_status);
    signal(SIGINT, hpssixd_dump_status);
    signal(SIGPIPE, SIG_IGN);

    return ret;
}

static int daemonize_systemd(void)
{
    return ENOSYS;
}

static int daemonize_sysv(void)
{
    int ret = 0;
    int fd = 0;
    pid_t pid = 0;
    pid_t sid = 0;

    if (getppid() == 1) /* already a daemon */
        return ret;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed.\n");
        ret = errno;
        goto out;
    }

    if (pid > 0)
        exit(0);

    /* child */
    sid = setsid();
    if (sid == (pid_t) -1) {
        fprintf(stderr, "failed to set a new sid.\n");
        ret = errno;
        goto out;
    }

    umask(0);

    ret = chdir("/");
    if (ret < 0) {
        fprintf(stderr, "failed to chdir to '/'.\n");
        ret = errno;
        goto out;
    }

    for (fd = 0; fd < getdtablesize(); fd++)
        close(fd);

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed.\n");
        ret = errno;
        goto out;
    }

    if (pid > 0)
        exit(0);

    pidfp = hpssixd_setup_pidfile(daemon_data);
    if (!pidfp) {
        ret = errno;
        goto out;
    }

    ret = hpssixd_setup_signals();
    if (ret) {
        fprintf(stderr, "failed to setup signals.\n");
        ret = errno;
        goto out;
    }

    sprintf(daemon_name, "hpssixd:%s", daemon_data->name);
    openlog(daemon_name, LOG_CONS | LOG_PID, LOG_USER);

out:
    return ret;
}

int hpssixd_daemonize(hpssixd_daemon_data_t *data, int systemd)
{
    if (!data)
        return EINVAL;

    daemon_data = data;

    if (systemd)
        return daemonize_systemd();
    else {
        use_syslog = 1;

        return daemonize_sysv();
    }
}

void hpssixd_daemon_close(void)
{
    if (use_syslog)
        closelog();
    else {
        /* TODO: here. */
    }
}

