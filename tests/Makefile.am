TESTS = check_ffwd
check_PROGRAMS = check_ffwd
check_ffwd_SOURCES = check_ffwd.c check_queue.c check_alsa.c $(top_builddir)/src/queue.h tests.h
check_ffwd_CFLAGS = @CHECK_CFLAGS@
check_ffwd_LDADD = $(top_builddir)/src/libqueue.la $(top_builddir)/src/libao.la @CHECK_LIBS@ $(DEPS_LIBS)