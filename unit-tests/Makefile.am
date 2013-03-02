TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
		  $(top_srcdir)/build-aux/tap-driver.sh

AM_CFLAGS = @CHECK_CFLAGS@

TESTS = 
check_PROGRAMS =
if HAVE_CHECK
TESTS += check_stream_buffer
check_PROGRAMS += check_stream_buffer
check_stream_buffer_SOURCES = check_stream_buffer.c $(top_builddir)/src/stream_buffer.h \
	$(top_builddir)/src/stream_buffer.c $(top_builddir)/src/ulog.c $(top_builddir)/src/util.c
check_stream_buffer_LDADD = @CHECK_LIBS@
endif

noinst_PROGRAMS = $(TESTS)
