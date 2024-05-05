#ifndef DB_H
#define DB_H

#define SUCCESS 0
#define ERROR -1

#include <stdlib.h>
#include <lmdb.h>
#include <event2/event.h>
#include <stdbool.h> 

#include "trash.h"
#include "../log/logger.h"

typedef struct TrashDB {
    MDB_env *env;
    const char *dbPath;

    size_t dbSize;
    size_t threads; 

    char *hostName;
    unsigned short port;
    int sockFam;
    bool isSockAbstract;

    int currConnections;
    int maxConnections;

    KiddiePool *pool;
} TrashDB;

#endif //DB_H