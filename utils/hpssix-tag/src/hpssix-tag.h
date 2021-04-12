/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_TAG_H
#define __HPSSIX_TAG_H
#include <config.h>

#include <hpssix.h>

typedef int (*hpssix_tag_cmdfunc_t)(int, char **);

struct _hpssix_tag_cmd {
    const char *name;
    const char *usage_str;
    hpssix_tag_cmdfunc_t func;
};

typedef struct _hpssix_tag_cmd hpssix_tag_cmd_t;

extern char *hpssix_tag_program_name;

extern hpssix_config_t *hpssix_config;

extern hpssix_tag_cmd_t hpssix_tag_cmd_set;

extern hpssix_tag_cmd_t hpssix_tag_cmd_get;

extern hpssix_tag_cmd_t hpssix_tag_cmd_del;

extern hpssix_tag_cmd_t hpssix_tag_cmd_list;

#endif /* __HPSSIX_TAG_H */

