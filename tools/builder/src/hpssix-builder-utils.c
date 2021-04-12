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
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "hpssix-builder.h"

enum {
    HPSSIX_DEFAULT_DEV = 44,
    HPSSIX_DEFAULT_RDEV = 0,
    HPSSIX_DEFAULT_BLKSIZE = 512,
};

/*
 * from hpss header to decode/code mode.
 */
#define NS_OBJECT_TYPE                   (0x80)
#define NS_OBJECT_TYPE_FILE              (0x81)
#define NS_OBJECT_TYPE_SYM_LINK          (0x82)
#define NS_OBJECT_TYPE_HARD_LINK         (0x83)
#define NS_OBJECT_TYPE_DIRECTORY         (0x84)
#define NS_OBJECT_TYPE_JUNCTION          (0x85)
#define NS_OBJECT_TYPE_FILESET_ROOT      (0x86)

#define NS_PERMS_CL   (0x01)  /* Control */
#define NS_PERMS_DL   (0x04)  /* Directory delete (w/wr permission) */
#define NS_PERMS_IN   (0x08)  /* Directory insert (w/wr permission) */
#define NS_PERMS_RD   (0x20)  /* Read */
#define NS_PERMS_WR   (0x40)  /* Write */
#define NS_PERMS_XS   (0x80)  /* Exec/Search */

static int get_st_mode(mode_t *mode_out, uint32_t type,
                       uint32_t uperm, uint32_t gperm, uint32_t operm)
{
    mode_t mode = 0;

    if (type == NS_OBJECT_TYPE_FILE)
        mode = S_IFREG;
    else if (type == NS_OBJECT_TYPE_SYM_LINK)
        mode = S_IFLNK;
    else if (type == NS_OBJECT_TYPE_DIRECTORY)
        mode = S_IFDIR;
    else /* other types maybe junction and fileset root */
        mode = 0;

    if (uperm & NS_PERMS_RD)
        mode |= S_IRUSR;
    if (uperm & NS_PERMS_WR)
        mode |= S_IWUSR;
    if (uperm & NS_PERMS_XS)
        mode |= S_IXUSR;

    if (gperm & NS_PERMS_RD)
        mode |= S_IRGRP;
    if (gperm & NS_PERMS_WR)
        mode |= S_IWGRP;
    if (gperm & NS_PERMS_XS)
        mode |= S_IXGRP;

    if (operm & NS_PERMS_RD)
        mode |= S_IROTH;
    if (operm & NS_PERMS_WR)
        mode |= S_IWOTH;
    if (operm & NS_PERMS_XS)
        mode |= S_IXOTH;

    *mode_out = mode;

    return 0;
}

int hpssix_builder_parse_fattr(const char *line, struct stat *sb)
{
    int ret = 0;
    mode_t mode = 0;
    struct stat _sb = { 0, };
    uint64_t object_id = 0; /* 1: field index */
    uint32_t type = 0;      /* 2 */
    uint32_t uperm = 0;     /* 3 */
    uint32_t gperm = 0;     /* 4 */
    uint32_t operm = 0;     /* 5 */
    uint64_t nlink = 0;     /* 6 */
    uint32_t uid = 0;       /* 7 */
    uint32_t gid = 0;       /* 8 */
    uint64_t size = 0;      /* 9 */
    uint64_t atime = 0;     /* 10 */
    uint64_t mtime = 0;     /* 11 */
    uint64_t ctime = 0;     /* 12 */

    ret = sscanf(line, "%lu,%u,%u,%u,%u,%lu,%u,%u,%lu,%lu,%lu,%lu,%u",
                 &object_id, &type,
                 &uperm, &gperm, &operm,
                 &nlink,
                 &uid, &gid,
                 &size,
                 &atime, &mtime, &ctime);
    if (ret != 12)
        return EINVAL;

    ret = get_st_mode(&mode, type, uperm, gperm, operm);
    if (ret)
        return ret;

    _sb.st_dev = HPSSIX_DEFAULT_DEV;
    _sb.st_ino = object_id;
    _sb.st_mode = mode;
    _sb.st_nlink = nlink;
    _sb.st_uid = uid;
    _sb.st_gid = gid;
    _sb.st_rdev = HPSSIX_DEFAULT_RDEV;
    _sb.st_size = size;
    _sb.st_blksize = HPSSIX_DEFAULT_BLKSIZE;
    _sb.st_blocks = CEILING(size, HPSSIX_DEFAULT_BLKSIZE);
    _sb.st_atime = atime;
    _sb.st_mtime = mtime;
    _sb.st_ctime = ctime;

    *sb = _sb;

    return ret;
}

