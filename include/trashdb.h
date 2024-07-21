#ifndef TRASH_H
#define TRASH_H

#include "global.h"

#include "lmdb.h"
#include "cook.h"
#include "db.h"
#include "pool.h"

#define TRASH_SUCCESS (0)
#define TRASH_ERROR (-1)
#define TRASH_EOF (-2)

// we currently only support one env
extern const char *filename;
extern TPool *tp;

// these are default values for lmdb
#define DB_SIZE 10485760
// this is set to default readers for now
/**
 * @note    
 */
#define MAX_READERS 126
#define NUM_DBS 50

#ifdef TEST
#define DB_DIR "./test/"
#else
#define DB_DIR VAR_PATH "data/"
#endif

#define DB_DIR_LEN sizeof(DB_DIR) - 1

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