AM_CFLAGS = -std=gnu99

bin_PROGRAMS = noop upcase null

noop_SOURCES = noop.c
null_SOURCES = noop.c
upcase_SOURCES = upcase.c


test: $(bin_PROGRAMS)
	(cd .. && prove t/*.t)
