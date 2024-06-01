CC	= gcc
W = -W -Wall -g
INCLUDE = ../include ../include/trashdb
CFLAGS = $(W) $(addprefix -I, $(INCLUDE))
LDFLAGS = -llmdb

BUILDDIR = ../../build

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))

TARGETS = testenvx

.PHONY: all clean

clean:
	rm -rf $(BUILDDIR) $(TARGETS) 

all: $(OBJS)

testenv = utilities.o envtest.o env.o
testenv: testenvx

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $(BUILDDIR)/$@ $<

%.o: ../utils/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $(BUILDDIR)/$@ $<

%x: W += -DDEBUG
.SECONDEXPANSION:
%x: $$($$*)
	@echo $(W)
	$(CC) $(CFLAGS) $(patsubst %.o,$(BUILDDIR)/%.o,$($*)) -o $@ $(LDFLAGS)