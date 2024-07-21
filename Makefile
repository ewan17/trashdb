CC	= gcc
W = -W -Wall -g
INCLUDE = include
CFLAGS = $(W) $(addprefix -I, $(INCLUDE)) -DTEST
DCFLAGS = $(CFLAGS) 
LDFLAGS = -llmdb

TARGETS = test/env
TESTS = dbtest

.PHONY: all clean

clean:
	rm -rf $(TARGETS) $(TESTS) *.[ao] *.[ls]o

dbtest: utilities.o db.o dbtest.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: ./src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: ./test/%.c
	$(CC) $(CFLAGS) -c -o $@ $<