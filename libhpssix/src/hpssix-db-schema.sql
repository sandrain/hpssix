--
-- hpssix main database schema (postgresql)
--

BEGIN TRANSACTION;

DROP TABLE IF EXISTS hpssix_object cascade;
DROP TRIGGER IF EXISTS hpssix_file_name_update ON hpssix_file;
DROP FUNCTION IF EXISTS hpssix_file_get_name;
DROP TABLE IF EXISTS hpssix_file cascade;
DROP TABLE IF EXISTS hpssix_attr_key cascade;
DROP FUNCTION IF EXISTS hpssix_attr_key_pop;
DROP TRIGGER IF EXISTS hpssix_attr_val_update ON hpssix_attr_val;
DROP FUNCTION IF EXISTS hpssix_attr_parse_numeric;
DROP TABLE IF EXISTS hpssix_attr_val cascade;
DROP TRIGGER IF EXISTS hpssix_attr_document_tsv_update ON hpssix_attr_document;
DROP FUNCTION IF EXISTS documents_search_trigger;
DROP TABLE IF EXISTS hpssix_attr_document cascade;

CREATE TABLE hpssix_object (
    oid BIGINT NOT NULL,        -- object_id in HPSS
    valid BOOL NOT NULL DEFAULT 't',
    -- st_ino is identical to the oid
    st_dev INTEGER NOT NULL,
    st_mode INTEGER NOT NULL,
    st_nlink INTEGER NOT NULL,
    st_uid INTEGER NOT NULL,
    st_gid INTEGER NOT NULL,
    st_rdev INTEGER NOT NULL,
    st_size INTEGER NOT NULL,
    st_blksize INTEGER NOT NULL,
    st_blocks INTEGER NOT NULL,
    st_atime INTEGER NOT NULL,
    st_mtime INTEGER NOT NULL,
    st_ctime INTEGER NOT NULL,
    PRIMARY KEY (oid)
);

CREATE INDEX ix_hpssix_valid ON hpssix_object(valid);
CREATE INDEX ix_hpssix_mode ON hpssix_object(st_mode);
CREATE INDEX ix_hpssix_uid ON hpssix_object(st_uid);
CREATE INDEX ix_hpssix_gid ON hpssix_object(st_gid);
CREATE INDEX ix_hpssix_size ON hpssix_object(st_size);
CREATE INDEX ix_hpssix_atime ON hpssix_object(st_atime);
CREATE INDEX ix_hpssix_mtime ON hpssix_object(st_mtime);
CREATE INDEX ix_hpssix_ctime ON hpssix_object(st_ctime);

CREATE TABLE hpssix_file (
    fid BIGINT GENERATED ALWAYS AS IDENTITY,
    oid BIGINT NOT NULL REFERENCES hpssix_object(oid) ON DELETE CASCADE,
    path TEXT NOT NULL,         -- full path of the file
    name TEXT NOT NULL,         -- file name

    primary key (fid)
);

CREATE INDEX ix_hpssix_file_name ON hpssix_file (name);

CREATE FUNCTION hpssix_file_get_name() RETURNS TRIGGER AS $$
BEGIN
    new.name := substring(new.path from '[^/]+$');
    RETURN new;
END
$$ LANGUAGE plpgsql;

CREATE TRIGGER hpssix_file_name_update
    BEFORE INSERT OR UPDATE ON hpssix_file
    FOR EACH ROW EXECUTE PROCEDURE hpssix_file_get_name();

CREATE TABLE hpssix_attr_key (
    kid BIGINT GENERATED ALWAYS AS IDENTITY,
    name TEXT NOT NULL,

    PRIMARY KEY (kid),
    UNIQUE (name)
);

CREATE FUNCTION hpssix_attr_key_pop(_name text) RETURNS BIGINT AS $$
    DECLARE id BIGINT;
BEGIN
    SELECT kid INTO id FROM hpssix_attr_key WHERE name = _name;

    IF id IS NULL THEN
        INSERT INTO hpssix_attr_key(name) VALUES (_name) RETURNING kid INTO id;
    END IF;

    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE TABLE hpssix_attr_val (
    vid BIGINT GENERATED ALWAYS AS IDENTITY,
    oid BIGINT NOT NULL REFERENCES hpssix_object(oid) ON DELETE CASCADE,
    kid BIGINT NOT NULL REFERENCES hpssix_attr_key(kid) ON DELETE CASCADE,
    rval DOUBLE PRECISION,  -- numeric representaion if possible
    sval TEXT,

    PRIMARY KEY (vid),
    UNIQUE (oid, kid)
);

CREATE INDEX ix_hpssix_attr_rval ON hpssix_attr_val (kid, rval);
CREATE INDEX ix_hpssix_attr_sval ON hpssix_attr_val (kid, sval);

CREATE FUNCTION hpssix_attr_parse_numeric() RETURNS TRIGGER AS $$
    DECLARE _rval DOUBLE PRECISION DEFAULT NULL;
BEGIN
    _rval := CAST(new.sval AS DOUBLE PRECISION);
    new.rval := _rval;
    RETURN new;
EXCEPTION
    WHEN OTHERS THEN -- ignore: do not populate rval field
    RETURN new;
END
$$ LANGUAGE plpgsql;

CREATE TRIGGER hpssix_attr_val_update
    BEFORE INSERT OR UPDATE ON hpssix_attr_val
    FOR EACH ROW EXECUTE PROCEDURE hpssix_attr_parse_numeric();

--
-- currently, adopted the method from:
-- https://blog.lateral.io/2015/05/full-text-search-in-milliseconds-with-postgresql/
--
CREATE TABLE hpssix_attr_document (
    tid BIGINT GENERATED ALWAYS AS IDENTITY,
    oid BIGINT NOT NULL REFERENCES hpssix_object(oid) ON DELETE CASCADE,
    meta JSONB,
    text TEXT,
    tsv TSVECTOR,

    PRIMARY KEY (tid),
    UNIQUE (oid)
);

CREATE INDEX ix_tsv ON hpssix_attr_document USING GIN (tsv);

CREATE FUNCTION documents_search_trigger() RETURNS TRIGGER AS $$
BEGIN
    new.tsv := SETWEIGHT(TO_TSVECTOR(COALESCE(new.meta->>'title','')), 'A') ||
               SETWEIGHT(TO_TSVECTOR(COALESCE(new.text,'')), 'D');
    RETURN new;
END
$$ LANGUAGE plpgsql;

CREATE TRIGGER hpssix_attr_document_tsv_update
    BEFORE INSERT OR UPDATE ON hpssix_attr_document
    FOR EACH ROW EXECUTE PROCEDURE documents_search_trigger();

END TRANSACTION;

