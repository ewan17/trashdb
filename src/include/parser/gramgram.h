#ifndef GRAMGRAM_H
#define GRAMGRAM_H

typedef struct Token Token;

struct Token {
    const char *s;
    unsigned int sLen;
    int n;
};

#endif //GRAMGRAM_H