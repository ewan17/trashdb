#ifndef TRASH_H
#define TRASH_H

#include "global.h"

#include "lmdb.h"
#include "uthash.h"
#include "cook.h"
#include "db.h"
#include "env.h"
#include "buffy.h"

#define MAX_DIR_SIZE 1024
#define DB_SIZE 10485760

/**
 * @note    this needs fixing
*/
#define TRASH_PATH "../../../var/local/trashdb/"
#define LOG_DIR TRASH_PATH "logs/"
#define DB_DIR TRASH_PATH "data/"

/**
 * @note    this will change later
*/
#define MAX_DBS 1

/**
 * @note    the open envs and the dbs can be placed in there own env in lmdb, but may be too much overhead to check if they are open in there vs looping list
 * @note    honestly this looping could be bad
 * @todo    fix later
*/

typedef struct TrashThread {
    pthread_t tid;
    

    TrashData *td;
    // original set to NULL until the message is parsed
    char *tablename;
    char *indexname;

    MDB_txn *txn;
} TrashThread;

#endif //TRASH_H