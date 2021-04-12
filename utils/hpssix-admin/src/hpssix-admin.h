/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_ADMIN_H
#define __HPSSIX_ADMIN_H

#include <config.h>

#include <errno.h>
#include <getopt.h>
#include <hpssix.h>

extern char *hpssix_admin_program_name;

typedef int (*hpssix_admin_cmdfunc_t)(int, char **);

struct _hpssix_admin_cmd {
    const char *name;
    const char *usage_str;
    hpssix_admin_cmdfunc_t func;
};

typedef struct _hpssix_admin_cmd hpssix_admin_cmd_t;

extern hpssix_admin_cmd_t hpssix_admin_cmd_history;

#endif /* __HPSSIX_ADMIN_H */
