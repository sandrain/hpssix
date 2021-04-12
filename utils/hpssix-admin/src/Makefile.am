sbin_PROGRAMS = hpssix-admin

noinst_HEADERS = hpssix-admin.h

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src -D_GNU_SOURCE \
              $(LIBPQ_CFLAGS) $(LIBCONFIG_CFLAGS) $(SQLITE3_CFLAGS)

AM_LDFLAGS = $(top_builddir)/libhpssix/src/libhpssix.la -pthread \
             $(LIBPQ_LIBS) $(LIBCONFIG_LIBS) $(SQLITE3_LIBS)

hpssix_admin_SOURCES = hpssix-admin.c \
                       hpssix-admin-history.c

CLEANFILES = $(sbin_PROGRAMS)
