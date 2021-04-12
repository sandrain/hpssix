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
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <hpssix.h>
#include <sys/time.h>

#include "hpssix-search.h"

static hpssix_db_t db;
static hpssix_config_t config;

enum {
    HPSSIX_SEARCH_PATH  = 1,
    HPSSIX_SEARCH_FATTR = 2,
    HPSSIX_SEARCH_TAG   = 4,
    HPSSIX_SEARCH_TSV   = 8,
};

static int search_mode = HPSSIX_SEARCH_FATTR;

static int countonly;
static int removed;
static int showino;
static int directory_only;
static int file_only;

static char *keyword;
static uint64_t limit;
static int verbose;
static int print_meta;
static int print_text;

static char *cond_str[N_COND_ARG];

const char *opstr[] = { "=", ">", ">=", "<", "<=", "<>" };

#define MAX_TAG_CONDS   128

static uint32_t n_tag_conds;
static hpssix_search_tag_cond_t tag_conds[MAX_TAG_CONDS];

static char datetimebuf[512];

static char *name;
static char *path;

static char *sql_fattr;
static char *sql_tags;
static char *sql_tsv;
static char sqlbuf[1<<20];

static inline void print_sql(char *sql)
{
    int i = 0;
    char ch = 0;

    printf("## SQL:");

    for (i = 0; i < strlen(sql); i++) {
        ch = sql[i];

        if (ch == '\n')
            printf("\n## ");
        else
            fputc(ch, stdout);
    }

    printf("\n\n");
}

struct timeval tv_start;
struct timeval tv_end;

static inline double timegap_sec(struct timeval *t1, struct timeval *t2)
{
    double usec = (t2->tv_sec - t1->tv_sec)*1e6 + (t2->tv_usec - t1->tv_usec);

    return usec/1e6;
}

static inline void print_result(FILE *out, PGresult *res)
{
    int i = 0;
    int rows = PQntuples(res);

    if (verbose)
        print_sql(sqlbuf);

    if (countonly)
        fprintf(out, "%s\n", PQgetvalue(res, 0, 0));
    else
        for (i = 0; i < rows; i++) {
            if (showino) {
                fprintf(out, "%10lu: %s%s\n", 
                             strtoull(PQgetvalue(res, i, 0), NULL, 0),
                             config.hpss_mountpoint, PQgetvalue(res, i ,1));
            }
            else
                fprintf(out, "%s%s\n",
                             config.hpss_mountpoint, PQgetvalue(res, i ,1));
        }

    if (verbose)
        fprintf(out, "\n## %lu records found in %.6lf seconds.\n",
                     rows, timegap_sec(&tv_start, &tv_end));
}

static inline uint64_t parse_datetime_str(char *str)
{
    int ret = 0;
    int date = 0;
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    struct tm tm = { 0, };

    ret = sscanf(str, "%d-%02d:%02d:%02d", &date, &hour, &min, &sec);
    if (ret != 4) {
        fprintf(stderr, "cannot parse the date: %s\n", str);
        exit(1);
    }

    year = date/10000;
    month = (date%10000)/100;
    day = date%100;

    memset((void *) &tm, 0, sizeof(tm));
    tm.tm_sec = sec;
    tm.tm_min = min;
    tm.tm_hour = hour;
    tm.tm_mday = day;
    tm.tm_mon = month - 1;
    tm.tm_year = year - 1900;
    tm.tm_isdst = -1;

    return (uint64_t) mktime(&tm);
}

/* parse YYYYMMDD-hh:mm:ss */
static char *parse_datetime(const char *_str)
{
    uint64_t d1 = 0;
    uint64_t d2 = 0;
    int op = -1;
    char *pos = NULL;
    char *str = strdup(_str);

    if (!str) {
        fprintf(stderr, "cannot allocate memory.\n");
        exit(1);
    }

    if (isdigit(str[0])) {
        if (NULL != (pos = strchr(str, ','))) {
            *pos++ = '\0';

            d1 = parse_datetime_str(str);
            d2 = parse_datetime_str(pos);

            sprintf(datetimebuf, "%lu,%lu", d1, d2);
            goto out_free;
        }
        else {
            d1 = parse_datetime_str(str);

            sprintf(datetimebuf, "%lu", d1);
            goto out_free;
        }
    }

    if (isdigit(str[1])) {
        d1 = parse_datetime_str(&str[1]);
        sprintf(datetimebuf, "%c%lu", str[0], d1);
    }
    else if (isdigit(str[2])) {
        d1 = parse_datetime_str(&str[2]);
        sprintf(datetimebuf, "%c%c%lu", str[0], str[1], d1);
    }
    else {
        fprintf(stderr, "cannot parse the date %s\n", str);
        exit(1);
    }

out_free:
    free(str);

    return datetimebuf;
}

