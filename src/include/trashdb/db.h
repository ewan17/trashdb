#ifndef DB_H
#define DB_H

typedef struct TrashTxn TrashTxn;

TrashTxn *begin_txn(const char *tableName, TrashTxn *parent, bool write);
void return_txn(TrashTxn *tt);

int trash_put(TrashTxn *tt, MDB_dbi dbi, MDB_val *key, MDB_val *val, unsigned int flags);
int trash_get(TrashTxn *tt, MDB_dbi dbi, MDB_val *key, MDB_val *val);

#ifdef DEBUG
MDB_txn *get_trash_txn(TrashTxn *tt);
#endif

#endif //DB_H