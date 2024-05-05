#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"

typedef struct Testy {
    char const * name;
    char const * description;
    void *data;
} Testy;

Testy *generate_test_abstract(void *data, char const * name, char const * description) {
    Testy *testy;
    testy = (Testy *)malloc(sizeof(Testy));
    if(testy == NULL) {
        printf("Memory allocation error in test");
        /**
         * @todo    Handle differently
        */
        return NULL;
    }
    testy->data = data;
    testy->name = name;
    testy->description = description;

    return testy;
}

Buffer *generate_test_buffer() {
    Buffer *buffer;
    create_buffer(&buffer);
    return buffer;
}

typedef struct Append {
    Buffer *buffer;
    char const * data;
} Append;

void test_append_data() {   
    struct Testy testy[3];

    Buffer *buffer = generate_test_buffer();
    Append data = {.buffer=buffer, .data="test test"};
    testy[0] = *generate_test_abstract(&data, "case 1", "description");

    Buffer *buffer2 = generate_test_buffer();
    Append data2 = {.buffer=buffer2, .data="q test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test tesqw"};
    testy[1] = *generate_test_abstract(&data2, "case 2", "description");

    Buffer *buffer3 = generate_test_buffer();
    Append data3 = {.buffer=buffer3, .data="q test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test tesq"};
    testy[2] = *generate_test_abstract(&data3, "case 2", "description");

    for (size_t i = 0; i < 3; i++)
    {
        Append *append;
        append = (Append *)testy[i].data;
        append_data(append->buffer, append->data);

        printf("Buffer Data: %s\n", append->buffer->data);
    }
}

int main() {
    test_append_data();
    return 0;
}