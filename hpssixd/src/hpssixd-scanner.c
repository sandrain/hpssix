/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#include <config.h>

#include "hpssixd.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <rpc/pmap_clnt.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <hpssix.h>

/*
 * TODO: register inodify watch list for configuration file modifications.
 */
static hpssixd_daemon_data_t *scanner_data;
static hpssix_mdb_t _mdb;

static
const char *db2_new_insert_script = LIBEXECDIR "/hpssix-db2-scan.sh";

static pthread_t control_thread;
static volatile int terminate;

struct timeval rpc_timeout;

static int builder_running;
static int extractor_running;

int *scanner_connect_1_svc(int *argp, struct svc_req *rqstp)
{
    static int  result = 0;
    int sender = *argp;

    /*
     * insert server code here
     */
    switch (sender) {
        case HPSSIXD_ID_BUILDER:
            builder_running = 1;
            break;
        case HPSSIXD_ID_EXTRACTOR:
            extractor_running = 1;
            break;
        default:
            break;
    }

    hpssixd_log_info("connection request: "
                     "builder_running = %d, extractor_running = %d",
                     builder_running, extractor_running);

    return &result;
}

static void
hpssixd_scanner_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
    union {
        int scanner_connect_1_arg;
    } argument;
    char *result;
    xdrproc_t _xdr_argument, _xdr_result;
    char *(*local)(char *, struct svc_req *);

    switch (rqstp->rq_proc) {
        case NULLPROC:
            (void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
            return;

        case scanner_connect:
            _xdr_argument = (xdrproc_t) xdr_int;
            _xdr_result = (xdrproc_t) xdr_int;
            local = (char *(*)(char *, struct svc_req *)) scanner_connect_1_svc;
            break;

        default:
            svcerr_noproc (transp);
            return;
    }
    memset ((char *)&argument, 0, sizeof (argument));
    if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
        svcerr_decode (transp);
        return;
    }
    result = (*local)((char *)&argument, rqstp);
    if (result != NULL
        && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result))
        svcerr_systemerr (transp);

    if (!svc_freeargs(transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
        hpssixd_log_err("unable to free RPC arguments");
        exit(1);
    }
    return;
}

static void *scanner_control(void *data)
{
    int ret = 0;
    register SVCXPRT *transp;

    pmap_unset (HPSSIXD_SCANNER, SCANNER_VER);

    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL) {
        hpssixd_log_err("cannot create udp service.");
        exit(1);
    }
    if (!svc_register(transp, HPSSIXD_SCANNER, SCANNER_VER,
                      hpssixd_scanner_1, IPPROTO_UDP)) {
        hpssixd_log_err("unable to register scanner udp service.");
        exit(1);
    }

    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        hpssixd_log_err("cannot create tcp service.");
        exit(1);
    }
    if (!svc_register(transp, HPSSIXD_SCANNER, SCANNER_VER,
                      hpssixd_scanner_1, IPPROTO_TCP)) {
        hpssixd_log_err("unable to register scanner tcp service.");
        exit(1);
    }

    hpssixd_log_info("scanner rpcpservice registered..");

    svc_run();

    hpssixd_log_info("RPC svc_run returned");

out:
    return 0;
}

static int rpc_run_worker(int worker, uint64_t task, int **result)
{
    int ret = 0;
    CLIENT *clnt = NULL;
    char *host = NULL;
    uint64_t task_id = task;

    if (worker == HPSSIXD_ID_BUILDER) {
        host = scanner_data->config.builder_host;
        clnt = clnt_create(host, HPSSIXD_BUILDER, BUILDER_VER, "tcp");
    }
    else if (worker == HPSSIXD_ID_EXTRACTOR) {
        host = scanner_data->config.extractor_host;
        clnt = clnt_create(host, HPSSIXD_EXTRACTOR, EXTRACTOR_VER, "tcp");
    }
    else
        return EINVAL;

    if (clnt == NULL) {
        clnt_pcreateerror(host);
        ret = errno;
        goto out;
    }

    clnt_control(clnt, CLSET_TIMEOUT, (char *) &rpc_timeout);

    if (worker == HPSSIXD_ID_BUILDER)
        *result = builder_run_1(&task_id, clnt);
    else if (worker == HPSSIXD_ID_EXTRACTOR)
        *result = extractor_run_1(&task_id, clnt);
    else {
        ret = EINVAL;
        goto out;
    }

    if (*result == (int *) NULL)
        clnt_perror (clnt, "call failed");

out:
    clnt_destroy (clnt);

    return ret;
}

