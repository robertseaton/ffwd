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
AM_PATH_CHECK

# Checks for libraries.
AC_CHECK_LIB([X11], [XSendEvent])
AC_CHECK_LIB([asound], [snd_pcm_open])
AC_CHECK_LIB([avcodec], [avcodec_decode_video2])
AC_CHECK_LIB([avformat], [avformat_open_input])
AC_CHECK_LIB([avutil], [av_dict_get])
AC_CHECK_LIB([swscale], [sws_getCachedContext])
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
