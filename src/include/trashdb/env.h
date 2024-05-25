#ifndef ENV_H
#define ENV_H

MDB_env *open_env(char *path, size_t dbsize, unsigned int numdbs, unsigned int numthreads);

/**
 * @note    all txns and cursors must be closed before calling this function
 * @note    database handlers will be closed within this function
*/
void close_env(MDB_env *env);

MDB_env *get_env(const char *path);

MDB_dbi *open_db(const char *table, const char *indexName);

int get_dbi(MDB_env *env, const char *index, MDB_dbi *dbi);

#endif //ENV_H