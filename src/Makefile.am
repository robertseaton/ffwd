lib_LTLIBRARIES = libqueue.la libao.la libvo.la
libqueue_la_SOURCES = queue.c
libao_la_SOURCES = ao/alsa.c ao/ao.c
libvo_la_SOURCES = vo/vo.c vo/x11.c

bin_PROGRAMS = ffwd
ffwd_SOURCES = main.c util.c kbd/kbd.c kbd/kbd_x11.c ffwd.c
ffwd_LDADD = libqueue.la libao.la libvo.la $(DEPS_LIBS)
