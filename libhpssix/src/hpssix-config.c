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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <libconfig.h>

#include "hpssix-config.h"

static const char *hpssix_sysconf_file = CONFDIR "/hpssix.conf";
static const char *hpssix_extfilter_file = CONFDIR "/extfilter.conf";

static void read_hpssix_schedule(hpssix_config_t *config, const char *str)
{
    struct tm *schedule = &config->schedule;

    sscanf(str, "%2d:%2d:%2d",
                &schedule->tm_hour, &schedule->tm_min, &schedule->tm_sec);
}

int hpssix_config_read_sysconf(hpssix_config_t *config)
{
    int ret = 0;
    config_t section = { 0, };
    config_setting_t *setting = NULL;
    const char *sval = NULL;
    int ival = 0;

    if (!config) {
        errno = EINVAL;
        return -1;
    }

    memset(config, 0, sizeof(*config));

    ret = config_read_file(&section, hpssix_sysconf_file);
    if (ret == CONFIG_TRUE) {
        /* reaad the global configuration */
        setting = config_lookup(&section, "hpssix");
        if (setting) {
            ret = config_setting_lookup_string(setting, "workdir", &sval);
            if (ret == CONFIG_TRUE)
                config->workdir = strdup(sval);

            ret = config_setting_lookup_string(setting, "schedule", &sval);
            if (ret == CONFIG_TRUE)
                read_hpssix_schedule(config, sval);

            ret = config_setting_lookup_int(setting, "rpctimeout", &ival);
            if (ret == CONFIG_TRUE)
                config->rpc_timeout = ival;

            ret = config_setting_lookup_int(setting, "firstoid", &ival);
            if (ret == CONFIG_TRUE)
                config->first_oid = ival;
        }

        /* tagging configuration */
        setting = config_lookup(&section, "tagging");
        if (setting) {
            ret = config_setting_lookup_string(setting, "prefix", &sval);
            if (ret == CONFIG_TRUE)
                config->tag_prefix = strdup(sval);
        }

        /* read the hpss configuration */
        setting = config_lookup(&section, "hpss");
        if (setting) {
            ret = config_setting_lookup_string(setting, "mountpoint", &sval);
            if (ret == CONFIG_TRUE)
                config->hpss_mountpoint = strdup(sval);

            ret = config_setting_lookup_string(setting, "db2path", &sval);
            if (ret == CONFIG_TRUE)
                config->db2path = strdup(sval);

            ret = config_setting_lookup_string(setting, "db2user", &sval);
            if (ret == CONFIG_TRUE)
                config->db2user = strdup(sval);

            ret = config_setting_lookup_string(setting, "db2password", &sval);
            if (ret == CONFIG_TRUE)
                config->db2password = strdup(sval);
        }

        /* read the database configuration */
        setting = config_lookup(&section, "database");
        if (setting) {
            ret = config_setting_lookup_string(setting, "host", &sval);
            if (ret == CONFIG_TRUE)
                config->db_host = strdup(sval);

            ret = config_setting_lookup_int(setting, "port", &ival);
            if (ret == CONFIG_TRUE)
                config->db_port = ival;

            ret = config_setting_lookup_string(setting, "user", &sval);
            if (ret == CONFIG_TRUE)
                config->db_user = strdup(sval);

            ret = config_setting_lookup_string(setting, "password", &sval);
            if (ret == CONFIG_TRUE)
                config->db_password = strdup(sval);

            ret = config_setting_lookup_string(setting, "database", &sval);
            if (ret == CONFIG_TRUE)
                config->db_database = strdup(sval);
        }

        /* read the tika configuration */
        setting = config_lookup(&section, "tika");
        if (setting) {
            ret = config_setting_lookup_string(setting, "host", &sval);
            if (ret == CONFIG_TRUE)
                config->tika_host = strdup(sval);

            ret = config_setting_lookup_int(setting, "port", &ival);
            if (ret == CONFIG_TRUE)
                config->tika_port = ival;
        }

        /* read the scanner configuration */
        setting = config_lookup(&section, "scanner");
        if (setting) {
            ret = config_setting_lookup_string(setting, "host", &sval);
            if (ret == CONFIG_TRUE)
                config->scanner_host = strdup(sval);
        }

        /* read the builder configuration */
        setting = config_lookup(&section, "builder");
        if (setting) {
            ret = config_setting_lookup_string(setting, "host", &sval);
            if (ret == CONFIG_TRUE)
                config->builder_host = strdup(sval);
        }

        /* read the extractor configuration */
        setting = config_lookup(&section, "extractor");
        if (setting) {
            ret = config_setting_lookup_string(setting, "host", &sval);
            if (ret == CONFIG_TRUE)
                config->extractor_host = strdup(sval);

            ret = config_setting_lookup_int(setting, "maxfilesize", &ival);
            if (ret == CONFIG_TRUE)
                config->extractor_maxfilesize = ival;

            ret = config_setting_lookup_int(setting, "nthreads", &ival);
            if (ret == CONFIG_TRUE)
                config->extractor_nthreads = ival;
        }
    }
    else {
        fprintf(stderr, "Cannot read %s: %s (line %d)\n",
                hpssix_sysconf_file,
                config_error_text(&section),
                config_error_line(&section));
    }

    return 0;
}

