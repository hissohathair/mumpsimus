#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mumpsimus], [0.1], [hisso@cpan.org])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([src/noop.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile src/Makefile unit-tests/Makefile system-tests/Makefile])
AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AX_CFLAGS_WARN_ALL

# Check for packages
AM_PATH_CHECK([0.9.4], [], [AC_MSG_WARN([Check not present. No unit tests will be run.])])
AM_CONDITIONAL([HAVE_CHECK], [test x$CHECK_LIBS != "x"])
#PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

# Checks for libraries.
#AC_CHECK_LIB( [event], [event_enable_debug_mode] )
AC_SEARCH_LIBS([ceil], [m])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h sysexits.h])
#AC_CHECK_HEADERS([event2/event.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strlcat])

AC_OUTPUT
