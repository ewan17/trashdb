#ifndef DB_H
#define DB_H

#define METADATA "metadata"

#define ENV_EXISTS 0x02

#define DB_EXISTS 0x04
#define DB_DNE 0x08

#define TRASH_RD_TXN 0x01
#define TRASH_WR_TXN 0x02
#define TRASH_TXN_COMMIT 0X04
#define TRASH_TXN_DONT_FIN 0x08

enum DbAction {
    open,
    close,
    empty,
    delete
};

// this is metadata that is stored in the lmdb file
// can be accessed with lmdb txn and then deserialized
struct EnvMeta {
    /**
     * @todo    add metadata fields later
     */
};

// this is metadata that is stored in the lmdb file
// can be accessed with lmdb txn and then deserialized
struct DbMeta {
    // the flags used to open the db
    unsigned int flags;
    /**
     * @todo add additional values when neeeded
     */
};

// Tracks all the currently open envs
/**
 * @todo    the filename should not be the key, it should be some uuid.
 * @todo    hash the filename for the id
*/
struct OpenEnv {
    MDB_env *env;
    // flags applied to open db
    unsigned int envFlags;
    // list of open dbs
    struct LL dbs;
    pthread_rwlock_t envLock;
};

// the open databases that are mainly for store the db id
// the db id can be different if you re open the env
struct OpenDb {
    IL move;
    // db name
    const char *name;
    // flags applied to open the db
    unsigned int flags;
    // db handler id
    MDB_dbi id;
    
    // actions that can be applied to the db
    enum DbAction action;
    
    // num of threads currently using db id
    unsigned int count;
    pthread_mutex_t odbMutex;
};

typedef struct TrashTxn {
    MDB_txn *txn;
    // the flags represent the type of txn
    // if it should be committed
    // if it is should be returned to the reader pool (only for rd txns)
    unsigned int actions;

    struct OpenDb *db;
} TrashTxn;

void init_thread_local_readers();

int open_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads);
void close_env();

int begin_txn(TrashTxn **tt, const char *dbname, int rd);

int change_txn_db(TrashTxn *tt, const char *dbname);
// int nest_txn(TrashTxn **tt, const char *db, TrashTxn *pTxn);
void return_txn(TrashTxn *tt);

void create_cursor(MDB_cursor **curr, TrashTxn *tt);
void return_cursor(MDB_cursor *curr);

void open_db(TrashTxn *tt, const char *dbname, unsigned int flags);

int trash_put(TrashTxn *tt, void *key, size_t keyLen, void *val, size_t valLen, unsigned int flags);
int trash_get(TrashTxn *tt, void *key, size_t keyLen, MDB_val *data);

#endif //DB_H