/*
 * TODO: this function is kinda error-prone.
 */
static inline int parse_op(char *str, int *op, char **val1, char **val2)
{
    int ret = 0;
    char ch = 0;
    double rval = .0f;
    char *tmp = NULL;

    if (!str)
        return EINVAL;

    ret = sscanf(str, "%lf%c", &rval, &ch);
    if (ret > 0) {
        if (!ch) {
            *op = HPSSIX_OP_EQ;
            *val1 = str;

            return 0;
        }
        else if (ch == ',') {
            tmp = strchr(str, ',');

            ret = sscanf(&tmp[1], "%lf%c", &rval, &ch);
            if (ret != 1)
                goto strval;

            *op = HPSSIX_OP_BETWEEN;

            tmp[0] = '\0';
            *val1 = str;
            *val2 = &tmp[1];

            return 0;
        }
        else
            goto strval;
    }

    ret = sscanf(&str[1], "%lf%c", &rval, &ch);
    if (ret < 0)
        goto strval;
    else if (ret > 0) {
        if (ch)
            goto strval;

        *val1 = &str[1];

        switch (str[0]) {
        case '=':
            *op = HPSSIX_OP_EQ;
            break;
        case '>':
            *op = HPSSIX_OP_GT;
            break;
        case '<':
            *op = HPSSIX_OP_LT;
            break;
        default:
            goto strval;
        }

        return 0;
    }

    ret = sscanf(&str[2], "%lf%c", &rval, &ch);
    if (ret < 0)
        goto strval;
    else if (ret > 0) {
        if (ch)
            goto strval;

        *val1 = &str[2];

        if (strncmp(str, ">=", 2) == 0 || strncmp(str, "=>", 2) == 0) {
            *op = HPSSIX_OP_GE;
            return 0;
        }
        else if (strncmp(str, "<=", 2) == 0 || strncmp(str, "=<", 2) == 0) {
            *op = HPSSIX_OP_LE;
            return 0;
        }
        else if (strncmp(str, "!=", 2) == 0 || strncmp(str, "<>", 2) == 0) {
            *op = HPSSIX_OP_NE;
            return 0;
        }
        else
            goto strval;
    }

strval:
    *op = -1;

    return 0;
}

/*
 * format: <name:expr>
 *
 * terminate the program when the format is not correct.
 */
static void add_tag_cond(char *str)
{
    char *name = strdup(str);
    int op = 0;
    char *val = NULL;
    char *val1 = NULL;
    char *val2 = NULL;
    hpssix_search_tag_cond_t *cond = &tag_conds[n_tag_conds++];

    if (!name) {
        perror("cannot allocate memory");
        exit(ENOMEM);
    }

    val = strchr(name, ':');
    if (!val) {
        fprintf(stderr, "cannot parse the expr: %s\n", str);
        exit(EINVAL);
    }

    val[0] = '\0';

    sprintf(cond->tag_name, "user.hpssix.%s", name);

    cond->tag_val = &val[1];
    cond->op = -1;

    if (parse_op(cond->tag_val, &op, &val1, &val2) > 0) {
        fprintf(stderr, "cannot parse the expr: %s\n", str);
        exit(EINVAL);
    }

    cond->op = op;

    if (op >= 0) {
        sscanf(val1, "%lf", &cond->val1);
        if (val2)
            sscanf(val2, "%lf", &cond->val2);
    }
}

