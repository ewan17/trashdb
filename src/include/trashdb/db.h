#ifndef DB_H
#define DB_H

#define METADATA "metadata"

#define ENV_EXISTS 0x02

#define DB_EXISTS 0x04
#define DB_DNE 0x08

#define TRASH_RD_TXN 0x01
#define TRASH_WR_TXN 0x02
#define TRASH_TXN_COMMIT 0X04
#define TRASH_TXN_FIN 0x08

typedef struct TrashTxn {
    MDB_txn *txn;
    // the flags represent the type of txn
    // if it should be committed
    // if it is should be returned to the reader pool (only for rd txns)
    unsigned int flags;

    struct OpenDb *db;
} TrashTxn;

MDB_env *open_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads);
void close_env();

int begin_txn(TrashTxn **tt, const char *dbname, int rd);
int change_txn_db(TrashTxn *tt, const char *dbname);
// int nest_txn(TrashTxn **tt, const char *db, TrashTxn *pTxn);
void return_txn(TrashTxn *tt);

void create_cursor(MDB_cursor **curr, TrashTxn *tt);

void open_db(TrashTxn *tt, const char *dbname, unsigned int flags);

int trash_put(TrashTxn *tt, void *key, size_t keyLen, void *val, size_t valLen, unsigned int flags);
int trash_get(TrashTxn *tt, void *key, size_t keyLen, MDB_val *data);

#endif //DB_H