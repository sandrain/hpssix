/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#ifndef __HPSSIX_SEARCH_H
#define __HPSSIX_SEARCH_H

#include <config.h>

#include <stdint.h>
#include <string.h>
#include <hpssix.h>

#define COND_ARG_NONE        (-1)
#define COND_ARG_INODE       0
#define COND_ARG_MODE        1
#define COND_ARG_UID         2
#define COND_ARG_GID         3
#define COND_ARG_SIZE        4
#define COND_ARG_ATIME       5
#define COND_ARG_MTIME       6
#define COND_ARG_CTIME       7
#define N_COND_ARG           8

enum {
    HPSSIX_OP_EQ = 0,
    HPSSIX_OP_GT = 1,
    HPSSIX_OP_GE = 2,
    HPSSIX_OP_LT = 3,
    HPSSIX_OP_LE = 4,
    HPSSIX_OP_NE = 5,
    HPSSIX_OP_BETWEEN = 6,
    HPSSIX_OP_BITAND = 7,
};

extern const char *opstr[];

struct _hpssix_search_fattr_cond {
    int attr;
    int op;
    uint64_t val1;
    uint64_t val2;
};

typedef struct _hpssix_search_fattr_cond hpssix_search_fattr_cond_t;

struct _hpssix_search_fattr {
    char *expression;
    FILE *output;
    int verbose;        /* show the sql statement */
    int countonly;
    int removed;

    char *path;
    char *name;

    hpssix_search_fattr_cond_t cond[N_COND_ARG];
};

typedef struct _hpssix_search_fattr hpssix_search_fattr_t;

static inline void hpssix_search_fattr_init(hpssix_search_fattr_t *search)
{
    if (search) {
        int i = 0;

        memset((void *) search, 0, sizeof(search));

        for (i = 0; i < N_COND_ARG; i++)
            search->cond[i].attr = COND_ARG_NONE;
    }
}

/**
 * @brief
 *
 * @param search
 * @param cond
 *
 * @return
 */
static inline int hpssix_search_fattr_add_cond(hpssix_search_fattr_t *search,
                                               hpssix_search_fattr_cond_t *cond)
{
    if (!search || !cond)
        return EINVAL;

    search->cond[cond->attr] = *cond;

    return 0;
}

char *hpssix_search_fattr_get_sql(hpssix_search_fattr_t *search);

struct _hpssix_search_tag_cond {
    char tag_name[1024];
    char *tag_val;

    int op;
    double val1;
    double val2;
};

typedef struct _hpssix_search_tag_cond hpssix_search_tag_cond_t;

struct _hpssix_search_tag {
    uint32_t count;
    hpssix_search_tag_cond_t *conds;
};

typedef struct _hpssix_search_tag hpssix_search_tag_t;

char *hpssix_search_tag_get_sql(hpssix_search_tag_t *search);

struct _hpssix_search_tsv {
    const char *keyword;
    uint64_t limit;
    FILE *output;

    int verbose;
    int meta;
    int text;
};

typedef struct _hpssix_search_tsv hpssix_search_tsv_t;

char *hpssix_search_tsv_get_sql(hpssix_search_tsv_t *search);

#endif /* __HPSSIX_SEARCH_H */

