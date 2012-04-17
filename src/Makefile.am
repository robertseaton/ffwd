lib_LTLIBRARIES = libqueue.la
libqueue_la_SOURCES = queue.c queue.h

bin_PROGRAMS = ffwd
ffwd_SOURCES = ffwd.c ao/alsa.c ao/ao.c kbd/kbd.c kbd/kbd_x11.c vo/vo.c vo/x11.c
ffwd_LDADD = libqueue.la