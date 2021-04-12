connect to hsubsys1 user hpssdbp using oldrat99;
set schema hpss;

-- declare global temporary table session.target_objects as (
--     select n.object_id from nsobject n
--        inner join
--          (select distinct object_id
--             from nsobject_history
--             where sys_end > '2019-02-05' and object_id <= 6066200) h
--        on n.object_id = h.object_id
--     union all
--     select n.object_id from nsobject n where object_id > 6066200
-- ) definition only;

-- object
export to 'scanner.888.fattr.csv' of del
with target_objects (object_id) as (
    select n.object_id from nsobject n
       inner join
         (select distinct object_id
            from nsobject_history
            where sys_end > '2019-02-05' and object_id <= 6066200) h
       on n.object_id = h.object_id
    union all
    select n.object_id from nsobject n where object_id > 6066200)
select
    n.object_id,
    coalesce(ascii(n.type), 0),
    coalesce(ascii(n.user_perms), 0),
    coalesce(ascii(n.group_perms), 0),
    coalesce(ascii(n.other_perms), 0),
    coalesce(b.bfattr_link_count, 0),
    coalesce(n.uid, 0),
    coalesce(n.gid, 0),
    coalesce(b.bfattr_data_len, 0),
    coalesce(b.bfattr_read_time, 0),
    coalesce(b.bfattr_write_time, 0),
    coalesce(b.bfattr_modify_time, 0)
from
    nsobject n inner join target_objects on n.object_id = target_objects.object_id
    left outer join bitfile b on n.bitfile_id = b.bfid
order by n.object_id asc;

-- path
export to 'scanner.888.file.csv' of del
with target_objects (object_id) as (
    select n.object_id from nsobject n
       inner join
         (select distinct object_id
            from nsobject_history
            where sys_end > '2019-02-05' and object_id <= 6066200) h
       on n.object_id = h.object_id
    union all
    select n.object_id from nsobject n where object_id > 6066200),
fullpath (leaf, object_id, parent_id, path) as (
    select object_id as leaf, object_id, parent_id, '/' || name from nsobject
        where object_id in (select object_id from target_objects)
    union all
    select p.leaf as leaf, n.object_id, n.parent_id, '/' || name || p.path
        from fullpath p, nsobject n where p.parent_id = n.object_id)
select t.object_id, f.path from target_objects t join fullpath f
    on t.object_id = f.leaf where parent_id = 1 order by t.object_id asc;

-- xattrs
export to 'scanner.888.tags.csv' of del
with target_objects (object_id) as (
    select n.object_id from nsobject n
       inner join
         (select distinct object_id
            from nsobject_history
            where sys_end > '2019-02-05' and object_id <= 6066200) h
       on n.object_id = h.object_id
    union all
    select n.object_id from nsobject n where object_id > 6066200)
select u.object_id, xmlserialize(u.attributes as clob)
from userattrs u inner join target_objects on u.object_id = target_objects.object_id;

