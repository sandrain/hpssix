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
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <curl/curl.h>
#include <sqlite3.h>

#include <hpssix.h>

static char *tika_host = "localhost";
static int tika_port = 9998;
static uint64_t seconds = 30;

static uint64_t nthreads = 1;
static pthread_t *threads;

static int content_mode;        /* just display the content */

static uint64_t total_count;

struct extractor_data {
    int id;
    uint64_t count;     /* processed files */
    FILE *tmpfp;
};

static struct extractor_data *worker_data;

static volatile int terminate;
static const char *testfile = "paper";
static struct stat testfile_stat;

static void dump_extracted_data(struct extractor_data *data)
{
    FILE *fp = data->tmpfp;
    char linebuf[LINE_MAX] = { 0, };

    rewind(fp);

    while (fgets(linebuf, LINE_MAX-1, fp) != NULL)
        fputs(linebuf, stdout);

    if (ferror(fp))
        perror("fgets failed");

    fclose(fp);
}

#define call_curl(fn)                                           \
        do {                                                    \
            CURLcode c = (fn);                                  \
            if (c != CURLE_OK)                                  \
                fprintf(stderr, "%s\n", curl_easy_strerror(c)); \
        } while (0)


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

static void *extractor_worker_func(void *arg)
{
    int ret = 0;
    struct extractor_data *data = (struct extractor_data *) arg;
    int id = data->id;
    FILE *fp = NULL;
    FILE *tmpfp = data->tmpfp;
    struct curl_slist *list = NULL;
    CURL *curl = NULL;
    CURLcode cc;

#if 0
    tmpfp = tmpfile();
    if (!tmpfp) {
        fprintf(stderr, "## [E:%d] tmpfile failed.\n", id);
        goto out;
    }
#endif

    fp = fopen(testfile, "r");
    if (!fp) {
        fprintf(stderr, "## [E:%d] fopen failed.\n", id);
        goto out;
    }

    curl = curl_easy_init();

    list = curl_slist_append(list, "Accept: application/json");

    cc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    cc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) tmpfp);
    cc = curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:9998/rmeta");
    cc = curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    cc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    cc = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    cc = curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, testfile_stat.st_size);
    cc = curl_easy_setopt(curl, CURLOPT_READDATA, fp);

    if (cc != CURLE_OK) {
        fprintf(stderr, "## [E:%d] curl processing failed.\n", id);
        goto out;
    }

    while (!terminate) {
        cc = curl_easy_perform(curl);
        if (cc != CURLE_OK) {
            fprintf(stderr,
                    "## [E:%d] curl processing failed at iteration %lu.\n",
                    id, data->count);
        }

        data->count++;

        if (content_mode)
            terminate = 1;
        else {
            if (terminate)
                break;
            rewind(fp);

            rewind(tmpfp);
            ret = ftruncate(fileno(tmpfp), 0);
            if (ret)
                fprintf(stderr, "## [#:%d] ftruncate.\n", id);
        }

    }

    fclose(fp);

out:
    return (void *) 0;
}

static struct option options[] = {
    { "content", no_argument, 0, 'c' },
    { "tika-host", required_argument, 0, 'a' },
    { "file", required_argument, 0, 'f' },
    { "help", no_argument, 0, 'h' },
    { "nthreads", required_argument, 0, 'n' },
    { "tika-port", required_argument, 0, 'p' },
    { "seconds", required_argument, 0, 's' },
    { 0, 0, 0, 0},
};

static const char *optstr = "a:cf:hn:p:s:";

static const char *usage_str = "\n"
"Usage: extractor [options] ...\n"
"\n"
"Available options:\n"
"  -a, --tika-host        tika host address (default: localhost).\n"
"  -c, --content          show the contents rather than performance test.\n"
"  -f, --file             input file to be used.\n"
"  -p, --tika-port        tika server port (default: 9998).\n"
"  -n, --nthreads=<NUM>   number of threads to be spawned.\n"
"  -s, --seconds=<SEC>    run test for SEC seconds (default: 30).\n"
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

    while ((ch = getopt_long(argc, argv, optstr, options, &optind)) != -1) {
        switch (ch) {
        case 'a':
            tika_host = strdup(optarg);
            break;

        case 'c':
            content_mode = 1;
            break;

        case 'f':
            testfile = strdup(optarg);
            break;

        case 'n':
            nthreads = strtoull(optarg, 0, 0);
            break;

        case 'p':
            tika_port = atoi(optarg);
            break;

        case 's':
            seconds = strtoull(optarg, 0, 0);
            break;

        case 'h':
        case '?':
        default:
            usage_exit();
            break;
        }
    }

    if (!content_mode && nthreads <= 0) {
        fprintf(stderr, "nthreads should be greater than 0.\n");
        return EINVAL;
    }

    if (content_mode)
        nthreads = 1;

    threads = calloc(nthreads, sizeof(*threads) + sizeof(*worker_data));
    if (!threads) {
        perror("## [E] calloc");
        return errno;
    }

    worker_data = (struct extractor_data *) &threads[nthreads];

    ret = stat(testfile, &testfile_stat);
    if (ret < 0) {
        perror("## [E] stat");
        return errno;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    for (i = 0; i < nthreads; i++) {
        struct extractor_data *data = &worker_data[i];
        FILE *tmpfp = tmpfile();

        if (!tmpfp) {
            perror("## [E] tmpfile failed");
            goto out;
        }

        data->id = i;
        data->tmpfp = tmpfp;

        ret = pthread_create(&threads[i], 0, extractor_worker_func,
                             (void *) data);
        if (ret) {
            perror("## [E] pthread_create failed");
            goto out;
        }
    }

    if (!content_mode) {
        sleep(seconds);
        terminate = 1;
    }

    for (i = 0; i < nthreads; i++) {
        ret = pthread_join(threads[i], NULL);
        if (ret)
            perror("## [E] pthread_join");

        if (!content_mode)
            fclose(worker_data[i].tmpfp);

        printf("## [%lu] processed %lu files.\n", i, worker_data[i].count);

        total_count += worker_data[i].count;
    }

    if (content_mode)
        dump_extracted_data(worker_data);

    printf("## %lu files processed for %lu seconds (%.3f files/sec).\n",
           total_count, seconds, (1.0F*total_count)/(1.0F*seconds));

out:
    free(threads);

    return ret;
}
