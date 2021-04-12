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

static hpssixd_daemon_data_t *extractor_data;

static const char *extractor_exe = LIBEXECDIR "/hpssix-extractor";

static pthread_t control_thread;
static uint64_t current_task;

static int do_extractor(uint64_t task_id, int *result)
{
    int ret = 0;
    char *pos = NULL;
    FILE *pout = NULL;
    uint64_t n_extracted = 0;
    char cmd[LINE_MAX] = { 0, };
    char builder_output[PATH_MAX] = { 0, };
    hpssix_config_t *config = &extractor_data->config;

    ret = hpssix_get_datadir_today(builder_output, PATH_MAX);
    if (ret)
        goto out;

    pos = &builder_output[strlen(builder_output)];
    sprintf(pos, "/builder.%lu.db", task_id);

    sprintf(cmd, "%s --nthreads=%lu %s",
            extractor_exe, config->extractor_nthreads, builder_output);

    hpssixd_log_info("launching extractor.. (cmd: %s)", cmd);

    pout = popen(cmd, "r");
    if (!pout) {
        ret = errno ? errno : ENOMEM;
        goto out;
    }

    while (fgets(cmd, LINE_MAX-1, pout) != NULL) {
        fputs(cmd, stdout);

        catch_line_output(cmd, "## files extracted", &n_extracted);
    }
    if (ferror(pout))
        perror("fgets");
    else
        *result = (int) n_extracted;

    hpssixd_log_info("extracted contents from %lu files", n_extracted);

    pclose(pout);
out:
    return ret;
}

static void extractor_reload(void)
{
    hpssixd_log_info("reloading extractor..");
}

static void extractor_cleanup(void)
{
    hpssixd_log_info("terminating extractor..");

    pthread_kill(control_thread, SIGTERM);
}

int *extractor_run_1_svc(u_quad_t *argp, struct svc_req *rqstp)
{
    int ret = 0;
    static int result = 0;
    uint64_t task_id = *((uint64_t *) argp);

    hpssixd_log_info("received the task request: id = %lu", task_id);

    if (task_id == current_task) {
        hpssixd_log_warning("same task is already running, "
                            "thus ignoring the request");
    }
    else {
        current_task = task_id;

        /* run builder here .. */
        ret = do_extractor(task_id, &result);
        if (ret)
            hpssixd_log_err("running extractor failed.");
    }

    return &result;
}

int *extractor_alive_1_svc(void *argp, struct svc_req *rqstp)
{
    static int result = 0;

    hpssixd_log_info("received alive ping..");

    return &result;
}

int *extractor_terminate_1_svc(void *argp, struct svc_req *rqstp)
{
    static int result = 0;

    hpssixd_log_info("received terminate request..");

    extractor_cleanup();

    return &result;
}

static void
hpssixd_extractor_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
    union {
        u_quad_t extractor_run_1_arg;
    } argument;
    char *result;
    xdrproc_t _xdr_argument, _xdr_result;
    char *(*local)(char *, struct svc_req *);

    switch (rqstp->rq_proc) {
        case NULLPROC:
            (void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
            return;

        case extractor_run:
            _xdr_argument = (xdrproc_t) xdr_u_quad_t;
            _xdr_result = (xdrproc_t) xdr_int;
            local = (char *(*)(char *, struct svc_req *)) extractor_run_1_svc;
            break;

        case extractor_alive:
            _xdr_argument = (xdrproc_t) xdr_void;
            _xdr_result = (xdrproc_t) xdr_int;
            local = (char *(*)(char *, struct svc_req *)) extractor_alive_1_svc;
            break;

        case extractor_terminate:
            _xdr_argument = (xdrproc_t) xdr_void;
            _xdr_result = (xdrproc_t) xdr_int;
            local = (char *(*)(char *, struct svc_req *))
                    extractor_terminate_1_svc;
            break;

        default:
            svcerr_noproc (transp);
            return;
    }
    memset ((char *)&argument, 0, sizeof (argument));
    if (!svc_getargs(transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
        svcerr_decode (transp);
        return;
    }
    result = (*local)((char *)&argument, rqstp);
    if (result != NULL
        && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
        svcerr_systemerr (transp);
    }
    if (!svc_freeargs(transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
        hpssixd_log_err("unable to free arguments");
        exit (1);
    }
    return;
}

static void *extractor_control(void *data)
{
    register SVCXPRT *transp;

    pmap_unset (HPSSIXD_EXTRACTOR, EXTRACTOR_VER);

    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL) {
        hpssixd_log_err("cannot create udp service.");
        exit(1);
    }
    if (!svc_register(transp, HPSSIXD_EXTRACTOR, EXTRACTOR_VER,
                      hpssixd_extractor_1, IPPROTO_UDP)) {
        hpssixd_log_err("unable to register extractor udp service.");
        exit(1);
    }

    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        hpssixd_log_err("cannot create tcp service.");
        exit(1);
    }
    if (!svc_register(transp, HPSSIXD_EXTRACTOR, EXTRACTOR_VER,
                      hpssixd_extractor_1, IPPROTO_TCP)) {
        hpssixd_log_err("unable to register extractor tcp service.");
        exit(1);
    }

    hpssixd_log_info("extractor rpcpservice registered..");

    svc_run();

    hpssixd_log_info("RPC svc_run returned");

    return 0;
}

static int connect_scanner(char *host, int id)
{
    CLIENT *clnt;
    int  *result_1;
	int  arg = id;

    clnt = clnt_create(host, HPSSIXD_SCANNER, SCANNER_VER, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror (host);
        return -1;
    }

    result_1 = scanner_connect_1(&arg, clnt);
    if (result_1 == (int *) NULL)
        clnt_perror (clnt, "call failed");

    clnt_destroy (clnt);

    return *result_1 == HPSSIXD_RC_SUCCESS ? 0 : -1;
}

int hpssixd_daemon_extractor(hpssixd_daemon_data_t *data)
{
    int ret = 0;
    int signum = 0;
    sigset_t set = { 0, };

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGHUP);

    ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (ret) {
        hpssixd_log_err("failed to initialize signals");
        goto out;
    }

    extractor_data = data;

    ret = pthread_create(&control_thread, NULL, extractor_control,
                         (void *) &sigset);
    if (ret) {
        hpssixd_log_err("failed to spawn control thread.");
        goto out;
    }

    ret = connect_scanner(data->config.scanner_host, HPSSIXD_ID_EXTRACTOR);
    if (ret) {
        hpssixd_log_err("failed to connect to scanner.");
        goto out;
    }

    while (1) {
        ret = sigwait(&set, &signum);
        if (ret)
            hpssixd_log_err("failed to fetch signal");

        hpssixd_log_info("received signal: %s (%d)", strsignal(signum), signum);

        switch (signum) {
        case SIGHUP:
            extractor_reload();
            break;

        case SIGTERM:
            extractor_cleanup();
            goto quit;

        default:
            break;
        }
    }

quit:
    pthread_join(control_thread, NULL);
out:
    return ret;
}

