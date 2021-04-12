pkglibexec_PROGRAMS = hpssix-extractor

noinst_HEADERS = hpssix-extractor.h

AM_CFLAGS = $(LIBCONFIG_CFLAGS) $(LIBPQ_CFLAGS) $(CURL_CFLAGS) $(SQLITE3_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src

AM_LDFLAGS = $(LIBCONFIG_LIBS) $(LIBPQ_LIBS) $(CURL_LIBS) $(SQLITE3_LIBS)
AM_LDFLAGS += $(top_builddir)/libhpssix/src/libhpssix.la -pthread

hpssix_extractor_SOURCES = hpssix-extractor.c \
			   hpssix-extractor-tika.c

CLEANFILES = $(libexec_PROGRAMS)
