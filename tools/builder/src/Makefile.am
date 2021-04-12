pkglibexec_PROGRAMS = hpssix-builder

noinst_HEADERS = hpssix-builder.h

AM_CFLAGS = $(LIBCONFIG_CFLAGS) $(LIBPQ_CFLAGS) $(SQLITE3_CFLAGS) \
            $(LIBXML_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src

AM_LDFLAGS = $(LIBCONFIG_LIBS) $(LIBPQ_LIBS)  $(SQLITE3_LIBS) $(LIBXML_LIBS)
AM_LDFLAGS += $(top_builddir)/libhpssix/src/libhpssix.la -pthread

hpssix_builder_SOURCES = hpssix-builder.c \
                         hpssix-builder-utils.c \
                         hpssix-builder-file.c \
                         hpssix-builder-xattrs.c

CLEANFILES = $(libexec_PROGRAMS)
