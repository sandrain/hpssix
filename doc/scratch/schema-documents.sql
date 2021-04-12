\connect hpssmddb;

drop table if exists documents;

create table documents (
  id integer not null default('documents_id_seq'::regclass),
  object_id bigint not null,
  tsv tsvector,
  primary key(id)
);

create index tsv_idx on documents using gin(tsv);
