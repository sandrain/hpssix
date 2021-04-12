lib_LTLIBRARIES = libhpssix.la

libhpssixdir = $(includedir)/hpssix

libhpssix_HEADERS = hpssix.h \
                    hpssix-config.h \
                    hpssix-tmpdb.h \
                    hpssix-mdb.h \
                    hpssix-db.h \
                    hpssix-workdata.h \
                    hpssix-utils.h

libhpssix_la_SOURCES = hpssix.c \
                       hpssix-config.c \
                       hpssix-tmpdb.c \
                       hpssix-mdb.c \
                       hpssix-db.c \
                       hpssix-db-schema.c \
                       hpssix-workdata.c \
                       hpssix-utils.c

libhpssix_la_LDFLAGS = -version-info $(LIBHPSSIX_LT_VERSION)

libhpssix_la_CPPFLAGS = $(LIBPQ_CFLAGS) $(LIBCONFIG_CFLAGS) $(SQLITE3_CFLAGS)
libhpssix_la_LIBADD = $(LIBPQ_LIBS) $(LIBCONFIG_LIBS) $(SQLITE3_LIBS)

libhpssix_la_CPPFLAGS += -DCONFDIR=\"$(sysconfdir)/hpssix\" \
                         -DLOCALSTATEDIR=\"$(localstatedir)/hpssix\"

hpssix-db-schema.c: hpssix-db-schema.sql
	@(echo "const char *hpssix_db_schema_sqlstr = "; \
	       sed 's/^/"/; s/$$/\\n"/' < $< ; \
	       echo ";" ) > $@

CLEANFILES = libhpssix.la hpssix-db-schema.c