static int rpc_terminate_worker(int worker, int **result)
{
    int ret = 0;
    CLIENT *clnt = NULL;
    char *host = NULL;
    struct timeval timeout = { 0, };

    if (worker == HPSSIXD_ID_BUILDER) {
        host = scanner_data->config.builder_host;
        clnt = clnt_create(host, HPSSIXD_BUILDER, BUILDER_VER, "tcp");
    }
    else if (worker == HPSSIXD_ID_EXTRACTOR) {
        host = scanner_data->config.extractor_host;
        clnt = clnt_create(host, HPSSIXD_EXTRACTOR, EXTRACTOR_VER, "tcp");
    }
    else
        return EINVAL;

    if (clnt == NULL) {
        clnt_pcreateerror(host);
        ret = errno;
        goto out;
    }

    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    clnt_control(clnt, CLSET_TIMEOUT, (char *) &timeout);

    if (worker == HPSSIXD_ID_BUILDER)
        *result = builder_terminate_1(NULL, clnt);
    else if (worker == HPSSIXD_ID_EXTRACTOR)
        *result = extractor_terminate_1(NULL, clnt);
    else {
        ret = EINVAL;
        goto out;
    }

    if (*result == (int *) NULL)
        clnt_perror (clnt, "call failed");

out:
    clnt_destroy (clnt);

    return ret;
}

static int scanner_terminate_all(void)
{
    int ret = 0;
    int *result = NULL;

    if (builder_running) {
        ret = rpc_terminate_worker(HPSSIXD_ID_BUILDER, &result);
        if (ret)
            hpssixd_log_err("terminating builder faild..");
    }

    if (extractor_running) {
        ret = rpc_terminate_worker(HPSSIXD_ID_EXTRACTOR, &result);
        if (ret)
            hpssixd_log_err("terminating extractor faild..");
    }

    hpssixd_daemon_close();

    return ret;
}

/*
 * TODO: use execve instead of popen()
 */
static int launch_scanner_process(const char *outdir,
                                  hpssix_work_status_t *status)
{
    int ret = 0;
    FILE *pout = NULL;
    char datebuf[11] = { 0, };  /* YYYY-MM-DD */
    char buf[LINE_MAX] = { 0, };
    uint64_t last_oid = 0;
    uint64_t ts = 0;
    hpssix_config_t *config = &scanner_data->config;

    ret = hpssix_mdb_get_last_scanned_status(&_mdb, &last_oid, &ts);
    if (ret)
        goto out;

    /*
     * if this is the first scan (last_oid == 0 && ts == 0),
     * read the config file to get the first oid
     */
    if (0 == last_oid) {
        last_oid = config->first_oid;
        ts = time(NULL);
    }

    /* scanning from the previous day for deleted/updated files. */
    ts -= 60*60*24;
    strftime(datebuf, sizeof(datebuf), "%F", localtime(&ts));

    /*
     * script <db2> <user> <passwd> <last oid> <fromdate> <outdir> <ts>
     */
    sprintf(buf, "%s %s/bin/db2 %s %s %lu %s %s %lu",
                 db2_new_insert_script,
                 config->db2path,
                 config->db2user,
                 config->db2password,
                 last_oid,
                 datebuf,
                 outdir,
                 status->id);

    hpssixd_log_info("launching scanner.. (last scanned oid=%lu, output=%s)",
                     last_oid, outdir);

    pout = popen(buf, "r");
    if (!pout) {
        ret = errno ? errno : ENOMEM;
        goto out;
    }

    while (fgets(buf, LINE_MAX-1, pout) != NULL) {
        fputs(buf, stdout);

        if (buf[0] == '#' && buf[1] == '#')
            hpssixd_log_info("%s", buf);

        catch_line_output(buf, "## start oid", &status->oid_start);
        catch_line_output(buf, "## end oid", &status->oid_end);
        catch_line_output(buf, "## scanned", &status->n_scanned);
        catch_line_output(buf, "## deleted", &status->n_deleted);
    }
    if (ferror(pout))
        hpssixd_log_info("failed to read output from scanner process");

    ret = pclose(pout);
    if (ret)
        hpssixd_log_info("scanner process failed (ret=%d)..", ret);
out:
    return ret;
}

