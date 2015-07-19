#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mumpsimus], [0.2], [hisso@cpan.org])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/noop.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile src/Makefile unit-tests/Makefile system-tests/Makefile])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror])

# Checks for programs.
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CC_C99
AX_CFLAGS_WARN_ALL
AC_PROG_LN_S

# Check for packages
AM_PATH_CHECK([0.9.4], [have_check="yes"], 
	       AC_MSG_WARN([Check not present. No unit tests will be run.])
	       [have_check="no"])
AM_CONDITIONAL([HAVE_CHECK], [test x$have_check = "xyes"])
LIBCURL_CHECK_CONFIG([yes], [7.37.1])

# Checks for header files. 
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/syslimits.h])
AC_CHECK_HEADERS([linux/limits.h])

# Checks for libraries.
#AC_SEARCH_LIBS([ceil], [m])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strlcat])

AC_OUTPUT
