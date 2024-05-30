CC	= gcc
W = -W -Wall -g
INCLUDE = ../include ../include/trashdb
CFLAGS = $(W) $(addprefix -I, $(INCLUDE)) 
LDFLAGS = -llmdb

BUILDDIR = ../../build

TARGETS = envx

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))

clean:
	rm -rf $(BUILDDIR) $(TARGETS) 

envx: utilities.o envtest.o env.o
	$(CC) $(CFLAGS) $(addprefix $(BUILDDIR)/,$^) -o $@ $(LDFLAGS)

all: $(OBJS)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $(BUILDDIR)/$@ $<

%.o: ../utils/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $(BUILDDIR)/$@ $<