#ifndef GLOBAL_H
#define GLOBAL_H

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include "utilities.h"
#include "il.h"
#include "logger.h"

#define VAR_PATH "/var/local/trashdb/"

#define TRASH_SUCCESS (0)
#define TRASH_ERROR (-1)
#define TRASH_EOF (-2)

extern const char *logFile;
extern int logFileLen;

#endif //GLOBAL_H
