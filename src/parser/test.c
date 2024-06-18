#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "gramgram.h"

int main(int argc, char **argv) {
    Token token;
    trashScanner ts;
    trashYYSTYPE *yylval;
    void *parser;


    const char *arg;

    ts = begin_scan(arg);

    parser = ParseAlloc( malloc );

    int lexCode;
    do {
        lexCode = yylex(yylval, ts, token);
        Parse(parser, lexCode, yyget_text(ts));
    } while (lexCode > 0);

    if (lexCode == -1) {
        fprintf(stderr, "The scanner encountered an error.\n");
    }

    end_scan(ts);
    ParseFree(parser, free);
    return 0;
}