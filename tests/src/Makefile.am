noinst_PROGRAMS = test-config \
                  test-db \
                  test-mdb \
                  test-workdata \
                  test-workdata-dispatch \
                  test-extfilter \
                  test-psql \
                  test-tika \
                  test-tika-extractor

noinst_HEADERS = testlib.h

AM_CFLAGS = $(LIBCONFIG_CFLAGS) $(LIBPQ_CFLAGS) $(CURL_CFLAGS) $(SQLITE3_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src

AM_LDFLAGS = $(LIBCONFIG_LIBS) $(LIBPQ_LIBS) $(CURL_LIBS) $(SQLITE3_LIBS)
AM_LDFLAGS += $(top_builddir)/libhpssix/src/libhpssix.la -pthread

test_config_SOURCES = test-config.c testlib.c

test_db_SOURCES = test-db.c testlib.c

test_mdb_SOURCES = test-mdb.c testlib.c

test_workdata_SOURCES = test-workdata.c testlib.c

test_workdata_dispatch_SOURCES = test-workdata-dispatch.c testlib.c

test_extfilter_SOURCES = test-extfilter.c testlib.c

test_psql_SOURCES = test-psql.c testlib.c

test_tika_SOURCES = test-tika.c testlib.c

test_tika_extractor_SOURCES = test-tika-extractor.c testlib.c

CLEANFILES = $(noinst_PROGRAMS)
