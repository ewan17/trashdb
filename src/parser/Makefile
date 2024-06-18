CC  = gcc
W = -W -Wall -g
INCLUDE = ../include ../include/parser
CFLAGS = $(W) $(addprefix -I, $(INCLUDE))

TESTS = parsertest

FLEX_FILE = $(wildcard *.l)
FLEX_BASE = $(basename $(FLEX_FILE))
FLEX_C = $(FLEX_BASE).c
FLEX_H = $(FLEX_BASE).h

GRAM_FILE = $(wildcard *.y)
GRAM_BASE = $(basename $(GRAM_FILE))
GRAM_C = $(GRAM_BASE).c
GRAM_H = $(GRAM_BASE).h

FLEX_TARGET = $(FLEX_C) $(FLEX_H)
PARSER_TARGET = lemon $(GRAM_C) $(GRAM_H)

TARGETS = $(FLEX_TARGET) $(PARSER_TARGET) 

.PHONY: all clean

clean:
	rm -rf $(TESTS) $(TARGETS) *.tab.[ch] *.[ao] *.out

parsertest:	grammar.o lex.o test.o 
	$(CC) $(CFLAGS) -o $@ $^

test.o: test.c
	$(CC) $(CFLAGS) -c $<

grammar.o: $(GRAM_C) $(GRAM_H)
	$(CC) $(CFLAGS) -c $<

$(GRAM_C) $(GRAM_H): lemon
	./lemon $(GRAM_FILE)

lex.o: $(FLEX_C) $(FLEX_H)
	$(CC) $(CFLAGS) -c $<

$(FLEX_C) $(FLEX_H): $(FLEX_FILE)
	flex --outfile=$(FLEX_C) --header-file=$(FLEX_H) $(FLEX_FILE)

lemon: lemon.c
	$(CC) -o $@ $<