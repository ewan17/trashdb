#ifndef ENV_H
#define ENV_H

typedef struct OpenEnv OpenEnv;

MDB_env *open_env(char *tableName, size_t dbsize, unsigned int numdbs, unsigned int numthreads);
MDB_env *get_env(const char *tableName);
void close_env(MDB_env *env);

bool is_all_open(OpenEnv *oEnv);
void open_all(MDB_txn *txn, OpenEnv *oEnv);
int open_db(MDB_txn *txn, OpenEnv *oEnv, const char *indexName, MDB_dbi *dbi);

int get_dbi(const char *tableName, const char *indexName, MDB_dbi *dbi);

OpenEnv *get_env_from_cur(MDB_cursor *cur);
OpenEnv *get_env_from_txn(MDB_txn *txn);
OpenEnv *get_env_from_path(const char *tableName);

void begin_write_txn(OpenEnv *oEnv);
void end_write_txn(OpenEnv *oEnv);

#endif //ENV_H