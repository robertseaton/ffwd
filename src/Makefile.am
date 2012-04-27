AM_CFLAGS = -I/usr/include/ffmpeg -I/usr/local/include/ffmpeg
lib_LTLIBRARIES = libqueue.la libao.la libvo.la libclock.la
libqueue_la_SOURCES = queue.c
libao_la_SOURCES = ao/alsa.c ao/ao.c
libvo_la_SOURCES = vo/vo.c vo/x11.c
libclock_la_SOURCES = clock.c

bin_PROGRAMS = ffwd
ffwd_SOURCES = main.c util.c kbd/kbd.c kbd/kbd_x11.c ffwd.c
ffwd_LDADD = $(lib_LTLIBRARIES) $(DEPS_LIBS)
