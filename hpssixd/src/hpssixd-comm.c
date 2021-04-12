#include <memory.h> /* for memset */
#include "hpssixd-comm.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

int *scanner_connect_1(int *argp, CLIENT *clnt)
{
    static int clnt_res;

    memset((char *)&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call (clnt, scanner_connect,
                   (xdrproc_t) xdr_int, (caddr_t) argp,
                   (xdrproc_t) xdr_int, (caddr_t) &clnt_res,
                   TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&clnt_res);
}

int *builder_run_1(u_quad_t *argp, CLIENT *clnt)
{
    static int clnt_res;

    memset((char *)&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call (clnt, builder_run,
                   (xdrproc_t) xdr_u_quad_t, (caddr_t) argp,
                   (xdrproc_t) xdr_int, (caddr_t) &clnt_res,
                   TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&clnt_res);
}

int *builder_alive_1(void *argp, CLIENT *clnt)
{
    static int clnt_res;

    memset((char *)&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call (clnt, builder_alive,
                (xdrproc_t) xdr_void, (caddr_t) argp,
                (xdrproc_t) xdr_int, (caddr_t) &clnt_res,
                TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&clnt_res);
}

int *builder_terminate_1(void *argp, CLIENT *clnt)
{
    static int clnt_res;

    memset((char *)&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call (clnt, builder_terminate,
                   (xdrproc_t) xdr_void, (caddr_t) argp,
                   (xdrproc_t) xdr_int, (caddr_t) &clnt_res,
                   TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&clnt_res);
}

int *extractor_run_1(u_quad_t *argp, CLIENT *clnt)
{
    static int clnt_res;

    memset((char *)&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call (clnt, extractor_run,
                   (xdrproc_t) xdr_u_quad_t, (caddr_t) argp,
                   (xdrproc_t) xdr_int, (caddr_t) &clnt_res,
                   TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&clnt_res);
}

int *extractor_alive_1(void *argp, CLIENT *clnt)
{
    static int clnt_res;

    memset((char *)&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call (clnt, extractor_alive,
                   (xdrproc_t) xdr_void, (caddr_t) argp,
                   (xdrproc_t) xdr_int, (caddr_t) &clnt_res,
                   TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&clnt_res);
}

int *extractor_terminate_1(void *argp, CLIENT *clnt)
{
    static int clnt_res;

    memset((char *)&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call (clnt, extractor_terminate,
                   (xdrproc_t) xdr_void, (caddr_t) argp,
                   (xdrproc_t) xdr_int, (caddr_t) &clnt_res,
                   TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&clnt_res);
}
