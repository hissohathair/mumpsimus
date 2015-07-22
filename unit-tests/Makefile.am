TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
		  $(top_srcdir)/build-aux/tap-driver.sh

AM_CFLAGS = @CHECK_CFLAGS@

TESTS = 
check_PROGRAMS =
if HAVE_CHECK
TESTS += check_stream_buffer check_util check_pipes
check_PROGRAMS += check_stream_buffer check_util check_pipes

check_stream_buffer_SOURCES = check_stream_buffer.c ../src/stream_buffer.h ../src/stream_buffer.c \
	../src/ulog.c ../src/util.c \
	../src/ulog.h ../src/util.h
check_stream_buffer_LDADD = @CHECK_LIBS@

check_util_SOURCES = check_util.c ../src/ulog.c ../src/util.c
check_util_LDADD = @CHECK_LIBS@ 

check_pipes_SOURCES = check_pipes.c ../src/pipes.c ../src/pipes.h \
	../src/ulog.c ../src/util.c \
	../src/ulog.h ../src/util.h
check_pipes_LDADD = @CHECK_LIBS@
endif

noinst_PROGRAMS = $(TESTS)
