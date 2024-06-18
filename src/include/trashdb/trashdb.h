#ifndef TRASH_H
#define TRASH_H

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <regex.h>
#include <assert.h>

#include "lmdb.h"
#include "uthash.h"
#include "env.h"
#include "db.h"

#define TRASH_SUCCESS (0)
#define TRASH_ERROR (-1)
#define TRASH_EOF (-2)

extern unsigned int workerThreads;

// these are default values for lmdb
#define DB_SIZE 10485760
// this should be the number of threads that we have running
#define MAX_READERS workerThreads
#define NUM_DBS 50

#ifdef DEBUG
#define TRASH_DIR ""
extern int flag;
#else
#define TRASH_DIR "/var/local/trashdb/"
#endif

#define TABLE_DIR TRASH_DIR "tables/"
#define TABLE_DIR_LEN sizeof(TABLE_DIR) - 1

/**
 * @note    this will change later
*/
#define MAX_DBS 1

/**
 * @todo    figure out what these pool size numbers should be
*/
#ifdef NOTLS
// the max readers we can have 
#define TXN_POOL_CAPACITY workerThreads
#else
// only one read thread when working with thread local storage
#define TXN_POOL_CAPACITY 1
#endif

#endif //TRASH_H