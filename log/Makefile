CC	= gcc
W = -W -Wall
CFLAGS = $(W)
TESTS = testbuffer1
CURRENT_DIR = $(shell pwd)
PROGS = $(CURRENT_DIR)/progs


clean:
	rm -rf $(TESTS) *.[ao] *.[ls]o

# This needs to support all tests
test:	testbuffer1
# ./testbuffer1

testbuffer1:	testbuffer1.o	liblog.a

%:	%.o
	$(CC) $(CFLAGS) $^ -o $@

%.o:	%.c logger.h buffer.h
	$(CC) $(CFLAGS) -c $< 

liblog.a:	buffer.o logger.o
	$(AR) rs $@ buffer.o logger.o

buffer.o: buffer.c buffer.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c buffer.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c logger.c
