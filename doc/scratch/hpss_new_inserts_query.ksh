#!/usr/bin/env ksh

# Query to get new records from the DB2, based on the last nsobject_id in 
# the external metadata server.
# the export to <file_name> and last nsobject_id need need to
# made as variable input parameters to this script for the query below.

db2 connect to hsubsys1
db2 set schema hpss

db2 " export to /ccs/hpssdev/reference/MetadataIndexing/raghul_devel/Dec26.csv of del 
    select \
    n.object_id, \
    n.parent_id, \
    CAST(hex(n.type) as INT), \
    hex(b.bfid), \
    n.uid, \
    n.gid, \
    n.account, \
    n.time_created, \
    b.bfattr_create_time, \
    b.bfattr_read_time, \
    b.bfattr_write_time, \
    b.bfattr_modify_time, \
    hex(n.user_perms), \
    hex(n.group_perms), \
    hex(n.other_perms), \
    b.bfattr_cos_id, \
    b.bfattr_data_len, \
    n.name \
from nsobject n left outer join bitfile b \
    on  n.bitfile_id = b.bfid
	where n.object_id > 17823"

db2 connect reset
 
