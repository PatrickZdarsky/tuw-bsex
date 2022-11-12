# fb_arc_set
# Author: Patrick Zdarsky (12123697)

CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pthread -pedantic $(DEFS)
LDFLAGS = -lpthread -lrt $(DEFS)

.PHONY: all clean
all: generator supervisor

generator: fbArcSetCommon.o generator.o
	$(CC) $(LDFLAGS) -o $@ $^

supervisor: fbArcSetCommon.o supervisor.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o generator supervisor