static int do_scanner(void)
{
    int ret = 0;
    int *result = NULL;
    uint64_t task_id = 0;
    uint64_t elapsed = 0;
    char outdir[PATH_MAX] = { 0, };
    hpssix_mdb_t *mdb = &_mdb;
    hpssix_work_status_t status = { 0, };

    task_id = (uint64_t) time(NULL);

    hpssixd_log_info("launching the scanner, task = %lu", task_id);

    if (!builder_running) {
        hpssixd_log_err("builder is not running, aborting task.");
        return EINVAL;
    }

    if (!extractor_running) {
        hpssixd_log_err("extractor is not running, aborting task.");
        return EINVAL;
    }

    memset((void *) &status, 0, sizeof(status));

    status.id = task_id;

    /* open the master db to work with */
    ret = hpssix_mdb_open(mdb);
    if (ret) {
        hpssixd_log_err("failed to open the master db");
        return EIO;
    }

    /* .. run the scanner here .. */
    ret = hpssix_get_datadir_today(outdir, PATH_MAX);
    if (ret) {
        hpssixd_log_err("failed to prepare the workdir");
        goto out;
    }


    status.scanner_start = time(NULL);

    ret = launch_scanner_process(outdir, &status);
    if (ret) {
        hpssixd_log_err("failed to run the scanner.");
        goto out;
    }

    status.builder_start = time(NULL);

    if (status.n_scanned + status.n_deleted == 0) {
        hpssixd_log_info("no records scanned, terminating..");
        goto out;
    }

    hpssixd_log_info("%lu files newly scanned, %lu deleted files..",
                     status.n_scanned, status.n_deleted);

    /* run builder */
    hpssixd_log_info("sending request to builder..");

    ret = rpc_run_worker(HPSSIXD_ID_BUILDER, status.id, &result);
    if (ret) {
        hpssixd_log_err("builder failed, aborting task.");
        goto out;
    }

    status.extractor_start = time(NULL);
    elapsed = status.extractor_start - status.scanner_start;

    hpssixd_log_info("builder finished: %d records in %lu seconds.",
                     *result, elapsed);

    status.n_indexed = *result;

    /* run extractor */
    hpssixd_log_info("sending request to extractor..");

    ret = rpc_run_worker(HPSSIXD_ID_EXTRACTOR, status.id, &result);
    if (ret) {
        hpssixd_log_err("extractor failed, aborting the job.");
        goto out;
    }

    status.extractor_end = time(NULL);
    elapsed = status.extractor_end - status.extractor_start;

    hpssixd_log_info("extractor finished: %d records in %lu seconds.",
                     *result, elapsed);

    status.n_extracted = *result;
    status.status = 0;

    ret = hpssix_mdb_record(mdb, &status);
    if (ret)
        hpssixd_log_err("faild to record to the master db.");

    hpssixd_log_info("all finished, %lu files indexed and "
                     "extracted contents from %lu files",
                     status.n_indexed, status.n_extracted);

out:
    hpssix_mdb_close(mdb);

    return ret;
}

static unsigned int register_alarm(void)
{
    uint64_t sleep_duration_sec = 0;
    struct tm run_at = { 0, };
    struct tm now = { 0, };
    time_t time_now = time(NULL);
    time_t time_run_at = 0;

    memset((void *) &run_at, 0, sizeof(run_at));
    run_at = scanner_data->config.schedule;

    localtime_r(&time_now, &now);

    run_at.tm_year = now.tm_year;
    run_at.tm_mon = now.tm_mon;
    run_at.tm_mday = now.tm_mday;

    time_run_at = mktime(&run_at);
    if (time_run_at < time_now)
        time_run_at += 24*60*60;

    sleep_duration_sec = time_run_at - time_now;

    return alarm(sleep_duration_sec);
}

int hpssixd_daemon_scanner(hpssixd_daemon_data_t *data)
{
    int ret = 0;
    int signum = 0;
    sigset_t set = { 0, };
    hpssix_mdb_t *mdb = &_mdb;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);

    ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (ret) {
        hpssixd_log_err("failed to initialize signals");
        goto out;
    }

    scanner_data = data;
    rpc_timeout.tv_sec = scanner_data->config.rpc_timeout;

    ret = pthread_create(&control_thread, NULL, scanner_control,
                         (void *) &sigset);
    if (ret) {
        hpssixd_log_err("failed to spawn control thread");
        goto out;
    }

    while (1) {
        register_alarm();

        ret = sigwait(&set, &signum);
        if (ret)
            hpssixd_log_err("failed to fetch signal");

        hpssixd_log_info("received signal: %s (%d)", strsignal(signum), signum);

        switch (signum) {
        case SIGALRM:
        case SIGUSR1:
            do_scanner();
            break;

        case SIGUSR2:
            break;

        case SIGHUP:
            break;

        case SIGTERM:
            scanner_terminate_all();
            goto quit;

        default:
            break;
        }
    }

quit:
    pthread_kill(control_thread, SIGKILL);
    pthread_join(control_thread, NULL);
out:
    return ret;
}

