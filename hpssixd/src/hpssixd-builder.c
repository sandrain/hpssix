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
#include <sys/socket.h>

static hpssixd_daemon_data_t *builder_data;

static const char *builder_exe = LIBEXECDIR "/hpssix-builder";

static pthread_t control_thread;
static uint64_t current_task;

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int do_builder(uint64_t task_id, int *result)
{
    int ret = 0;
    char *pos = NULL;
    FILE *pout = NULL;
    uint64_t n_processed = 0;
    uint64_t n_regular = 0;
    char cmd[LINE_MAX] = { 0, };
    char datadir[PATH_MAX] = { 0, };

    ret = hpssix_get_datadir_today(datadir, PATH_MAX);
    if (ret)
        goto out;

    sprintf(cmd, "%s %s %lu", builder_exe, datadir, task_id);

    hpssixd_log_info("launching builder.. (cmd: %s)", cmd);

    pout = popen(cmd, "r");
    if (!pout) {
        ret = errno ? errno : ENOMEM;
        goto out;
    }

    while (fgets(cmd, LINE_MAX-1, pout) != NULL) {
        fputs(cmd, stdout);

        if (cmd[0] == '#' && cmd[1] == '#')
            hpssixd_log_info("%s", cmd);

        catch_line_output(cmd, "## files indexed", &n_processed);
        catch_line_output(cmd, "## regular files", &n_regular);
    }
    if (ferror(pout))
        perror("fgets");
    else
        *result = (int) n_processed;

    hpssixd_log_info("%lu files indexed (%lu regular files)",
                     n_processed, n_regular);

    pclose(pout);
out:
    return ret;
}

static void builder_reload(void)
{
    hpssixd_log_info("reloading builder..");
}

static void builder_cleanup(void)
{
    hpssixd_log_info("terminating builder..");

    pthread_kill(control_thread, SIGTERM);
}

int *builder_run_1_svc(u_quad_t *argp, struct svc_req *rqstp)
{
    int ret = 0;
    static int result = 0;
    uint64_t task_id = *((uint64_t *) argp);

    hpssixd_log_info("received the task request: id = %lu.", task_id);

    if (task_id == current_task) {
        hpssixd_log_warning("same task is already running "
                            "thus ignoring the request.");
    }
    else {
        current_task = task_id;

        /* run builder here .. */
        ret = do_builder(task_id, &result);
        if (ret)
            hpssixd_log_err("builder failed.");
    }

    return &result;
}

int *builder_alive_1_svc(void *argp, struct svc_req *rqstp)
{
    static int result = 0;

    hpssixd_log_info("received alive ping..\n");

    return &result;
}

int *builder_terminate_1_svc(void *argp, struct svc_req *rqstp)
{
    static int result = 0;

    hpssixd_log_info("received terminate request..\n");

    builder_cleanup();

    return &result;
}


static void
hpssixd_builder_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
    union {
        u_quad_t builder_run_1_arg;
    } argument;
    char *result;
    xdrproc_t _xdr_argument, _xdr_result;
    char *(*local)(char *, struct svc_req *);

    switch (rqstp->rq_proc) {
    case NULLPROC:
        (void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
        return;

    case builder_run:
        _xdr_argument = (xdrproc_t) xdr_u_quad_t;
        _xdr_result = (xdrproc_t) xdr_int;
        local = (char *(*)(char *, struct svc_req *)) builder_run_1_svc;
        break;

    case builder_alive:
        _xdr_argument = (xdrproc_t) xdr_void;
        _xdr_result = (xdrproc_t) xdr_int;
        local = (char *(*)(char *, struct svc_req *)) builder_alive_1_svc;
        break;

    case builder_terminate:
        _xdr_argument = (xdrproc_t) xdr_void;
        _xdr_result = (xdrproc_t) xdr_int;
        local = (char *(*)(char *, struct svc_req *)) builder_terminate_1_svc;
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
        hpssixd_log_err("unable to free RPC arguments");
        exit(1);
    }
    return;
}

static void *builder_control(void *data)
{
    register SVCXPRT *transp;

    pmap_unset(HPSSIXD_BUILDER, BUILDER_VER);

    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL) {
        hpssixd_log_err("cannot create udp service.");
        exit(1);
    }
    if (!svc_register(transp, HPSSIXD_BUILDER, BUILDER_VER,
                      hpssixd_builder_1, IPPROTO_UDP)) {
        hpssixd_log_err("unable to register builder udp service.");
        exit(1);
    }

    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        hpssixd_log_err("cannot create tcp service.");
        exit(1);
    }
    if (!svc_register(transp, HPSSIXD_BUILDER, BUILDER_VER,
                      hpssixd_builder_1, IPPROTO_TCP)) {
        hpssixd_log_err("unable to register builder tcp service.");
        exit(1);
    }

    hpssixd_log_info("builder rpcpservice registered..");

    svc_run();

    hpssixd_log_info("RPC svc_run returned");

    return 0;
}

static int connect_scanner(char *host, int id)
{
    CLIENT *clnt;
    int *result_1;
    int arg = id;

    clnt = clnt_create(host, HPSSIXD_SCANNER, SCANNER_VER, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        return -1;
    }

    result_1 = scanner_connect_1(&arg, clnt);
    if (result_1 == (int *) NULL)
        clnt_perror (clnt, "call failed");

    clnt_destroy (clnt);

    return *result_1 == HPSSIXD_RC_SUCCESS ? 0 : -1;
}

int hpssixd_daemon_builder(hpssixd_daemon_data_t *data)
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

    builder_data = data;

    ret = pthread_create(&control_thread, NULL, builder_control,
                         (void *) &sigset);
    if (ret) {
        hpssixd_log_err("failed to spawn control thread");
        goto out;
    }

    ret = connect_scanner(data->config.scanner_host, HPSSIXD_ID_BUILDER);
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
            builder_reload();
            break;

        case SIGTERM:
            builder_cleanup();
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

