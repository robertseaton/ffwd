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
something_missing=no
have_x11=no
have_alsa=no
have_avcodec=no
have_avformat=no
have_avutil=no
have_swscale=no
have_pthread=no
have_check=no

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [have_check=yes], [have_check=no])

AC_CHECK_LIB([X11], [XSendEvent], [have_x11=yes])
AC_CHECK_LIB([asound], [snd_pcm_open], [have_alsa=yes])
AC_CHECK_LIB([avcodec], [avcodec_decode_video2], [have_avcodec=yes])
AC_CHECK_LIB([avformat], [avformat_open_input], [have_avformat=yes])
AC_CHECK_LIB([avutil], [av_dict_get], [have_avutil=yes])
AC_CHECK_LIB([swscale], [sws_getCachedContext], [have_swscale=yes])
AC_CHECK_LIB([pthread], [pthread_create], [have_pthread=yes])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([stdlib.h string.h sys/timeb.h])
AC_CHECK_HEADERS([X11/Xlib.h X11/Xutil.h X11/keysym.h], [], [have_x11=no something_missing=yes])
AC_CHECK_HEADERS([alsa/asoundlib.h], [], [have_alsa=no something_missing=yes])
AC_CHECK_HEADERS([libavcodec/avcodec.h], [], [have_avcodec=no something_missing=yes])
AC_CHECK_HEADERS([libavformat/avformat.h], [], [have_avformat=no something_missing=yes])
AC_CHECK_HEADERS([libavutil/avutil.h], [], [have_avutil=no something_missing=yes])
AC_CHECK_HEADERS([libswscale/swscale.h], [], [have_swscale=no something_missing=yes])
AC_CHECK_HEADERS([pthread.h], [], [have_pthread=no something_missing=yes])

# Alert user that something is missing.
if test "${something_missing}" = yes; then
   echo "----------------------------------"
fi

if test "${have_x11}" = no; then
   echo "X11 is missing! Please install X11."
fi

if test "${have_alsa}" = no; then
   echo "ALSA is missing! Please install ALSA's userspace library."
fi

if test "${have_avcodec}" = no; then
   echo "libavcodec is missing! Please install ffmpeg."
fi

if test "${have_avformat}" = no; then
   echo "libavformat is missing! Please install ffmpeg."
fi

if test "${have_avutil}" = no; then
   echo "libavutil is missing! Please install ffmpeg."
fi

if test "${have_swscale}" = no; then    
   echo "libswscale is missing! Please install ffmpeg."
fi

if test "${have_pthread}" = no; then
   echo "Support for POSIX threads is missing!"
fi

if test "${something_missing}" = yes; then
   echo ""
   echo "You will need -dev packages if your distro splits packages."
   echo "----------------------------------"
   echo "Required libraries missing. Aborting..."
   (exit 1); exit 1;
fi

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
