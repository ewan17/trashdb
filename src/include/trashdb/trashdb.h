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

#include "lmdb.h"
#include "uthash.h"
#include "env.h"

#define TRASH_SUCCESS (0)
#define TRASH_ERROR (-1)
#define TRASH_EOF (-2)

#define DB_SIZE 10485760

#ifdef DEBUG
#define TRASH_DIR ""
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
 * @note    the open envs and the dbs can be placed in there own env in lmdb, but may be too much overhead to check if they are open in there vs looping list
 * @note    honestly this looping could be bad
 * @todo    fix later
*/

// typedef struct TrashThread {
//     pthread_t tid;
    

//     TrashData *td;
//     // original set to NULL until the message is parsed
//     char *tablename;
//     char *indexname;

//     MDB_txn *txn;
// } TrashThread;

#endif //TRASH_H