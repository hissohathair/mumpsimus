AM_CFLAGS = -std=gnu99

bin_PROGRAMS = noop null log

log_SOURCES = log.c http_parser.c http_parser.h util.c util.h ulog.c ulog.h
noop_SOURCES = noop.c util.c util.h
null_SOURCES = noop.c util.c util.h

# for "make test"
test: $(bin_PROGRAMS)
	(cd .. && $(PROVE) t/*.t)
