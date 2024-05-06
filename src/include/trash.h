#ifndef TRASH_H
#define TRASH_H

#include <stddef.h>

#include "lmdb.h"
#include "trashinit.h"
#include "db.h"
#include "utilities.h"

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

typedef struct TrashDB {
    MDB_env *env;
    KiddiePool *pool;
} TrashDB;

typedef struct TrashWorker {
    MDB_txn *txn;
    char *dbname;
} TrashWorker;

#endif //TRASH_H