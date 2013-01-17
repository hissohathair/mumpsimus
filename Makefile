# credit: http://stackoverflow.com/questions/1484817/how-do-i-make-a-simple-makefile-gcc-unix

TARGETS = noop
DOUBLES = null
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall

.PHONY: clean all default

default: $(TARGETS) $(DOUBLES)
all: default

test: $(TARGETS) $(DOUBLES)
	prove t/*.t

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGETS) $(OBJECTS)

$(TARGETS): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

null: noop
	-ln noop null

clean:
	-rm -f *.o *~
	-rm -f $(TARGETS) $(DOUBLES)