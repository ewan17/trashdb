#ifndef TRASH_H
#define TRASH_H

#include "global.h"

#include "lmdb.h"
#include "db.h"
#include "pool.h"

#define TRASH_SUCCESS (0)
#define TRASH_ERROR (-1)
#define TRASH_EOF (-2)

// we currently only support one env
extern char *filename;
extern unsigned int workerThreads;

// these are default values for lmdb
#define DB_SIZE 10485760
// this should be the number of threads that we have running
#define MAX_READERS workerThreads
#define NUM_DBS 50

#ifdef TEST
#define TRASH_DIR "./test/"
#else
#define TRASH_DIR "/var/local/trashdb/"
#endif

#define ENV_DIR TRASH_DIR "env/"
#define ENV_DIR_LEN sizeof(ENV_DIR) - 1

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