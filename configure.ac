#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

# AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_INIT([attcp], [0.6.6], [bug-report@nowhere.to.send])
AC_CONFIG_SRCDIR([src/attcp_main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
## added by AIXTOOLS.net, but it never gets here - as
## AC_PROG_C has already failed (exit 77)
## as_fn_error 77 "C compiler cannot create executables
##
## ** If AIX, if gcc, if OBJECT_MODE=64
## then CC="${CC} -maix64"
## else CC="${CC} -maix32"
## fi fi fi
AC_PROG_INSTALL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lperfstat':
AC_CHECK_LIB([perfstat], [perfstat_cpu_total])
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
# AC_FUNC_MALLOC - giving problems on AIX with xlc
AC_CHECK_FUNCS([alarm bzero gethostbyname gettimeofday inet_ntoa select socket])

AC_CONFIG_FILES([Makefile
		 src/utils/Makefile
                 src/Makefile])
AC_OUTPUT
