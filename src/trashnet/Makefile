CC	= gcc
W = -W -Wall -g
CFLAGS = $(W)
LDFLAGS = -levent
TESTS = servertest clienttest

SRCS = server.c utilities.c buffy.c message.c protocol.c
OBJS = $(SRCS:.c=.o)

clean:
	rm -rf $(TESTS) *.[ao] *.[ls]o

test:	$(TESTS)

clienttest: testclient.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

servertest: testserver.o	nettwerk.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

nettwerk.a: $(OBJS)
	$(AR) rs $@ $^

%.o: %.c server.h
	$(CC) $(CFLAGS) -c $<