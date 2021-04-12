/* Copyright (C) 2018 - Hyogi Sim <simh@ornl.gov>
 * 
 * Please refer to COPYING for the license.
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_TESTLIB_H
#define __HPSSIX_TESTLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

static inline void die(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vfprintf(stderr, format, args);

    va_end(args);

    fprintf(stderr, "ERR %d: %s\n", errno, strerror(errno));

    exit(-1);
}

#endif /* __HPSSIX_TESTLIB_H */
