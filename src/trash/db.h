#ifndef DB_H
#define DB_H

int start_txn();
int end_txn();
int abort_txn();
int open_db();

void create_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads);

int open_env(MDB_env *env, const char *path);

void close_env(MDB_env *env);

#endif //DB_H