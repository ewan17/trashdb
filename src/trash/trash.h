#ifndef TRASH_H
#define TRASH_H

#define MAX_DIR_SIZE 1024

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

int init_db();

/**
 * Will not support multiple dbs at this time.
 * 
 * @param   Settings    settings struct
 * @note    Returns NULL if cannot start db.
*/
void start_trash(Settings *settings);

void kill_trash(TrashDB *db);

#endif //TRASH_H