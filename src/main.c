#include "global.h"

#define NO_NULL(item) \
    check_not_null(item, __FILE__, __LINE__)

struct event_base *serverBase;
pthread_mutex_t servBaseMutex = PTHREAD_MUTEX_INITIALIZER;

static struct event *sigintEvent;
static struct event *sighupEvent;
static struct event *sigtermEvent;

static void setup_signals();
static void graceful_exit();
static void sighup_handler();

static void check_not_null(void *item, const char *file, int line);

int main() {
    serverBase = event_base_new();
    NO_NULL(serverBase);
    if(serverBase == NULL) {
        exit(1);
    }

    setup_signals();

    run_server();
}

static void setup_signals() {
    sigintEvent = evsignal_new(serverBase, SIGINT, graceful_exit, NULL);
    NO_NULL(sigintEvent);

    sigtermEvent = evsignal_new(serverBase, SIGTERM, graceful_exit, NULL);
    NO_NULL(sigintEvent);

    /**
     * @todo    handle config file updates
    */
    sighupEvent = evsignal_new(serverBase, SIGHUP, sighup_handler, NULL);
    NO_NULL(sigintEvent);
}

static void graceful_exit() {
    /**
     * @todo    make sure all the threads are handled accordingly
     * @note    this currently only stops the event loop does not handle clean up in the pool
    */
    kill_server();
}

static void sighup_handler() {
    /**
     * @todo    want this to read a conf file and update the new db settings
    */
}

static void check_not_null(void *item, const char *file, int line) {
    if(item == NULL) {
        /**
         * @todo    add logging to include the file and the line
         * @note    for now printf()
        */
       printf("Error at: \n-- %s\n-- %d", file, line);
       exit(1);
    }
    
}