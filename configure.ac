#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mumpsimus], [0.1], [hisso@cpan.org])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([src/noop.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile src/Makefile t/Makefile])
AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Checks for programs.
AC_PROG_CC
#AC_PROG_SED
#AC_PROG_AWK

# Checks for more programs :)
AC_ARG_VAR([PROVE], [Perl testsuite harness to use (prove or perl)])
AC_PATH_PROGS([PROVE], [prove perl])

# Checks for libraries.
#AC_CHECK_LIB(curl, curl_version) 
AC_CHECK_LIB( [event], [event_enable_debug_mode] )

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h sysexits.h])
AC_CHECK_HEADERS([event2/event.h], [], [AC_MSG_ERROR([Cannot find event2/event.h])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_OUTPUT
