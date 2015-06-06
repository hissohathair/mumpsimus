
bin_PROGRAMS = noop log headers
bin_SCRIPTS = null$(EXEEXT) dup$(EXEEXT)
CLEANFILES = $(bin_SCRIPTS)

log_SOURCES = log.c http_parser.c http_parser.h util.c util.h ulog.c ulog.h
noop_SOURCES = noop.c util.c util.h ulog.h ulog.c
headers_SOURCES = headers.c pipes.h pipes.c util.h util.c ulog.h ulog.c http_parser.h http_parser.c stream_buffer.h stream_buffer.c


null$(EXEEXT): noop$(EXEEXT)
	$(RM) -f null$(EXEEXT)
	$(LN_S) noop$(EXEEXT) null$(EXEEXT)

dup$(EXEEXT): noop$(EXEEXT)
	$(RM) -f dup$(EXEEXT)
	$(LN_S) noop$(EXEEXT) dup$(EXEEXT)

clean-local:
	$(RM) -f null$(EXEEXT) dup$(EXEEXT)
