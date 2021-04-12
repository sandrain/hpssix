/* Copyright (C) 2019 - UT-Battelle, LLC. All right reserved.
 *
 * Please refer to COPYING for the license.
 *
 * Written by: Hyogi Sim <simh@ornl.gov>
 * ---------------------------------------------------------------------------
 * TODO: let postgresql parse the xml data itself.
 */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "hpssix-builder.h"

static const char *xattr_copy_stmt =
"COPY hpssix_attr_val (oid, kid, sval) FROM STDIN\n"
"  WITH DELIMITER ',' CSV QUOTE ''''\n";

int hpssix_builder_process_xattrs(hpssix_builder_t *self)
{
    int ret = 0;
    int abort = 0;
    uint64_t i = 0;
    uint64_t j = 0;
    uint64_t file_count = 0;
    hpssix_xattr_t **xattrs = NULL;
    char linebuf[LINE_MAX] = { 0, };
    char scanner_output[PATH_MAX] = { 0, };

    hpssix_builder_get_scanner_filename(self, scanner_output,
                                        SCANNER_OUTPUT_XATTR);

    xattrs = builder_collect_xattrs(scanner_output, &file_count);
    if (!xattrs) {
        ret = errno;
        goto out;
    }

    ret = hpssix_db_delete_xattrs(self->db, xattrs, file_count);
    if (ret)
        goto out_free;

    ret = hpssix_db_populate_xattr_keys(self->db, xattrs, file_count);
    if (ret)
        goto out_free;

    ret = hpssix_db_copy_init(self->db, xattr_copy_stmt);
    if (ret)
        goto out_free;

    for (i = 0; i < file_count; i++) {
        hpssix_xattr_t *current = xattrs[i];

        for (j = 0; j < current->n_tags; j++) {
            hpssix_tag_t *tag = &current->tags[j];
            char *eval = hpssix_db_escape_literal(self->db, tag->val);

            sprintf(linebuf, "%lu,%lu,%s\n", current->oid, tag->kid, eval);

            ret = hpssix_db_copy_put(self->db, linebuf);

            hpssix_db_freemem((void *) eval);

            if (ret) {
                abort = EIO;
                goto finish;
            }
        }
    }

finish:
    ret = hpssix_db_copy_end(self->db, abort);
    if (abort)  /* if copy failed, return previous error */
        ret = abort;

out_free:
    for (i = 0; i < file_count; i++)
        if (xattrs[i])
            free(xattrs[i]);

    free(xattrs);

out:
    return ret;
}

