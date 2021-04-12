bin_PROGRAMS = hpssix-tag

noinst_HEADERS = hpssix-tag.h

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src -D_GNU_SOURCE \
              $(LIBPQ_CFLAGS) $(LIBCONFIG_CFLAGS) $(SQLITE3_CFLAGS)

AM_LDFLAGS = $(top_builddir)/libhpssix/src/libhpssix.la \
             $(LIBPQ_LIBS) $(LIBCONFIG_LIBS) $(SQLITE3_LIBS)

hpssix_tag_SOURCES = hpssix-tag.c \
                     hpssix-tag-xattr.c

CLEANFILES = $(bin_PROGRAMS)
