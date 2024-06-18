%include {
    #include <stdio.h>
    #include <assert.h>
    #include "gramgram.h"
}

%token_prefix   TOKEN_
%token_type {Token}
%start_symbol prog

%syntax_error
{
    printf("Error parsing command\n");
}

prog ::= obj.

obj ::= LBRACE RBRACE.                  {
                                            printf("{}\n");
                                        }

obj ::= LBRACE data(D) RBRACE.          {
                                            printf("{%s}\n", D.s);
                                        }

data(D) ::= NUM.                      {
                                            D.s = "number";
                                        }

data(D) ::= STRING.                      {
                                            D.s = "string";
                                        }