static hpssix_xattr_t *parse_xattr(const char *xmlstr)
{
    int ret = 0;
    uint32_t i = 0;
    uint32_t count = 0;
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL;
    xmlNodePtr fsnode = NULL;
    hpssix_xattr_t *xattrs = NULL;

    if (xmlstr == NULL)
        return NULL;

    doc = xmlParseMemory(xmlstr, strlen(xmlstr));
    if (!doc)
        return NULL;

    node = xmlDocGetRootElement(doc);
    if (!node || xmlStrcmp(node->name, (const xmlChar *) "hpss"))
        goto out;

    fsnode = node->xmlChildrenNode;
    if (!fsnode || xmlStrcmp(fsnode->name, (const xmlChar *) "fs"))
        goto out;

    node = fsnode->xmlChildrenNode;

    while (node != NULL) {
        count++;
        node = node->next;
    }

    if (!count)
        goto out;

    xattrs = calloc(1, sizeof(*xattrs) + count*sizeof(hpssix_tag_t));
    if (!xattrs) {
        ret = ENOMEM;
        goto out;
    }

    node = fsnode->xmlChildrenNode;

    for (i = 0, node = fsnode->xmlChildrenNode;
         node != NULL;
         i++, node = node->next)
    {
        hpssix_tag_t *tag = &xattrs->tags[i];
        char *name = strdup(node->name);
        char *val = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);

        //hpssix_tag_parse(tag, name, val);
        tag->key = name;
        tag->val = val;
    }

    xattrs->n_tags = count;

out:
    if (doc)
        xmlFreeDoc(doc);

    return xattrs;
}

/* buffer size to read the serialized xattr */
static const uint64_t builder_xattr_bufsize = 50*(1<<20);

hpssix_xattr_t **builder_collect_xattrs(const char *file, uint64_t *out_count)
{
    uint64_t i = 0;
    uint64_t count = 0;
    uint64_t oid = 0;
    char *linebuf = NULL;
    char *pos = NULL;
    char *endptr = NULL;
    FILE *fp = NULL;
    hpssix_xattr_t **xattrs = NULL;
    hpssix_xattr_t *current = NULL;

    fp = fopen(file, "r");
    if (!fp)
        return NULL;

    linebuf = calloc(1, sizeof(char)*builder_xattr_bufsize);
    if (!linebuf)
        goto out_close;

    while (fgets(linebuf, builder_xattr_bufsize-1, fp) != NULL) {
        pos = strstr(linebuf, "<hpss><fs>");
        if (!pos)
            continue;

        count++;
    }

    rewind(fp);

    xattrs = calloc(1, sizeof(*xattrs)*count);
    if (!xattrs)
        goto out_free;

    while (fgets(linebuf, builder_xattr_bufsize-1, fp) != NULL) {
        pos = strstr(linebuf, "<hpss><fs>");
        if (!pos)
            continue;

        endptr = &pos[strlen(pos)-2];
        if (endptr[0] == '"')
            endptr[0] = '\0';

        oid = strtoull(linebuf, &endptr, 0);
        if (endptr[0] != ',')   /* parse error */
            goto out_free_all;

        current = parse_xattr(pos);
        if (!current)
            goto out_free_all;

        current->oid = oid;

        xattrs[i++] = current;
    }

    *out_count = count;
    goto out_free;

out_free_all:
    for ( ; i >= 0; i--) {
        if (xattrs[i])
            free(xattrs[i]);
    }
    free(xattrs);

out_free:
    free(linebuf);

out_close:
    fclose(fp);

    return xattrs;
}

int builder_filter_meta_extract(hpssix_builder_t *self, const char *path,
                                mode_t st_mode, size_t st_size)
{
    uint64_t i = 0;
    const char *filename = NULL;
    const char *extpos = NULL;
    char ext[NAME_MAX] = { 0, };
    char *pos = NULL;
    hpssix_extfilter_t *filter = self->filter;
    hpssix_config_t *config = self->config;

    if (!S_ISREG(st_mode))
        return 0;

    if (st_size > config->extractor_maxfilesize)
        return 0;

    filename = strrchr(path, '/');
    if (!filename)
        return 0;

    filename++;

    extpos = strrchr(filename, '.');
    if (!extpos)
        return 0;

    strcpy(ext, &extpos[1]);
    pos = ext;

    while (*pos != '\0') {
        pos[0] = (char) tolower(pos[0]);
        pos++;
    }

    for (i = 0; i < filter->n_exts; i++)
        if (strncmp(ext, filter->exts[i], strlen(ext)) == 0)
            return 1;

    return 0;
}

