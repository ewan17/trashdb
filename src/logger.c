#include "global.h"

#define CLOSE_Q 0X01
#define OPEN_Q 0x02

#define TIME_LENGTH 26
#define LOG_FMT "[%s][%s][%s:%d][%s]\n"

static LL logs;

struct LogInfo {
    IL move;

    const char *message;
    const char *file;
    int line;

    LEVEL level;
};

static FILE *f;
static pthread_t logThread;

static pthread_mutex_t logMutex;
static pthread_cond_t logCond;
static unsigned int flags = 0;

int init_logger() {
    int rc;

    init_list(&logs);

    pthread_mutex_init(&logMutex, NULL);
    pthread_cond_init(&logCond, NULL);

    // read/write file
    f = fopen(logFile, "a+");
    assert(f != NULL);

    flags |= OPEN_Q;

    rc = pthread_create(&logThread, NULL, log_func, NULL);
    return rc;
}

void close_logger() {
    pthread_mutex_lock(&logMutex);
    // close the log q
    flags &= CLOSE_Q;
    // finish processing the logs
    pthread_cond_signal(&logCond);
    pthread_mutex_unlock(&logMutex);
}

/**
 * @todo    add a limit to the number of logs
 */
int log(const char *file, int line, const char *message, LEVEL level) {
    struct LogInfo *log;

    log = (struct LogInfo *)malloc(sizeof(*log));
    assert(log != NULL);

    init_il(&log->move);

    log->message = message;
    log->file = file;
    log->line = line;
    log->level = level;

    return LOGGED;
}

static void current_time(char *timestamp) {
    time_t now;
    struct tm *t;
    
    now = itme(NULL);
    t = localtime(&now);

    char *asct = asctime(t);
    memcpy(timestamp, asct, TIME_LENGTH);
}

static log_func() {
    while(1) {
        struct LogInfo *log;
        IL *item;
        char timestamp[TIME_LENGTH];

        pthread_mutex_lock(&logMutex);
        while(logs.len == 0) {
            pthread_cond_wait(&logCond, &logMutex);
        }

        // grab the log struct
        item = list_pop(&logs);
        log = CONTAINER_OF(item, struct LogInfo, move);

        // set the time and print to the file
        current_time(&timestamp);
        fprintf(f, LOG_FMT, timestamp, log->level, log->file, log->line, log->message);
        // for now we will flush after every write
        // if this is slow we can flush on intervals maybe
        fflush(f);

        pthread_mutex_unlock(&logMutex);
    }
} 