static inline
int parse_fattr_condition(hpssix_search_fattr_cond_t *cond, int attr)
{
    int ret = 0;
    int op = 0;
    char *str = cond_str[attr];
    char *val1 = NULL;
    char *val2 = NULL;

    ret = parse_op(str, &op, &val1, &val2);
    if (ret < 0)
        return EINVAL;

    if (op < 0)
        return EINVAL;

    cond->attr = attr;
    cond->op = op;
    cond->val1 = strtoull(val1, NULL, 0);

    if (val2)
        cond->val2 = strtoull(val2, NULL, 0);

    return 0;
}

static hpssix_search_fattr_cond_t cond_dir_mode = {
    .attr = COND_ARG_MODE,
    .op = HPSSIX_OP_BITAND,
    .val1 = S_IFDIR,
};

static hpssix_search_fattr_cond_t cond_file_mode = {
    .attr = COND_ARG_MODE,
    .op = HPSSIX_OP_BITAND,
    .val1 = S_IFREG,
};

static inline void fattr_get_sql(void)
{
    int ret = 0;
    int i = 0;
    hpssix_search_fattr_t fattr = { 0, };
    hpssix_search_fattr_cond_t cond = { 0, };

    hpssix_search_fattr_init(&fattr);

    fattr.path = path;
    fattr.name = name;

    for (i = 0; i < N_COND_ARG; i++) {
        if (cond_str[i]) {
            ret = parse_fattr_condition(&cond, i);
            if (ret) {
                fprintf(stderr, "failed to process the condition.\n");
                exit(EINVAL);
            }

            ret = hpssix_search_fattr_add_cond(&fattr, &cond);
            if (ret) {
                fprintf(stderr, "failed to process the condition.\n");
                exit(EINVAL);
            }
        }
    }

    if (directory_only + file_only > 0) {
        if (cond_str[COND_ARG_MODE]) {
            fprintf(stderr, "-D and -F options cannot be used with -m.\n");
            exit(EINVAL);
        }

        if (directory_only)
            hpssix_search_fattr_add_cond(&fattr, &cond_dir_mode);
        else if (file_only)
            hpssix_search_fattr_add_cond(&fattr, &cond_file_mode);
    }

    fattr.output = stdout;
    fattr.verbose = verbose;
    fattr.countonly = countonly;
    fattr.removed = removed;

    sql_fattr = hpssix_search_fattr_get_sql(&fattr);
}

static void tag_get_sql(void)
{
    hpssix_search_tag_t tag = { 0, };

    tag.count = n_tag_conds;
    tag.conds = tag_conds;

    sql_tags = hpssix_search_tag_get_sql(&tag);
}

static void tsv_get_sql(void)
{
    hpssix_search_tsv_t tsv = { 0, };

    tsv.keyword = keyword;
    tsv.limit = limit ? limit : 0;
    tsv.output = stdout;
    tsv.verbose = verbose;
    tsv.meta = print_meta;
    tsv.text = print_text;

    sql_tsv = hpssix_search_tsv_get_sql(&tsv);
}

static char *remove_single_quote(char *str)
{
    char *rpos = NULL;
    char *newstr = NULL;

    if (!str)
        return NULL;

    if (str[0] != '\'')
        return NULL;

    rpos = strrchr(str, '\'');
    if (!rpos)
        return NULL;
    *rpos = '\0';

    newstr = strdup(&str[1]);
    PQfreemem(str);

    return newstr;
}

static int get_file_predicate(char *out)
{
    int ret = 0;
    char *name_escaped = NULL;
    char *path_escaped = NULL;

    if (!name && !path)
        return 0;

    if (name) {
        name_escaped = PQescapeLiteral(db.dbconn, name, strlen(name));
        if (!name_escaped)
            return -errno;

        name_escaped = remove_single_quote(name_escaped);
    }

    if (path) {
        path_escaped = PQescapeLiteral(db.dbconn, path, strlen(path));
        if (!path_escaped) {
            ret = -errno;
            goto out_free;
        }

        path_escaped = remove_single_quote(path_escaped);
    }

    if (name_escaped && path_escaped)
        ret = sprintf(out, "WHERE "
                      "f.name LIKE '%%%%%s%%%%' AND f.path LIKE '%%%%%s%%%%' ",
                      name_escaped, path_escaped);
    else if (name_escaped)
        ret = sprintf(out, "WHERE f.name LIKE '%%%%%s%%%%' ", name_escaped);
    else if (path_escaped)
        ret = sprintf(out, "WHERE f.path LIKE '%%%%%s%%%%' ", path_escaped);
    else
        ret = -EINVAL;

out_free:
    if (path_escaped)
        free(path_escaped);
    if (name_escaped)
        free(name_escaped);

    return ret;
}

