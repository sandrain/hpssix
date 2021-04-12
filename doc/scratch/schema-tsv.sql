\connect hpssix

create table documents (
  id integer not null,
  tsv tsvector,
  primary key(id)
);

create index tsv_idx on documents using gin(tsv);
