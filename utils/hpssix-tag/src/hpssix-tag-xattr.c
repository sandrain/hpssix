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
#include <stdint.h>
#include <sys/types.h>
#include <utime.h>
#include <attr/xattr.h>
#include <getopt.h>
#include <hpssix.h>

#include "hpssix-tag.h"

static inline int tag_valid_path(const char *filename)
{
    char *hpssfs_mnt = hpssix_config->hpss_mountpoint;

    return 0 == strncmp(hpssfs_mnt, filename, strlen(hpssfs_mnt));
}

static inline void tag_print_value(char *value, size_t len)
{
    printf("%s\n", value);
}

static inline void tag_print_list(char *list, size_t len, int print_all)
{
    char *name = NULL;
    char *pos = NULL;
    size_t processed = 0;
    char xname[64] = { 0, };
    size_t namelen = 0;

    name = pos = list;

    sprintf(xname, "user.%s", hpssix_config->tag_prefix);
    namelen = strlen(xname);

    while (name[0] != '\0') {
        while (*pos != '\0')
            pos++;

        if (print_all)
            printf("%s\n", name);
        else if (0 == strncmp(xname, name, namelen))
            printf("%s\n", &name[namelen+1]);

        processed += strlen(name);
        name = ++pos;

        if (processed >= len)
            break;
    }
}

#define HPSSIX_TAG_CMD_SET_USAGE \
    "set [options] ... <filename>\n"                              \
    " available options:\n"                                       \
    " -l, --dereference          follow links\n"                  \
    " -n <name>, --name=<name>   set <name> as name of the tag\n" \
    " -v <val>, --value=<val>    set <val> as value of the tag\n" \
    "\n"

static const char *set_short_opts = "hln:v:";

static struct option const set_long_opts[] = {
    { "help", 0, 0, 'h' },
    { "dereference", 0, 0, 'l' },
    { "name", 1, 0, 'n' },
    { "value", 1, 0, 'v' },
    { 0, 0, 0, 0 },
};

static void set_usage(void)
{
    printf("%s %s", hpssix_tag_program_name, HPSSIX_TAG_CMD_SET_USAGE);
}

int hpssix_tag_cmd_set_func(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int optidx = 0;
    int dereference = 0;
    char *name = NULL;
    char xname[64] = { 0, };
    char *value = NULL;
    char *filename = NULL;

    while ((ch = getopt_long(argc, argv,
                             set_short_opts, set_long_opts, &optidx)) > 0) {
        switch (ch) {
        case 'l':
            dereference = 0;
            break;

        case 'n':
            name = strdup(optarg);
            break;

        case 'v':
            value = strdup(optarg);
            break;

        case 'h':
        default:
            set_usage();
            goto out;
        }
    }

    if (argc - optind != 1) {
        set_usage();
        goto out;
    }

    filename = realpath(argv[optind], NULL);
    if (!filename) {
        perror("cannot resolve the path");
        goto out;
    }

    if (!tag_valid_path(filename)) {
        fprintf(stderr, "%s does not belong to hpssfs.\n", filename);
        goto out_free;
    }

    if (!name || !value) {
        fprintf(stderr, "tag name and value should be given.\n");
        set_usage();
        goto out_free;
    }

    sprintf(xname, "user.%s.%s", hpssix_config->tag_prefix, name);

    if (dereference)
        ret = setxattr(filename, xname, (void *) value, strlen(value), 0);
    else
        ret = lsetxattr(filename, xname, (void *) value, strlen(value), 0);

    if (ret)
        perror("failed to set the tag");

    ret = utime(filename, NULL);

out_free:
    free(filename);
out:
    return ret;
}

#define HPSSIX_TAG_CMD_GET_USAGE \
    "get [options] ... <filename>\n"                              \
    " available options:\n"                                       \
    " -l, --dereference          follow links\n"                  \
    " -n <name>, --name=<name>   set <name> as name of the tag\n" \
    "\n"

