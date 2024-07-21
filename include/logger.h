#ifndef LOGGER_H
#define LOGGER_H

#define LOG_ERROR TRASH_ERROR
#define LOGGED TRASH_SUCCESS

/**
 * @todo    add additional args so we can format the message
 */
#define TRASH_LOG(level, message) do { \
    int rc; \
    rc = trash_log(level, __FILE__, __LINE__, message); \
    assert(rc == 0); \
} while(0)

typedef enum {
    INFO,
    DEBUG,
    WARN,
    ERROR
} LEVEL;

typedef struct Logger Logger;

int init_logger();
void close_logger();

int trash_log(LEVEL level, const char *file, int line, const char *message);

#endif //LOGGER_H