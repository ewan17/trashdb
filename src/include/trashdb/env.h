#ifndef ENV_H
#define ENV_H

MDB_env *open_env(const char *path, size_t dbsize, unsigned int numdbs, unsigned int numthreads);

/**
 * @note    all txns and cursors must be closed before calling this function
 * @note    database handlers will be closed within this function
*/
void close_env(MDB_env *env);

MDB_env *get_env(const char *path);

/**
 * @note    this will loop through the unnamed db and open all db handlers
 * @note    the txn should be a read-only txn
*/
int open_all_dbs(MDB_txn *txn);

MDB_dbi *open_db(MDB_txn *txn, const char *index, int flags);

MDB_dbi *get_dbi(MDB_env *env, const char *index);

#endif //ENV_H