#ifndef LOGGER_H
#define LOGGER_H

#define LOG_ERROR TRASH_ERROR
#define LOGGED TRASH_SUCCESS

#define LOG(level, message) do { \
    int rc; \
    rc = log(__FILE__, __LINE__, message, level); \
    assert(rc == 0); \
} while(0)

typedef enum {
    INFO,
    DEBUG,
    WARN,
    ERROR
} LEVEL;

typedef struct Logger Logger;

void init_logger();
void close_logger();

int log(const char *file, int line, const char *message, LEVEL level);

#endif //LOGGER_H