sbin_PROGRAMS = hpssixd

noinst_HEADERS = hpssixd.h hpssixd-comm.h

hpssixd_SOURCES = hpssixd.c \
                  hpssixd-comm.c \
                  hpssixd-daemon.c \
                  hpssixd-scanner.c \
                  hpssixd-builder.c \
                  hpssixd-extractor.c

AM_CFLAGS = $(LIBCONFIG_CFLAGS) $(LIBPQ_CFLAGS) $(SQLITE3_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/libhpssix/src -DRPC_SVC_FG \
              -D_BSD_SOURCE -DLIBEXECDIR=\"$(pkglibexecdir)\"

AM_LDFLAGS = $(LIBCONFIG_LIBS) $(LIBPQ_LIBS) $(SQLITE3_LIBS)
AM_LDFLAGS += $(top_builddir)/libhpssix/src/libhpssix.la
AM_LDFLAGS += -pthread -lnsl -rdynamic

CLEANFILES = $(sbin_PROGRAMS)
