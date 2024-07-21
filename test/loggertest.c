#include "global.h"

const char *logFile = "server.log";

void log_test() {
    init_logger();
    TRASH_LOG(WARN, "test warn");
    TRASH_LOG(INFO, "test info");
    TRASH_LOG(DEBUG, "test debug");
    TRASH_LOG(ERROR, "test error");
    close_logger();
}

/**
 * @note    this needs to have a check to validate the log file has been populated
 * 
 * @todo    fix later
 */
int main(int argc, char *argv[]) {   

    log_test();

    return 0;
}