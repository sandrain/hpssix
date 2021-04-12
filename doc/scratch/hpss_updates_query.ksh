#!/usr/bin/env ksh

db2 connect to hsubsys1
db2 set schema hpss

#get all new records that have been updated in the external metadata server
# the export to <file_name> and date or timestamp field need to
#made as variable input parameters to this script for the query below.

db2 " export to /ccs/hpssdev/reference/MetadataIndexing/raghul_devel/new_updates.csv of del 
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
	where n.object_id IN (SELECT distinct h.object_id from nsobject_history h 
        where h.sys_end > '2018-01-10') " 

db2 connect reset
 

#append records for insert into psql

sed -i 's/^/INSERT into objectstat values\(/g' /ccs/hpssdev/reference/MetadataIndexing/raghul_devel/new_updates.csv
sed -i 's/$/\) ON CONFLICT (object_id) \ 
        DO UPDATE SET parent_id = Excluded.parent_id, \
            type  = Excluded.type, \
            bfid = Excluded.bfid, \
            uid = Excluded.uid, \
            gid = Excluded.gid, \
            account = Excluded.account, \
            time_created = Excluded.time_created, \
            bfattr_create_time = Excluded.bfattr_create_time, \
            bfattr_read_time = Excluded.bfattr_read_time, \
            bfattr_write_time = Excluded.bfattr_write_time, \
            bfattr_modify_time = Excluded.bfattr_modify_time, \
            user_perms = Excluded.user_perms, \
            group_perms = Excluded.group_perms, \
            other_perms = Excluded.other_perms, \
            bfattr_cos_id = Excluded.bfattr_cos_id, \
            bfattr_data_len = Excluded.bfattr_data_len, \
            name = Excluded.name;/g' /ccs/hpssdev/reference/MetadataIndexing/raghul_devel/new_updates.csv


#load the file into psql