/*
 * main program
 */
static char *program;

static struct option const long_opts[] = {
    { "count-only", 0, 0, 'c' },
    { "date", 1, 0, 'd' },
    { "help", 0, 0, 'h' },
    { "keyword", 1, 0, 'k' },
    { "limit", 1, 0, 'l' },
    { "meta", 0, 0, 'V' },
    { "text", 0, 0, 't' },
    { "verbose", 0, 0, 'v' },
    { "inode", 1, 0, 'i' },
    { "mode", 1, 0, 'm' },
    { "uid", 1, 0, 'u' },
    { "gid", 1, 0, 'g' },
    { "size", 1, 0, 's' },
    { "name", 1, 0, 'n' },
    { "path", 1, 0, 'p' },
    { "atime", 1, 0, 'A' },
    { "mtime", 1, 0, 'M' },
    { "ctime", 1, 0, 'C' },
    { "tag", 1, 0, 'T' },
    { "show-ino", 0, 0, 'I' },
    { "directory-only", 0, 0, 'D' },
    { "file-only", 0, 0, 'F' },
    { "removed", 0, 0, 'r' },
    { 0, 0, 0, 0 },
};

static const char *short_opts = "A:M:C:DFIT:Vcd:hk:l:n:i:m:u:g:p:rs:tv";

static const char *usage_str =
"\n"
"Usage: %s [options] ...\n"
"\n"
"  -c, --count-only        Only print the number of resulting files.\n"
"  -d, --date=<datetime>   Look up based on <datetime> (YYYYMMDD-HH:mm:ss).\n"
"  -h, --help              Print this help message.\n"
"  -k, --keyword=<keyword> Look for document files having <keyword>.\n"
"  -l, --limit=<count>     Retrieve first <count> result.\n"
"  -V, --meta              Print document metadata.\n"
"  -t, --text              Print document fulltext.\n"
"  -v, --verbose           Show details about the result files.\n"
"  -i, --inode=<INODE>     Look up based on inode.\n"
"  -m, --mode=<MODE>       Look up based on mode.\n"
"  -u, --uid=<UID>         Look up based on uid.\n"
"  -g, --gid=<GID>         Look up based on gid.\n"
"  -s, --size=<SIZE>       Look up based on size.\n"
"  -n, --name=<keyword>    Look up files with filename containing <keyword>.\n"
"  -p, --path=<keyword>    Look up files with pathname containing <keyword>.\n"
"  -r, --removed           Look up from removed files.\n"
"  -A, --atime=<TIME>      Look up based on atime (access time).\n"
"  -M, --mtime=<TIME>      Look up based on mtime (modification time).\n"
"  -C, --ctime=<TIME>      Look up based on ctime (change time).\n"
"  -T, --tag=<name:expr>   Look up based on the tag <name:expr>.\n"
"  -I, --showino           Print inode number in result.\n"
"  -D, --directory-only    Search only directories.\n"
"  -F, --file-only         Search only regular files.\n"
"\n";

