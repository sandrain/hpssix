dnl
dnl Please refer to COPYING for the license.
dnl

AC_PREREQ([2.69])
AC_INIT([hpssix], [1.0.0], [simh@ornl.gov])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign tar-pax subdir-objects])
AM_SILENT_RULES([yes])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h syslog.h unistd.h])
AC_CHECK_HEADERS([limits.h mntent.h sys/time.h utime.h])
AC_CHECK_HEADERS([attr/xattr.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_C_INLINE
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_CHECK_FUNCS([memset strchr strdup strerror strrchr strtoul strtoull])
AC_CHECK_FUNCS([getmntent gettimeofday realpath utime ftruncate])

AC_CONFIG_FILES([Makefile
                 extras/Makefile
                 hpssixd/Makefile
                 hpssixd/src/Makefile
                 libhpssix/Makefile
                 libhpssix/src/Makefile
                 libhpssix.pc
                 tests/Makefile
                 tests/src/Makefile
                 tools/Makefile
                 tools/builder/Makefile
                 tools/builder/src/Makefile
                 tools/extractor/Makefile
                 tools/extractor/src/Makefile
                 tools/scanner/Makefile
                 tools/scanner/scripts/Makefile
                 utils/Makefile
                 utils/hpssix-admin/Makefile
                 utils/hpssix-admin/src/Makefile
                 utils/hpssix-search/Makefile
                 utils/hpssix-search/src/Makefile
                 utils/hpssix-meta/Makefile
                 utils/hpssix-meta/src/Makefile
                 utils/hpssix-tag/Makefile
                 utils/hpssix-tag/src/Makefile])

# Checks for libraries.
AC_ARG_WITH(pkgconfigdir,
            [  --with-pkgconfigdir=DIR      pkgconfig file in DIR @<:@LIBDIR/pkgconfig@:>@],
            [pkgconfigdir=$withval],
            [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST(pkgconfigdir)

PKG_CHECK_MODULES([LIBCONFIG], [libconfig], ,
                  AC_MSG_ERROR([libconfig is required to build hpssix.]))

PKG_CHECK_MODULES([SQLITE3], [sqlite3], ,
                  AC_MSG_ERROR([sqlite3 is required to build hpssix.]))

PKG_CHECK_MODULES([LIBPQ], [libpq], ,
                  AC_MSG_ERROR([libpq is required to build hpssix.]))

PKG_CHECK_MODULES([CURL], [libcurl], ,
                  AC_MSG_ERROR([cURL is required to build hpssix.]))

PKG_CHECK_MODULES([LIBXML], [libxml-2.0], ,
                  AC_MSG_ERROR([libxml2 is required to build hpssix.]))

HPSSIX_VERSION=${PACKAGE_VERSION}
AC_SUBST(HPSSIX_VERSION)
AC_SUBST([LIBHPSSIX_LT_VERSION], [1:0:0])

LIBHPSSIX_API_VERSION="1.0.0"
AC_SUBST(LIBHPSSIX_API_VERSION)

AC_OUTPUT