static const char *get_short_opts = "hln:";

static struct option const get_long_opts[] = {
    { "help", 0, 0, 'h' },
    { "dereference", 0, 0, 'l' },
    { "name", 1, 0, 'n' },
    { 0, 0, 0, 0 },
};

static void get_usage(void)
{
    printf("%s %s", hpssix_tag_program_name, HPSSIX_TAG_CMD_GET_USAGE);
}

int hpssix_tag_cmd_get_func(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int optidx = 0;
    int dereference = 0;
    char *name = NULL;
    char xname[64] = { 0, };
    char *value = NULL;
    ssize_t len = 0;
    char *filename = NULL;

    while ((ch = getopt_long(argc, argv,
                             get_short_opts, get_long_opts, &optidx)) > 0) {
        switch (ch) {
        case 'l':
            dereference = 0;
            break;

        case 'n':
            name = strdup(optarg);
            break;

        case 'h':
        default:
            get_usage();
            goto out;
        }
    }

    if (argc - optind != 1) {
        get_usage();
        goto out;
    }

    filename = realpath(argv[optind], NULL);
    if (!filename) {
        perror("cannot resolve the path");
        goto out;
    }

    if (!tag_valid_path(filename)) {
        fprintf(stderr, "%s does not belong to hpssfs.\n", filename);
        goto out_free;
    }

    if (!name) {
        fprintf(stderr, "tag name should be given.\n");
        get_usage();
        goto out_free;
    }

    sprintf(xname, "user.%s.%s", hpssix_config->tag_prefix, name);

    if (dereference) {
        len = getxattr(filename, xname, NULL, 0);
        if (len < 0) {
            perror("cannot retrieve the tag");
            goto out_free;
        }

        value = calloc(1, len+1);
        if (!value) {
            perror("cannot allocate memory");
            goto out_free;
        }

        len = getxattr(filename, xname, (void *) value, len);
    }
    else {
        len = lgetxattr(filename, xname, NULL, 0);
        if (len < 0) {
            perror("cannot retrieve the tag");
            goto out_free;
        }

        value = calloc(1, len+1);
        if (!value) {
            perror("cannot allocate memory");
            goto out_free;
        }

        len = lgetxattr(filename, xname, (void *) value, len);
    }

    if (len < 0)
        perror("failed to read the tag");
    else
        tag_print_value(value, len);

    free(value);

out_free:
    free(filename);
out:
    return ret;
}

#define HPSSIX_TAG_CMD_DEL_USAGE \
    "del [options] ... <filename>\n"                              \
    " available options:\n"                                       \
    " -l, --dereference          follow links\n"                  \
    " -n <name>, --name=<name>   set <name> as name of the tag\n" \
    "\n"

static const char *del_short_opts = "hln:";

static struct option const del_long_opts[] = {
    { "help", 0, 0, 'h' },
    { "dereference", 0, 0, 'l' },
    { "name", 1, 0, 'n' },
    { 0, 0, 0, 0 },
};

static void del_usage(void)
{
    printf("%s %s", hpssix_tag_program_name, HPSSIX_TAG_CMD_DEL_USAGE);
}

int hpssix_tag_cmd_del_func(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int optidx = 0;
    int dereference = 0;
    char *name = NULL;
    char xname[64] = { 0, };
    char *filename = NULL;

    while ((ch = getopt_long(argc, argv,
                             del_short_opts, del_long_opts, &optidx)) > 0) {
        switch (ch) {
        case 'l':
            dereference = 0;
            break;

        case 'n':
            name = strdup(optarg);
            break;

        case 'h':
        default:
            del_usage();
            goto out;
        }
    }

    if (argc - optind != 1) {
        del_usage();
        goto out;
    }

    filename = realpath(argv[optind], NULL);
    if (!filename) {
        perror("cannot resolve the path");
        goto out;
    }

    if (!tag_valid_path(filename)) {
        fprintf(stderr, "%s does not belong to hpssfs.\n", filename);
        goto out_free;
    }

    if (!name) {
        fprintf(stderr, "tag name should be given.\n");
        del_usage();
        goto out_free;
    }

    sprintf(xname, "user.%s.%s", hpssix_config->tag_prefix, name);

    if (dereference)
        ret = removexattr(filename, xname);
    else
        ret = lremovexattr(filename, xname);

    if (ret < 0)
        perror("failed to remove the tag");

out_free:
    free(filename);
out:
    return ret;
}


