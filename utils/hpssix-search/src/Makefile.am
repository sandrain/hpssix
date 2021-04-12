bin_PROGRAMS = hpssix-search

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src -D_GNU_SOURCE \
              $(LIBPQ_CFLAGS) $(LIBCONFIG_CFLAGS) $(SQLITE3_CFLAGS)

AM_LDFLAGS = $(top_builddir)/libhpssix/src/libhpssix.la -pthread \
             $(LIBPQ_LIBS) $(LIBCONFIG_LIBS) $(SQLITE3_LIBS)

noinst_HEADERS = hpssix-search.h

hpssix_search_SOURCES = hpssix-search.c \
                        hpssix-search-fattr.c \
                        hpssix-search-tag.c \
                        hpssix-search-tsv.c

CLEANFILES = $(bin_PROGRAMS)
