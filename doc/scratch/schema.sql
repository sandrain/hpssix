-- HPSS MD-index external db

\connect hpssmddb;

drop table if exists ObjectStat CASCADE;
drop table if exists ObjectPath;
drop table if exists ObjectAttrKey CASCADE;
drop table if exists ObjectAttrVal;

-- hpss files/directories
create table ObjectStat (
  object_id bigint not null,
  parent_id bigint not null,
  type smallint not null,
  bf_id varchar(64) DEFAULT NULL,
  uid integer not null,
  gid integer not null,
  account integer not null,
  nsobject_create_time integer not null,
  bfattr_create_time integer DEFAULT NULL,
  bfattr_read_time integer DEFAULT null,
  bfattr_write_time integer DEFAULT null,
  bfattr_modify_time integer DEFAULT null,
  user_perms varchar(6),
  group_perms varchar(4),
  other_perms varchar(4),
  bfattr_cos_id integer,
  bfattr_data_len bigint,
  name text not null,
  primary key(object_id)
);

-- hpss files/directories full path
create table ObjectPath (
  object_id integer not null references ObjectStat(object_id) ON DELETE CASCADE,
  parent_id bigint not null,
  name text not null,
  path text DEFAULT null,
  primary key(object_id)
);

-- ObjectAttrKey
create table ObjectAttrKey (
  kid integer not null,
  key text not null,
  primary key(kid)
);

-- ObjectAttrVal
create table ObjectAttrVal (
  xid integer not null,
  object_id integer not null references ObjectStat(object_id) ON DELETE CASCADE,
  kid integer not null references ObjectAttrKey(kid) ON DELETE CASCADE,
  ival integer,
  rval real,
  sval text,
  primary key(xid)
);
