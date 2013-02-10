AM_CFLAGS = -std=gnu99

bin_PROGRAMS = noop upcase null log pipetest 

log_SOURCES = log.c http_parser.c http_parser.h
noop_SOURCES = noop.c 
null_SOURCES = noop.c
upcase_SOURCES = upcase.c
pipetest_SOURCES = pipetest.c


# for "make test"
test: $(bin_PROGRAMS)
	(cd .. && $(PROVE) t/*.t)
