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

#include <hpssix.h>

static char *tika_host = "localhost";
static int tika_port = 9998;

struct extractor_data {
    uint64_t object_id;
    char *path;
    char *meta;
    char *content;

    FILE *fp;
    FILE *tmpfp;
};

static struct extractor_data data;

static char *testfile = "paper";
static struct stat testfile_stat;

#define call_curl(fn)                                           \
        do {                                                    \
            CURLcode c = (fn);                                  \
            if (c != CURLE_OK)                                  \
                fprintf(stderr, "%s\n", curl_easy_strerror(c)); \
        } while (0)

static
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    int ret = 0;
    FILE *tmpfp = (FILE *) userdata;

    ret = fwrite((void *) ptr, nmemb, 1, tmpfp);
    if (ret != 1) {
        perror("fwrite");
        exit(1);
    }

    return nmemb;
}

static int extract_metadata(struct extractor_data *data)
{
    int ret = 0;
    FILE *fp = data->fp;
    FILE *tmpfp = data->tmpfp;
    struct curl_slist *list = NULL;
    CURL *curl = NULL;
    CURLcode cc;
    char *buf = NULL;
    int tmpfd = 0;
    size_t datalen = 0;

    rewind(fp);
    rewind(tmpfp);
    ftruncate(fileno(tmpfp), 0);

    curl = curl_easy_init();

    list = curl_slist_append(list, "Accept: application/json");

    cc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    cc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) tmpfp);
    cc = curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:9998/meta");
    cc = curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    cc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    cc = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    cc = curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, testfile_stat.st_size);
    cc = curl_easy_setopt(curl, CURLOPT_READDATA, fp);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed.\n");
        return 1;
    }

    cc = curl_easy_perform(curl);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed.\n");
        return 1;
    }

    tmpfd = fileno(tmpfp);

    datalen = ftell(tmpfp);
    rewind(tmpfp);

    buf = calloc(1, datalen+1);
    assert(buf);

    ret = fread(buf, datalen, 1, tmpfp);
    assert(ret == 1);

    data->meta = buf;

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    return 0;
}


static int extract_content(struct extractor_data *data)
{
    int ret = 0;
    FILE *fp = data->fp;
    FILE *tmpfp = data->tmpfp;
    struct curl_slist *list = NULL;
    CURL *curl = NULL;
    CURLcode cc;
    char *buf = NULL;
    int tmpfd = 0;
    size_t datalen = 0;

    rewind(fp);
    rewind(tmpfp);
    ftruncate(fileno(tmpfp), 0);

    curl = curl_easy_init();

    list = curl_slist_append(list, "Accept: text/plain");

    cc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    cc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) tmpfp);
    cc = curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:9998/tika");
    cc = curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    cc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    cc = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    cc = curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, testfile_stat.st_size);
    cc = curl_easy_setopt(curl, CURLOPT_READDATA, fp);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed.\n");
        return 1;
    }

    cc = curl_easy_perform(curl);
    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E] curl processing failed.\n");
        return 1;
    }

    datalen = ftell(tmpfp);
    rewind(tmpfp);

    buf = calloc(1, datalen+1);
    assert(buf);

    ret = fread(buf, datalen, 1, tmpfp);
    assert(ret == 1);

    data->content = buf;

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    return 0;
}

static struct option options[] = {
    { "help", no_argument, 0, 'h' },
    { "tika-host", required_argument, 0, 'a' },
    { "file", required_argument, 0, 'f' },
    { "tika-port", required_argument, 0, 'p' },
    { 0, 0, 0, 0},
};

static const char *optstr = "a:f:hp:";

static const char *usage_str = "\n"
"Usage: extractor [options] ...\n"
"\n"
"Available options:\n"
"  -a, --tika-host        tika host address (default: localhost).\n"
"  -f, --file             input file to be used.\n"
"  -p, --tika-port        tika server port (default: 9998).\n"
"  -h, --help             print the help message.\n"
"\n";

static void usage_exit(void)
{
    fputs(usage_str, stderr);
    exit(1);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int optind = 1;
    int ch = 0;
    uint64_t i = 0;
    FILE *fp = NULL;

    while ((ch = getopt_long(argc, argv, optstr, options, &optind)) != -1) {
        switch (ch) {
        case 'a':
            tika_host = strdup(optarg);
            break;

        case 'f':
            testfile = strdup(optarg);
            break;

        case 'p':
            tika_port = atoi(optarg);
            break;

        case 'h':
        case '?':
        default:
            usage_exit();
            break;
        }
    }

    ret = stat(testfile, &testfile_stat);
    if (ret < 0) {
        perror("## [E] stat");
        return errno;
    }

    fp = fopen(testfile, "r");
    assert(fp);

    data.path = testfile;
    data.fp = fp;
    data.tmpfp = tmpfile();

    assert(data.tmpfp);

    curl_global_init(CURL_GLOBAL_ALL);

    ret = extract_metadata(&data);
    if (ret) {
        fprintf(stderr, "extract_metadata failed.\n");
        return 1;
    }

    ret = extract_content(&data);
    if (ret) {
        fprintf(stderr, "extract_content failed.\n");
        return 1;
    }

    printf("## meta: %s\n", data.meta);
    printf("## content: %s\n", data.content);

    curl_global_cleanup();

    return ret;
}