void hpssix_config_free(hpssix_config_t *config)
{
    if (config) {
        if (config->db_database)
            free(config->db_database);
        if (config->db_password)
            free(config->db_password);
        if (config->db_user)
            free(config->db_user);
        if (config->db_host)
            free(config->db_host);
        if (config->tag_prefix)
            free(config->tag_prefix);
        if (config->workdir)
            free(config->workdir);
        if (config->hpss_mountpoint)
            free(config->hpss_mountpoint);
        if (config->db2path)
            free(config->db2path);
        if (config->db2user)
            free(config->db2user);
        if (config->db2password)
            free(config->db2password);
        if (config->tika_host)
            free(config->tika_host);
        if (config->scanner_host)
            free(config->scanner_host);
        if (config->builder_host)
            free(config->builder_host);
        if (config->extractor_host)
            free(config->extractor_host);
    }
}

int hpssix_config_dump(hpssix_config_t *config, FILE *stream)
{
    if (!config) {
        errno = EINVAL;
        return -1;
    }

    fprintf(stream, "hpssix configuration:\n"
                    "* db_host = %s\n"
                    "* db_port = %d\n"
                    "* db_user = %s\n"
                    "* db_password = %d\n"
                    "* db_database = %s\n",
                    config->db_host,
                    config->db_port,
                    config->db_user,
                    config->db_password,
                    config->db_database);

    return 0;
}

static inline char *trim_extension(char *str)
{
    char *pos = NULL;

    if (!str)
        return str;

    /* trim whitespace in the end */
    pos = &str[strlen(str) - 1];
    while (*pos && isspace(*pos))
        *pos-- = '\0';

    /* make lower case */
    pos = str;
    while (*pos != '\0') {
        *pos = (char) tolower(*pos);
        pos++;
    }

    return str;
}

hpssix_extfilter_t *hpssix_extfilter_get(void)
{
    char *filter_file = NULL;
    FILE *fp = NULL;
    uint64_t lines = 0;
    uint64_t memsize = 0;
    char linebuf[LINE_MAX] = { 0, };
    hpssix_extfilter_t *filter = NULL;

    fp = fopen(hpssix_extfilter_file, "r");
    if (!fp)
        goto out;

    while (fgets(linebuf, LINE_MAX-1, fp) != NULL) {
        if (isspace(linebuf[0]) || linebuf[0] == '#')
            continue;

        lines++;
    }
    if (ferror(fp))
        goto out_close;

    memsize = sizeof(*filter) + lines*sizeof(char *) + 1;
    filter = malloc(memsize);
    if (!filter)
        goto out_close;

    memset((void *) filter, 0, memsize);

    rewind(fp);
    lines = 0;

    while (fgets(linebuf, LINE_MAX-1, fp) != NULL) {
        char *current = NULL;

        if (isspace(linebuf[0]) || linebuf[0] == '#')
            continue;

        current = strdup(trim_extension(linebuf));
        if (!current)
            goto out_close;
        filter->exts[lines] = current;

        lines++;
    }
    if (!ferror(fp))
        filter->n_exts = lines;

out_close:
    fclose(fp);
out:
    return filter;
}

void hpssix_extfilter_free(hpssix_extfilter_t *filter)
{
    uint64_t i = 0;

    if (filter) {
        if (filter->exts)
            for (i = 0; i < filter->n_exts; i++)
                free((char *) filter->exts[i]);

        free(filter);
    }

}
