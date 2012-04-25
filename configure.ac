#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([ffwd], [0.1], [https://github.com/robertseaton/ffwd/issues])
AC_CONFIG_SRCDIR([src/ffwd.c])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [have_check=yes], [have_check=no])
PKG_CHECK_MODULES([DEPS], [alsa x11 libavcodec libavformat libavutil libswscale])
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([stdlib.h string.h sys/timeb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT64_T

# pretty print build
AM_SILENT_RULES([yes])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([ftime memset strdup])
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_OUTPUT
