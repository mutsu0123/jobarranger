AC_PREREQ([2.69])
AC_INIT([jobarranger], [6.0.9], [support@example.com])

AC_CONFIG_SRCDIR([src/jobarg_server/jarun/jarun.c])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects])

AM_CONDITIONAL([SERVER], [true])
AM_CONDITIONAL([AGENT],  [true])
AM_CONDITIONAL([WITH_ODBC], [false])
AM_CONDITIONAL([JABBER], [false])
AM_CONDITIONAL([PROXY], [false])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
PKG_PROG_PKG_CONFIG

# Basic libraries
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([clock_gettime], [rt])

# PostgreSQL detection (required - JobArranger is PostgreSQL-only)
# Search for pg_config in PATH and RHEL/AlmaLinux non-standard locations.
PG_CONFIG="no"
AC_PATH_PROG([PG_CONFIG], [pg_config], [no],
    [$PATH:/usr/pgsql-16/bin:/usr/pgsql-15/bin:/usr/pgsql-14/bin:/usr/pgsql-13/bin:/usr/pgsql-12/bin])
if test "$PG_CONFIG" = "no"; then
    AC_MSG_ERROR([pg_config not found. Install PostgreSQL development package (libpq-dev / postgresql-devel) and ensure pg_config is in PATH.])
fi
AC_MSG_CHECKING([PostgreSQL version])
POSTGRESQL_VERSION=`$PG_CONFIG --version`
AC_MSG_RESULT([$POSTGRESQL_VERSION])
POSTGRESQL_CPPFLAGS="-I`$PG_CONFIG --includedir`"
POSTGRESQL_LDFLAGS="-L`$PG_CONFIG --libdir`"
POSTGRESQL_LIBS="-lpq"
AC_SUBST([POSTGRESQL_CPPFLAGS])
AC_SUBST([POSTGRESQL_LDFLAGS])
AC_SUBST([POSTGRESQL_LIBS])
AC_SUBST([POSTGRESQL_VERSION])
DB_CPPFLAGS="$POSTGRESQL_CPPFLAGS"
DB_LDFLAGS="$POSTGRESQL_LDFLAGS"
DB_LIBS="$POSTGRESQL_LIBS"
AC_SUBST([DB_CPPFLAGS])
AC_SUBST([DB_LDFLAGS])
AC_SUBST([DB_LIBS])
AC_DEFINE([HAVE_POSTGRESQL], [1], [Define to 1 if PostgreSQL is available])
# PQserverVersion is available since PostgreSQL 7.4
AC_DEFINE([HAVE_FUNCTION_PQSERVERVERSION], [1], [Define to 1 if PQserverVersion exists])
# iconv detection (required - jastr.c uses iconv for encoding conversion)
# On Linux glibc iconv is part of libc; on other platforms a separate libiconv
# may be needed. LIBICONV_CHECK_CONFIG defines HAVE_ICONV in config.h so that
# sysinc.h can conditionally include <iconv.h>.
found_iconv="no"
LIBICONV_CHECK_CONFIG([yes])
if test "x$found_iconv" != "xyes"; then
    AC_MSG_ERROR([iconv not found. Install libiconv-devel or libc-devel.])
fi

# libtar detection - prefer system libtar-dev; fall back to bundled build
AC_ARG_WITH([bundled-libtar],
    [AS_HELP_STRING([--with-bundled-libtar], [use bundled libtar (auto if system libtar not found)])],
    [with_bundled_libtar=$withval], [with_bundled_libtar=auto])

LIBTAR_CPPFLAGS=""
LIBTAR_LDFLAGS=""
LIBTAR_LIBS="-ltar"
found_libtar="no"

if test "$with_bundled_libtar" != "yes"; then
    AC_CHECK_HEADER([libtar.h], [
        AC_CHECK_LIB([tar], [tar_open], [found_libtar="system"])
    ])
fi

if test "$found_libtar" = "no"; then
    BUNDLED_LIBTAR_DIR="$(cd "$srcdir/libtar-master-6d0ab4c/build" 2>/dev/null && pwd)"
    if test -n "$BUNDLED_LIBTAR_DIR" && test -f "$BUNDLED_LIBTAR_DIR/lib/libtar.a"; then
        found_libtar="bundled"
        LIBTAR_CPPFLAGS="-I$BUNDLED_LIBTAR_DIR/include"
        LIBTAR_LDFLAGS="-L$BUNDLED_LIBTAR_DIR/lib"
        AC_MSG_RESULT([using bundled libtar from $BUNDLED_LIBTAR_DIR])
    else
        AC_MSG_ERROR([libtar not found. Run: sh libtar-master-6d0ab4c/build_libtar.sh])
    fi
fi

AC_SUBST([LIBTAR_CPPFLAGS])
AC_SUBST([LIBTAR_LDFLAGS])
AC_SUBST([LIBTAR_LIBS])

# json-c detection (required by jobarg_release and others that use <json.h>)
PKG_CHECK_MODULES([JSONC], [json-c], [], [
    AC_MSG_ERROR([json-c not found. Install json-c-devel (RHEL/AlmaLinux) or libjson-c-dev (Debian/Ubuntu).])
])

# Append to global compiler/linker flags so every Makefile.am picks them up
# without needing individual AM_CPPFLAGS changes (PostgreSQL is required everywhere)
CPPFLAGS="$CPPFLAGS $POSTGRESQL_CPPFLAGS $ICONV_CFLAGS $LIBTAR_CPPFLAGS $JSONC_CFLAGS"
LDFLAGS="$LDFLAGS $POSTGRESQL_LDFLAGS $ICONV_LDFLAGS $LIBTAR_LDFLAGS"
LIBS="$LIBS $POSTGRESQL_LIBS $ICONV_LIBS $LIBTAR_LIBS $JSONC_LIBS"

