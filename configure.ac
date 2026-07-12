AC_PREREQ([2.69])
AC_INIT([jobarranger], [6.0.9], [support@example.com])

AC_CONFIG_SRCDIR([src/jobarg_server/jarun/jarun.c])
AC_CONFIG_HEADERS([include/config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])

dnl ==== legacy conditionals for existing Makefile.am ====
AM_CONDITIONAL([SERVER], [false])
AM_CONDITIONAL([AGENT],  [false])
AM_CONDITIONAL([WITH_ODBC], [false])
AM_CONDITIONAL([JABBER], [false])
AM_CONDITIONAL([PROXY], [false])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB

# Basic libraries
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([clock_gettime], [rt])

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
src/libs/zbxcomms/Makefile
src/libs/zbxdb/Makefile
src/libs/zbxdbcache/Makefile
src/libs/zbxdbhigh/Makefile
src/libs/zbxjson/Makefile
src/libs/zbxsysinfo/Makefile
src/libs/zbxsysinfo/agent/Makefile
src/libs/zbxsysinfo/common/Makefile
src/libs/zbxsysinfo/simple/Makefile
src/libs/zbxsysinfo/linux/Makefile
src/libs/zbxsysinfo/unknown/Makefile
])



AC_OUTPUT
