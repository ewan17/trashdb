#ifndef DB_H
#define DB_H

typedef struct TrashTxn TrashTxn;

TrashTxn *begin_txn(const char *tableName, TrashTxn *parent, bool write);
void end_txn(TrashTxn *tt, bool commit);
void return_read_txn(TrashTxn *tt);

int trash_put();
int trash_get();

#endif //DB_H