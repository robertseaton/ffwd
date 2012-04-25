lib_LTLIBRARIES = libqueue.la libao.la libvo.la
libqueue_la_SOURCES = queue.c queue.h
libao_la_SOURCES = ao/alsa.c ao/ao.c ao/ao.h ao/ao_internal.h
libvo_la_SOURCES = vo/vo.c vo/x11.c vo/vo_internal.h vo/vo.h

bin_PROGRAMS = ffwd
ffwd_SOURCES = main.c util.c kbd/kbd.c kbd/kbd.h kbd/kdb_defs.h kbd/kbd_internal.h kbd/kbd_x11.c ffwd.c ffwd.h
ffwd_LDADD = libqueue.la libao.la libvo.la