# Linux only
ARCH="linux"
AC_DEFINE_UNQUOTED([ARCH], "linux", [Define OS name])
AC_SUBST(ARCH)

# Linux sysinfo checks
AC_CHECK_HEADERS([sys/sysinfo.h sys/time.h sys/resource.h])
AC_CHECK_FUNCS([sysinfo getloadavg])

# Network and POSIX system headers required by sysinc.h (Zabbix 1.x design).
# sysinc.h uses #ifdef HAVE_xxx guards; these checks populate config.h so
# the correct headers are included on each platform. Split into groups to
# avoid autoconf 2.69 multi-line bracket parsing issues.
AC_CHECK_HEADERS([netdb.h netinet/in.h sys/socket.h arpa/inet.h arpa/nameser.h])
AC_CHECK_HEADERS([errno.h fcntl.h assert.h pthread.h signal.h pwd.h grp.h])
AC_CHECK_HEADERS([syslog.h sys/ipc.h sys/sem.h sys/shm.h sys/wait.h sys/file.h])
AC_CHECK_HEADERS([sys/syscall.h sys/stat.h sys/param.h sys/times.h sys/sysmacros.h sys/ucontext.h])
AC_CHECK_HEADERS([sys/statvfs.h sys/vfs.h sys/mount.h sys/swap.h sys/sysctl.h sys/timeb.h sys/loadavg.h])
AC_CHECK_HEADERS([sys/mnttab.h sys/dkstat.h sys/disk.h sys/dk.h sys/user.h sys/procfs.h sys/proc.h])
AC_CHECK_HEADERS([sys/pstat.h sys/var.h sys/vmmeter.h resolv.h nlist.h net/if_mib.h kvm.h kstat.h])
AC_CHECK_HEADERS([mntent.h net/if.h time.h math.h regex.h dirent.h])
AC_CHECK_HEADERS([ctype.h stdarg.h stdio.h stdlib.h string.h strings.h inttypes.h stdint.h unistd.h])

AC_MSG_CHECKING(for /proc filesystem)
if test -d /proc; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_PROC,1,[Define to 1 if '/proc' exists])
else
    AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for file /proc/stat)
if test -r /proc/stat; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_PROC_STAT,1,[Define to 1 if '/proc/stat' exists])
else
    AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for file /proc/cpuinfo)
if test -r /proc/cpuinfo; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_PROC_CPUINFO,1,[Define to 1 if '/proc/cpuinfo' exists])
else
    AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for file /proc/loadavg)
if test -r /proc/loadavg; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_PROC_LOADAVG,1,[Define to 1 if '/proc/loadavg' exists])
else
    AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for file /proc/net/dev)
if test -r /proc/net/dev; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_PROC_NET_DEV,1,[Define to 1 if '/proc/net/dev' exists])
else
    AC_MSG_RESULT(no)
fi

# Output files
AC_CONFIG_FILES([
Makefile
jaconf/Makefile
jaconf/alert/Makefile
jaconf/extendedjob/Makefile
jaconf/locale/Makefile
jaconf/monitor/Makefile
man/Makefile
src/Makefile
src/jalibs/Makefile
src/jobarg_agent/Makefile
src/jobarg_command/Makefile
src/jobarg_exec/Makefile
src/jobarg_get/Makefile
src/jobarg_joblogput/Makefile
src/jobarg_monitor/Makefile
src/jobarg_release/Makefile
src/jobarg_session/Makefile
src/jobarg_server/Makefile
src/jobarg_server/jaabort/Makefile
src/jobarg_server/jaboot/Makefile
src/jobarg_server/jajob/Makefile
src/jobarg_server/jajobnet/Makefile
src/jobarg_server/jaloader/Makefile
src/jobarg_server/jamsgsnd/Makefile
src/jobarg_server/japurge/Makefile
src/jobarg_server/jarun/Makefile
src/jobarg_server/jaselfmon/Makefile
src/jobarg_server/jatrapper/Makefile
src/libs/Makefile
src/libs/zbxalgo/Makefile
src/libs/zbxcommon/Makefile
src/libs/zbxcomms/Makefile
src/libs/zbxcommshigh/Makefile
src/libs/zbxconf/Makefile
src/libs/zbxcrypto/Makefile
src/libs/zbxdb/Makefile
src/libs/zbxdbcache/Makefile
src/libs/zbxdbhigh/Makefile
src/libs/zbxexec/Makefile
src/libs/zbxicmpping/Makefile
src/libs/zbxjson/Makefile
src/libs/zbxlog/Makefile
src/libs/zbxmedia/Makefile
src/libs/zbxmemory/Makefile
src/libs/zbxnix/Makefile
src/libs/zbxself/Makefile
src/libs/zbxserver/Makefile
src/libs/zbxsys/Makefile
src/libs/zbxsysinfo/Makefile
src/libs/zbxsysinfo/agent/Makefile
src/libs/zbxsysinfo/common/Makefile
src/libs/zbxsysinfo/simple/Makefile
src/libs/zbxsysinfo/linux/Makefile
src/libs/zbxsysinfo/unknown/Makefile
])

SERVER_LIBS=""
SERVER_LDFLAGS=""
AGENT_LIBS=""
AGENT_LDFLAGS=""
AC_SUBST([SERVER_LIBS])
AC_SUBST([SERVER_LDFLAGS])
AC_SUBST([AGENT_LIBS])
AC_SUBST([AGENT_LDFLAGS])

AC_OUTPUT
