#ifndef DB_H
#define DB_H

int start_trash_txn(MDB_env *env, unsigned int flags);
int end_trash_txn();
int abort_txn();
int open_db();

#endif //DB_H