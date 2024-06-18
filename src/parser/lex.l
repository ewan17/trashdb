%{
    #include "scanner.h"
    #include "grammar.h"
    #include <stdio.h>

    #define YYSTYPE trashYYSTYPE

    #define YY_DECL int yylex(YYSTYPE * yylval, yyscan_t yyscanner)

    void yyerror(trashScanner ts, const char *message);
%}

%option noyywrap
%option reentrant
%option yylineno

%%
"{"                             return TOKEN_LBRACE;
"}"                             return TOKEN_RBRACE;

[a-zA-Z]                        {
                                    yylval->str = strdup(yytext);
                                    return TOKEN_STRING;
                                }

[0-9]+                          {
                                    yylval->ival = atoi(yytext);
                                    return TOKEN_NUM;
                                }
%%

void yyerror(trashScanner ts, const char *message) {
    printf("Error message: %s\n", message);
}

trashScanner begin_scan() {
    yyscan_t scanner;

    if(yylex_init(&scanner) != 0) {
        exit(1);
    }

    return scanner;
}

void end_scan(trashScanner ts) {
    yylex_destroy(ts);
}