bin_PROGRAMS = hpssix-meta

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src -D_GNU_SOURCE \
              $(LIBPQ_CFLAGS) $(LIBCONFIG_CFLAGS) $(SQLITE3_CFLAGS)

AM_LDFLAGS = $(top_builddir)/libhpssix/src/libhpssix.la -pthread \
             $(LIBPQ_LIBS) $(LIBCONFIG_LIBS) $(SQLITE3_LIBS)

hpssix_meta_SOURCES = hpssix-meta.c

CLEANFILES = $(bin_PROGRAMS)
