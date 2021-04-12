/* Copyright (C) 2018 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * 
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * 
 */
#include <config.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>
#include <sqlite3.h>

#include "hpssix-extractor.h"

#define call_curl(fn)                                           \
        do {                                                    \
            CURLcode c = (fn);                                  \
            if (c != CURLE_OK)                                  \
                fprintf(stderr, "%s\n", curl_easy_strerror(c)); \
        } while (0)

static
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *priv)
{
    int ret = 0;
    FILE *tmpfp = (FILE *) priv;

    ret = fwrite((void *) ptr, nmemb, 1, tmpfp);
    if (ret != 1)
        return 0;

    return nmemb;
}

static inline void reset_tmpfile(FILE *tmpfp)
{
    if (tmpfp) {
        rewind(tmpfp);
        ftruncate(fileno(tmpfp), 0);
    }
}

static inline void get_tika_url_meta(hpssix_extractor_data_t *data, char *buf)
{
    sprintf(buf, "http://%s:%d/meta", data->tika_host, data->tika_port);
}

static inline
void get_tika_url_content(hpssix_extractor_data_t *data, char *buf)
{
    sprintf(buf, "http://%s:%d/tika", data->tika_host, data->tika_port);
}

int hpssix_extractor_get_meta(hpssix_extractor_data_t *data)
{
    int ret = 0;
    struct curl_slist *list = NULL;
    CURL *curl = NULL;
    CURLcode cc = 0;
    char *buf = NULL;
    size_t datalen = 0;
    char url[1024] = { 0, };

    rewind(data->fp);
    reset_tmpfile(data->tmpfp);
    get_tika_url_meta(data, url);

    curl = curl_easy_init();
    list = curl_slist_append(list, "Accept: application/json");

    cc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    cc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) data->tmpfp);
    cc = curl_easy_setopt(curl, CURLOPT_URL, url);
    cc = curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    cc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    cc = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    cc = curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, data->file_size);
    cc = curl_easy_setopt(curl, CURLOPT_READDATA, data->fp);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed (%s).\n",
                curl_easy_strerror(cc));
        return EIO;
    }

    cc = curl_easy_perform(curl);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed (%s).\n",
                curl_easy_strerror(cc));
        return EIO;
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    datalen = ftell(data->tmpfp);
    rewind(data->tmpfp);

    if (datalen > 0) {
        buf = calloc(1, datalen+1);
        if (!buf)
            return ENOMEM;

        ret = fread(buf, datalen, 1, data->tmpfp);
        if (ret != 1)
            return errno;

        data->meta = buf;
        ret = 0;
    }
    else {
        data->meta = NULL;
        ret = EINVAL;
    }

    return ret;
}


int hpssix_extractor_get_content(hpssix_extractor_data_t *data)
{
    int ret = 0;
    struct curl_slist *list = NULL;
    CURL *curl = NULL;
    CURLcode cc;
    char *buf = NULL;
    size_t datalen = 0;
    char url[1024] = { 0, };

    rewind(data->fp);
    reset_tmpfile(data->tmpfp);
    get_tika_url_content(data, url);

    curl = curl_easy_init();
    list = curl_slist_append(list, "Accept: text/plain");

    cc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    cc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) data->tmpfp);
    cc = curl_easy_setopt(curl, CURLOPT_URL, url);
    cc = curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    cc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    cc = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    cc = curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, data->file_size);
    cc = curl_easy_setopt(curl, CURLOPT_READDATA, data->fp);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed (%s).\n",
                curl_easy_strerror(cc));
        return EIO;
    }

    cc = curl_easy_perform(curl);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed (%s).\n",
                curl_easy_strerror(cc));
        return EIO;
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    datalen = ftell(data->tmpfp);
    rewind(data->tmpfp);

    buf = calloc(1, datalen+1);
    if (!buf)
        return ENOMEM;

    ret = fread(buf, datalen, 1, data->tmpfp);
    if (ret != 1)
        return errno;

    data->content = buf;

    return 0;
}