#define HPSSIX_TAG_CMD_LIST_USAGE \
    "list [options] ... <filename>\n"                             \
    " available options:\n"                                       \
    " -a, --all                  print all attributes (xattr)\n"  \
    " -h, --help                 show help message\n"             \
    " -l, --dereference          follow links\n"                  \
    "\n"

static const char *list_short_opts = "ahl";

static struct option const list_long_opts[] = {
    { "all", 0, 0, 'a' },
    { "help", 0, 0, 'h' },
    { "dereference", 0, 0, 'l' },
    { 0, 0, 0, 0 },
};

static void list_usage(void)
{
    printf("%s %s", hpssix_tag_program_name, HPSSIX_TAG_CMD_LIST_USAGE);
}


int hpssix_tag_cmd_list_func(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int optidx = 0;
    int print_all = 0;
    int dereference = 0;
    char *list = NULL;
    ssize_t len = 0;
    char *filename = NULL;

    while ((ch = getopt_long(argc, argv,
                             list_short_opts, list_long_opts, &optidx)) > 0) {
        switch (ch) {
        case 'a':
            print_all = 1;
            break;

        case 'l':
            dereference = 0;
            break;

        case 'h':
        default:
            list_usage();
            goto out;
        }
    }

    if (argc - optind != 1) {
        list_usage();
        goto out;
    }

    filename = realpath(argv[optind], NULL);
    if (!filename) {
        perror("cannot resolve the path");
        goto out;
    }

    if (!tag_valid_path(filename)) {
        fprintf(stderr, "%s does not belong to hpssfs.\n", filename);
        goto out_free;
    }

    if (dereference) {
        len = listxattr(filename, NULL, 0);
        if (len < 0) {
            perror("cannot get the list of tags");
            goto out_free;
        }

        list = calloc(1, len+1);
        if (!list) {
            perror("cannot allocate memory");
            goto out_free;
        }

        len = listxattr(filename, list, len);
    }
    else {
        len = llistxattr(filename, NULL, 0);
        if (len < 0) {
            perror("cannot get the list of tags");
            goto out_free;
        }

        list = calloc(1, len+1);
        if (!list) {
            perror("cannot allocate memory");
            goto out_free;
        }

        len = llistxattr(filename, list, len);
    }

    if (len < 0)
        perror("failed to read the tag");
    else
        tag_print_list(list, len, print_all);

    free(list);

out_free:
    free(filename);
out:
    return ret;
}

hpssix_tag_cmd_t hpssix_tag_cmd_set = {
    .name = "set",
    .usage_str = HPSSIX_TAG_CMD_SET_USAGE,
    .func = hpssix_tag_cmd_set_func,
};

hpssix_tag_cmd_t hpssix_tag_cmd_get = {
    .name = "get",
    .usage_str = HPSSIX_TAG_CMD_GET_USAGE,
    .func = hpssix_tag_cmd_get_func,
};

hpssix_tag_cmd_t hpssix_tag_cmd_del = {
    .name = "del",
    .usage_str = HPSSIX_TAG_CMD_DEL_USAGE,
    .func = hpssix_tag_cmd_del_func,
};

hpssix_tag_cmd_t hpssix_tag_cmd_list = {
    .name = "list",
    .usage_str = HPSSIX_TAG_CMD_LIST_USAGE,
    .func = hpssix_tag_cmd_list_func,
};

