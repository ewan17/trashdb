#ifndef DB_H
#define DB_H

int start_trash_txn(MDB_env *env, unsigned int flags);
int end_trash_txn();
int abort_txn();
int open_db();

int open_env(const char *path, size_t dbsize, unsigned int numdbs, unsigned int numthreads);
void close_env(MDB_env *env);

#endif //DB_H