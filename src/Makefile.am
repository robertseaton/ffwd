lib_LTLIBRARIES = libqueue.la libao.la libvo.la libkbd.la
libqueue_la_SOURCES = queue.c queue.h
libao_la_SOURCES = ao/alsa.c ao/ao.c ao/ao.h ao/ao_internal.h
libvo_la_SOURCES = vo/vo.c vo/x11.c vo/vo_internal.h vo/vo.h
libkbd_la_SOURCES = kbd/kbd.c kbd/kbd.h kbd/kdb_defs.h kbd/kbd_internal.h kbd/kbd_x11.c

bin_PROGRAMS = ffwd
ffwd_SOURCES = ffwd.c util.c
ffwd_LDADD = libqueue.la libao.la libvo.la libkbd.la