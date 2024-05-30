CC	= gcc
W = -W -Wall -g
INCLUDE = ../include
CFLAGS = $(W) $(addprefix -I, $(INCLUDE)) 

clean:
	rm -rf *.[ao] *.[ls]o

utilities: utilities.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<