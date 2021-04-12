#!/bin/bash

function die() {
    echo "$1"
    exit 1
}

if [ $# -ne 2 ]; then
    die "Please check the usage."
fi

db2cmd="/opt/ibm/db2/default/bin/db2"
db2user="hpss"
db2password="oldrat99"
last_object_id="$1"
outfile="$2"

echo
echo "## scanning after oid ${last_object_id} .."
echo

#config_file="@sysconfdir@/hpssix/hpssix.conf"
#sql="export to $outfile of del 
#    select \
#    n.object_id, \
#    CAST(hex(n.type) as INT), \
#    n.uid, \
#    n.gid, \
#    b.bfattr_read_time, \
#    b.bfattr_write_time, \
#    b.bfattr_modify_time, \
#    CAST(hex(n.user_perms) as INT), \
#    CAST(hex(n.group_perms) as INT), \
#    CAST(hex(n.other_perms) as INT), \
#    b.bfattr_cos_id, \
#    b.bfattr_data_len, \
#    n.name \
#from nsobject n left outer join bitfile b \
#    on  n.bitfile_id = b.bfid
#	where n.object_id > $last_object_id"

#sql="export to $outfile of del
#with \
#fullpath (leaf, object_id, parent_id, path) as ( \
#    select object_id as leaf, object_id, parent_id, '/' || name from nsobject \
#        where object_id > $last_object_id \
#    union all \
#    select p.leaf as leaf, n.object_id, n.parent_id, '/' || name || p.path \
#        from fullpath p, nsobject n where p.parent_id = n.object_id), \
#attr (object_id, type, uid, gid, atime, mtime, ctime, uperm, gperm, operm, cos, size, name) as ( \
#    select \
#        n.object_id as object_id, \
#        CAST(hex(n.type) as INT) as type, \
#        n.uid as uid, \
#        n.gid as gid, \
#        b.bfattr_read_time as atime, \
#        b.bfattr_write_time as mtime, \
#        b.bfattr_modify_time as ctime, \
#        CAST(hex(n.user_perms) as INT) as uperm, \
#        CAST(hex(n.group_perms) as INT) as gperm, \
#        CAST(hex(n.other_perms) as INT) as operm, \
#        b.bfattr_cos_id as cos, \
#        b.bfattr_data_len as size, \
#        n.name as name \
#    from nsobject n left outer join bitfile b on  n.bitfile_id = b.bfid \
#        where n.object_id > $last_object_id) \
#select \
#    a.object_id, \
#    a.type, \
#    a.uid, \
#    a.gid, \
#    a.atime, \
#    a.mtime, \
#    a.ctime, \
#    a.uperm, \
#    a.gperm, \
#    a.operm, \
#    a.cos, \
#    a.size, \
#    a.name, \
#    f.path \
#from attr a join fullpath f on a.object_id = f.leaf \
#    where f.parent_id = 1"
#

sql="export to $outfile of del
with \
fullpath (leaf, object_id, parent_id, path) as ( \
    select object_id as leaf, object_id, parent_id, '/' || name from nsobject \
        where object_id > $last_object_id \
    union all \
    select p.leaf as leaf, n.object_id, n.parent_id, '/' || name || p.path \
        from fullpath p, nsobject n where p.parent_id = n.object_id), \
attr (object_id, type, uid, gid, atime, mtime, ctime, cos, size, name) as ( \
    select \
        n.object_id as object_id, \
        CAST(hex(n.type) as INT) as type, \
        n.uid as uid, \
        n.gid as gid, \
        b.bfattr_read_time as atime, \
        b.bfattr_write_time as mtime, \
        b.bfattr_modify_time as ctime, \
        b.bfattr_cos_id as cos, \
        b.bfattr_data_len as size, \
        n.name as name \
    from nsobject n left outer join bitfile b on  n.bitfile_id = b.bfid \
        where n.object_id > $last_object_id) \
select \
    a.object_id, \
    a.type, \
    a.uid, \
    a.gid, \
    a.atime, \
    a.mtime, \
    a.ctime, \
    a.cos, \
    a.size, \
    a.name, \
    f.path \
from attr a join fullpath f on a.object_id = f.leaf \
    where f.parent_id = 1"

echo $sql

$db2cmd connect to hsubsys1 user $db2user using $db2password \
    || die "connect to database failed"

$db2cmd set schema hpss || die "failed to set schema hpss"

$db2cmd "$sql"

$db2cmd connect reset

exit 0

