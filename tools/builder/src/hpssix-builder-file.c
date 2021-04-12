/* Copyright (C) 2019 - UT-Battelle, LLC. All right reserved.
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

#include "hpssix-builder.h"

static const char *builder_fattr_init_stmt =
"CREATE TEMPORARY TABLE __object AS (SELECT * FROM hpssix_object LIMIT 0);\n";

static const char *builder_fattr_copy_stmt =
"COPY __object (oid, st_dev, st_mode, st_nlink, st_uid, st_gid, st_rdev,\n"
"st_size, st_blksize, st_blocks, st_atime, st_mtime, st_ctime)\n"
"FROM STDIN WITH DELIMITER ',';\n";

static const char *builder_fattr_fini_stmt =
"WITH updated AS (\n"
"    UPDATE hpssix_object\n"
"       SET st_dev = s.st_dev,\n"
"           st_mode = s.st_mode,\n"
"           st_nlink = s.st_nlink,\n"
"           st_uid = s.st_uid,\n"
"           st_gid = s.st_gid,\n"
"           st_rdev = s.st_rdev,\n"
"           st_size = s.st_size,\n"
"           st_blksize = s.st_blksize,\n"
"           st_blocks = s.st_blocks,\n"
"           st_atime = s.st_atime,\n"
"           st_mtime = s.st_mtime,\n"
"           st_ctime = s.st_ctime\n"
"      FROM __object s\n"
"     WHERE hpssix_object.oid = s.oid\n"
" RETURNING s.oid\n"
") \n"
"INSERT INTO hpssix_object (oid, st_dev, st_mode, st_nlink, st_uid, st_gid,\n"
"                           st_rdev, st_size, st_blksize, st_blocks,\n"
"                           st_atime, st_mtime, st_ctime)\n"
"     SELECT s.oid, s.st_dev, s.st_mode, s.st_nlink, s.st_uid, s.st_gid,\n"
"            s.st_rdev, s.st_size, s.st_blksize, s.st_blocks, s.st_atime,\n"
"            s.st_mtime, s.st_ctime\n"
"       FROM __object s LEFT OUTER JOIN updated t ON s.oid = t.oid\n"
"      WHERE t.oid IS NULL;\n";
/* "DROP TABLE __object;\n"; */


static int builder_fattr_line_processor(const char *in, char *out, void *data)
{
    int ret = 0;
    struct stat sb = { 0, };
    char *pos = out;
    hpssix_builder_t *self = (hpssix_builder_t *) data;

    ret = hpssix_builder_parse_fattr(in, &sb);
    if (ret)
        return -1;

    pos += sprintf(pos, "%lu,", sb.st_ino);
    pos += sprintf(pos, "%lu,", sb.st_dev);
    pos += sprintf(pos, "%lu,", sb.st_mode);
    pos += sprintf(pos, "%lu,", sb.st_nlink);
    pos += sprintf(pos, "%lu,", sb.st_uid);
    pos += sprintf(pos, "%lu,", sb.st_gid);
    pos += sprintf(pos, "%lu,", sb.st_rdev);
    pos += sprintf(pos, "%lu,", sb.st_size);
    pos += sprintf(pos, "%lu,", sb.st_blksize);
    pos += sprintf(pos, "%lu,", sb.st_blocks);
    pos += sprintf(pos, "%lu,", sb.st_atime);
    pos += sprintf(pos, "%lu,", sb.st_mtime);
    pos += sprintf(pos, "%lu\n", sb.st_ctime);

    self->n_processed++;

    return 1;
}

int hpssix_builder_process_fattr(hpssix_builder_t *self)
{
    int ret = 0;
    hpssix_db_copy_t copy = { 0, };
    char scanner_output[PATH_MAX] = { 0, };

    hpssix_builder_get_scanner_filename(self, scanner_output,
                                        SCANNER_OUTPUT_FATTR);

    memset((void *) &copy, 0, sizeof(copy));

    copy.stmt_init = builder_fattr_init_stmt;
    copy.stmt_copy = builder_fattr_copy_stmt;
    copy.stmt_fini = builder_fattr_fini_stmt;
    copy.input_csv = scanner_output;
    copy.line_processor = &builder_fattr_line_processor;
    copy.line_processor_data = (void *) self;

    return hpssix_db_copy(self->db, &copy);
}

static const char *builder_path_init_stmt =
"CREATE TEMPORARY TABLE __file AS (SELECT * FROM hpssix_file LIMIT 0);\n";

static const char *builder_path_copy_stmt =
"COPY __file (oid, path) FROM STDIN WITH DELIMITER ',' CSV QUOTE '\"';\n";

static const char *builder_path_fini_stmt =
"WITH updated AS (\n"
"    UPDATE hpssix_file\n"
"       SET path = s.path\n"
"      FROM __file s\n"
"     WHERE hpssix_file.oid = s.oid\n"
" RETURNING s.oid\n"
") \n"
"INSERT INTO hpssix_file (oid, path)\n"
"     SELECT s.oid, s.path\n"
"       FROM __file s LEFT OUTER JOIN updated t ON s.oid = t.oid\n"
"      WHERE t.oid IS NULL;\n";

int hpssix_builder_process_path(hpssix_builder_t *self)
{
    int ret = 0;
    hpssix_db_copy_t copy = { 0, };
    char scanner_output[PATH_MAX] = { 0, };

    hpssix_builder_get_scanner_filename(self, scanner_output,
                                        SCANNER_OUTPUT_PATH);

    memset((void *) &copy, 0, sizeof(copy));

    copy.stmt_init = builder_path_init_stmt;
    copy.stmt_copy = builder_path_copy_stmt;
    copy.stmt_fini = builder_path_fini_stmt;
    copy.input_csv = scanner_output;

    return hpssix_db_copy(self->db, &copy);
}

static const char *builder_deleted_init_stmt =
"CREATE TEMPORARY TABLE __deleted AS\n"
"  (SELECT oid FROM hpssix_object LIMIT 0);\n";

static const char *builder_deleted_copy_stmt =
"COPY __deleted (oid) FROM STDIN;\n";

static const char *builder_deleted_fini_stmt =
"UPDATE hpssix_object SET valid = false FROM __deleted s\n"
" WHERE hpssix_object.oid = s.oid\n;"
"DROP TABLE __deleted;\n";

int hpssix_builder_process_deleted(hpssix_builder_t *self)
{
    int ret = 0;
    hpssix_db_copy_t copy = { 0, };
    char scanner_output[PATH_MAX] = { 0, };

    hpssix_builder_get_scanner_filename(self, scanner_output,
                                        SCANNER_OUTPUT_DELETED);

    memset((void *) &copy, 0, sizeof(copy));

    copy.stmt_init = builder_deleted_init_stmt;
    copy.stmt_copy = builder_deleted_copy_stmt;
    copy.stmt_fini = builder_deleted_fini_stmt;
    copy.input_csv = scanner_output;

    return hpssix_db_copy(self->db, &copy);
}