static void usage(int status)
{
    printf(usage_str, program);
    exit(status);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int optidx = 0;
    int i = 0;
    int count = 0;
    char *pos = NULL;
    PGresult *res = NULL;

    program = hpssix_path_basename(argv[0]);

    if (argc == 1)
        usage(0);

    while ((ch = getopt_long(argc, argv,
                             short_opts, long_opts, &optidx)) >= 0) {
        switch (ch) {
        case 'A':
            cond_str[COND_ARG_ATIME] = strdup(optarg);
            break;

        case 'C':
            cond_str[COND_ARG_CTIME] = strdup(optarg);
            break;

        case 'D':
            directory_only = 1;
            break;

        case 'F':
            file_only = 1;
            break;

        case 'M':
            cond_str[COND_ARG_MTIME] = strdup(optarg);
            break;

        case 'T':
            add_tag_cond(optarg);
            break;

        case 'I':
            showino = 1;
            break;

        case 'c':
            countonly = 1;
            break;

        case 'd':
            cond_str[COND_ARG_MTIME] = parse_datetime(optarg);
            break;

        case 'k':
            keyword = strdup(optarg);
            break;

        case 'l':
            limit = strtoull(optarg, NULL, 0);
            break;

        case 'V':
            print_meta = 1;
            break;

        case 'r':
            removed = 1;
            break;

        case 't':
            print_text = 1;
            break;

        case 'v':
            verbose = 1;
            break;

        case 'i':
            cond_str[COND_ARG_INODE] = strdup(optarg);
            break;

        case 'm':
            cond_str[COND_ARG_MODE] = strdup(optarg);
            break;

        case 'u':
            cond_str[COND_ARG_UID] = strdup(optarg);
            break;

        case 'g':
            cond_str[COND_ARG_GID] = strdup(optarg);
            break;

        case 's':
            cond_str[COND_ARG_SIZE] = strdup(optarg);
            break;

        case 'n':
            name = strdup(optarg);
            break;

        case 'p':
            path = strdup(optarg);
            break;

        case 'h':
        default:
            usage(0);
            break;
        }
    }

    pos = sqlbuf;

    if (name || path)
        search_mode |= HPSSIX_SEARCH_PATH;

    for (i = 0; i < N_COND_ARG; i++)
        if (cond_str[i])
            count++;

    /* fattr query should be included always for searching valid files only */
    fattr_get_sql();
    pos += sprintf(pos, "\n%s", sql_fattr);

    if (n_tag_conds) {
        search_mode |= HPSSIX_SEARCH_TAG;
        tag_get_sql();

        pos += sprintf(pos, ",\n%s", sql_tags);
    }

    if (keyword) {
        search_mode |= HPSSIX_SEARCH_TSV;
        tsv_get_sql();

        pos += sprintf(pos, ",\n%s", sql_tsv);
    }

    if (!search_mode)
        usage(0);

    if (directory_only && file_only) {
        fprintf(stderr, "you cannot specify -D and -F together.\n");
        ret = EINVAL;
        goto out;
    }

    ret = hpssix_config_read_sysconf(&config);
    if (ret) {
        fprintf(stderr, "failed to read the configuration file.\n");
        goto out;
    }

    ret = hpssix_db_connect(&db, &config);
    if (ret) {
        fprintf(stderr, "failed to connect to the database.\n");
        goto out;
    }

    if (countonly)
        pos += sprintf(pos, "\nSELECT COUNT(f.path) FROM fattr_oids ");
    else
        pos += sprintf(pos, "\nSELECT f.oid, f.path FROM fattr_oids ");

    if (search_mode & HPSSIX_SEARCH_TAG)
        pos += sprintf(pos, "NATURAL JOIN tag_oids ");

    if (search_mode & HPSSIX_SEARCH_TSV)
        pos += sprintf(pos, "NATURAL JOIN tsv_oids t ");

    pos += sprintf(pos, "NATURAL JOIN hpssix_file f ");

    ret = get_file_predicate(pos);
    if (ret < 0) {
        fprintf(stderr, "failed to generate a SQL.\n");
        goto out;
    }
    else
        pos += ret;

    if (search_mode & HPSSIX_SEARCH_TSV) {
        pos += sprintf(pos, "\nORDER BY "
                            "TS_RANK_CD(t.tsv, PLAINTO_TSQUERY('%s')) DESC ",
                            keyword);
    }

    if (limit)
        pos += sprintf(pos, " LIMIT %lu;", limit);

    gettimeofday(&tv_start, NULL);

    res = hpssix_db_psql_query(&db, sqlbuf);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "No data received!\n");
        goto out;
    }

    gettimeofday(&tv_end, NULL);

    print_result(stdout, res);

    PQclear(res);

out_disconnect:
    hpssix_db_disconnect(&db);
out:
    return ret;
}

