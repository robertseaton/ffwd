lib_LTLIBRARIES = libqueue.la libao.la
libqueue_la_SOURCES = queue.c queue.h
libao_la_SOURCES = ao/alsa.c ao/ao.c ao/ao.h ao/ao_internal.h

bin_PROGRAMS = ffwd
ffwd_SOURCES = ffwd.c kbd/kbd.c kbd/kbd_x11.c vo/vo.c vo/x11.c util.c
ffwd_LDADD = libqueue.la libao.la