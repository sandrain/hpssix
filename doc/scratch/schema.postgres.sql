-- \connect hpssix

drop table if exists hpssix_object_stat cascade;
drop table if exists hpssix_object_path cascade;
drop table if exists hpssix_object_attr_key cascade;
drop table if exists hpssix_object_attr_val cascade;
drop table if exists hpssix_object_tsv;

-- hpss files/directories
create table hpssix_object_stat (
  oid bigserial primary key,
  object_id bigint not null unique,
  parent_id bigint not null,
  type smallint not null,
  bf_id varchar(64) default null,
--   uid integer not null,
--   gid integer not null,
--   account integer not null,
--   nsobject_create_time integer not null,
--   bfattr_create_time integer default null,
--   bfattr_read_time integer default null,
--   bfattr_write_time integer default null,
--   bfattr_modify_time integer default null,
--   user_perms varchar(6),
--   group_perms varchar(4),
--   other_perms varchar(4),
--   bfattr_cos_id integer,
--   bfattr_data_len bigint,
  name text not null
);

-- hpss files/directories full path
create table hpssix_object_path (
  pid bigserial primary key,
  oid bigint not null references hpssix_object_stat(oid) on delete cascade,
  parent_id bigint not null,
  name text not null,
  path text default null
);

-- object attribute keys
create table hpssix_object_attr_key (
  kid bigserial primary key,
  key text not null
);

-- pre-populate the known attribute keys (stat)
insert into hpssix_object_attr_key (key) values ('uid');
insert into hpssix_object_attr_key (key) values ('gid');
insert into hpssix_object_attr_key (key) values ('account');
insert into hpssix_object_attr_key (key) values ('user_perms');
insert into hpssix_object_attr_key (key) values ('group_perms');
insert into hpssix_object_attr_key (key) values ('other_perms');
insert into hpssix_object_attr_key (key) values ('nsobject_create_time');
insert into hpssix_object_attr_key (key) values ('bfattr_create_time');
insert into hpssix_object_attr_key (key) values ('bfattr_read_time');
insert into hpssix_object_attr_key (key) values ('bfattr_write_time');
insert into hpssix_object_attr_key (key) values ('bfattr_write_time');
insert into hpssix_object_attr_key (key) values ('bfattr_modify_time');
insert into hpssix_object_attr_key (key) values ('bfattr_cos_id');
insert into hpssix_object_attr_key (key) values ('bfattr_data_len');

-- object attribute values
create table hpssix_object_attr_val (
  xid bigserial primary key,
  oid bigint not null references hpssix_object_stat(oid) on delete cascade,
  kid bigint not null references hpssix_object_attr_key(kid) on delete cascade,
  ival integer,
  rval real,
  sval text
);

-- full text search index
create table hpssix_object_tsv (
 tid bigserial primary key,
 object_id bigint not null,
 tsv tsvector
);

