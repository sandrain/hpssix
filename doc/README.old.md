# HPSSIX

HPSSIX incrementally indexes metadata of files in running HPSS system and
allows users to quickly locate files of interest with file metadata, including
POSIX file attributes (or stat metadata) and POSIX extended attributes. HPSSIX
also features the fulltext search for supported file types, e.g., Microsoft
Office document files.

This document provides the technical overview of HPSSIX.

## Data directory structure

HPSSIX maintains internal files inside the `${prefix}/var/hpssix/data`
directory, which is organized by the following files and directories:

* `/pid`: Maintains the pid files for daemons.
* `/data`: Scanned file lists in the sqlite3 format. Files are organized in
  `YEAR/YYYYMMDD/`.
* `hpssix.db`: Keeps track of operation logs and the current scanning status of
  HPSS.

# Below is the old documentation.

## Source code organization

* `README.md`: This file
* `scripts/`: Scripts to build the index database from HPSS DB2
database.
* `tika/`: Tika software and testing scripts for metadata extraction.

## Building the metadata index database

HPSS system maintains its own metadata database using IBM DB2 database, which
cannot be directly used for metadata-based file search. To this end, we
currently use PostgreSQL to build the metadata index database. The following
code presents the database schema of metadata index database.

```sql
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
```

In specific, we populate the PostgreSQL database by scanning the DB2 database in
HPSS on a daily basis. This involves the following three separate tasks.

1. Initial snapshot population
2. Incremental update
3. Pathname generation

Below, we explain each step in detail.

### Initial snapshot population

This process is to populate the initially empty metadata index database. This
process further consists of two steps: 1) dumping the records from the HPSS DB2
database into a csv file, and 2) populating the PostgreSQL database from the csv
file.

#### Creating the CSV file from the HPSS DB2

* source code: `scripts/hpss_initial_snapshot_as_csv.ksh`

The very first step is to initial population of the metadata index database.
The script connects to the HPSS DB2 database and dump all records into a csv
file (e.g., `today.csv`).
The above script file (`scripts/hpss_initial_snapshot_as_csv.ksh`) dumps all
records from the `nsobject` and `bitfile` tables into a csv file.

#### Populating the PostgreSQL database

After successfully creating the csv file (e.g., `today.csv`), the records can be
imported by the following PostgreSQL command:

```SQL
---
--- \copy <table_name> FROM '<PATH_NAME>' DELIMITER ',' CSV
---
\copy ObjectStat FROM /path/to/today.csv DELIMITER ',' CSV
```

### Incremental update

* source code: `scripts/hpss_new_inserts_query.ksh`,
               `scripts/hpss_updates_query.ksh`,
               `scripts/hpss_deletes_query.ksh`

After the initial population, the records in our PostgreSQL database need to be
synchronized to the changes made in the HPSS DB2 database.
The current database schema of the HPSS DB2 database does not provide a simple
way to achieve this, but we need to execute three separate queries for
1) newly created files (`scripts/hpss_new_inserts_query.ksh`),
2) deleted files (`scripts/hpss_updates_query.ksh`),
and 3) updated files (`scripts/hpss_deletes_query.ksh`).

Each query script generates the file list in a csv file, which can be used to
make appropriate changes into the PostgreSQL database. For instance, the
following code fragment shows the example to convert the output csv file into a
PostgreSQL SQL syntax.

```bash
## from scripts/hpss_updates_query.ksh
sed -i 's/^/INSERT into objectstat values\(/g' /path/to/new_updates.csv
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
            name = Excluded.name;/g' /path/to/new_updates.csv
```

#### TODO

* Some of the scripts have some hardcoded parameters and need to be updated to
  accept command line arguments instead.

### Pathname generation

TODO

## Tagging

## Metadata extraction


