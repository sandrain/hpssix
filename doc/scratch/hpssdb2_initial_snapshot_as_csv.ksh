#!/usr/bin/env ksh

#Query to get complete metadata snapshot from HPSS db2
#compliant with the schema.sql 

db2 connect to hsubsys1
db2 set schema hpss

db2 " export to /ccs/hpssdev/reference/MetadataIndexing/initial_snapshot.csv of del
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
    on  n.bitfile_id = b.bfid"

db2 connect reset
