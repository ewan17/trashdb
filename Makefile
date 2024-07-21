CC	= gcc
W = -W -Wall -g
INCLUDE = include
CFLAGS = $(W) $(addprefix -I, $(INCLUDE)) -DTEST
DCFLAGS = $(CFLAGS) 
LDFLAGS = -llmdb

TARGETS = test/env test/logs
TESTS = dbtest loggertest

.PHONY: all clean

clean:
	rm -rf $(TARGETS) $(TESTS) *.[ao] *.[ls]o

dbtest: utilities.o db.o dbtest.o
	$(CC) $(CFLAGS) -o $@ $^ -llmdb

loggertest: utilities.o logger.o loggertest.o

%.o: ./src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: ./test/%.c
	$(CC) $(CFLAGS) -c -o $@ $<