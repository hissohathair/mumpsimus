AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = noop log headers body connect$(EXEEXT)
bin_SCRIPTS = null$(EXEEXT) dup$(EXEEXT)
CLEANFILES = $(bin_SCRIPTS)

log_SOURCES = log.c http_parser.c http_parser.h util.c util.h ulog.c ulog.h
noop_SOURCES = noop.c util.c util.h ulog.h ulog.c
headers_SOURCES = headers.c pipes.h pipes.c util.h util.c ulog.h ulog.c http_parser.h http_parser.c stream_buffer.h stream_buffer.c
body_SOURCES = body.c pipes.h pipes.c util.h util.c ulog.h ulog.c http_parser.h http_parser.c stream_buffer.h stream_buffer.c
connect_SOURCES = connect.c http_parser.h http_parser.c ulog.h ulog.c
connect_LDFLAGS = $(LIBCURL)

null$(EXEEXT): noop$(EXEEXT)
	$(RM) -f null$(EXEEXT)
	$(LN_S) noop$(EXEEXT) null$(EXEEXT)

dup$(EXEEXT): noop$(EXEEXT)
	$(RM) -f dup$(EXEEXT)
	$(LN_S) noop$(EXEEXT) dup$(EXEEXT)

clean-local:
	$(RM) -f null$(EXEEXT) dup$(EXEEXT)

indent:
	 indent -gnu -npcs *.c *.h
