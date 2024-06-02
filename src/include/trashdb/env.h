#ifndef ENV_H
#define ENV_H

#define INDICES "indices"

#define MAX_TABLE_NAME_LEN 64

typedef struct OpenEnv OpenEnv;
typedef unsigned int EnvId;

MDB_env *open_env(char *tableName, size_t dbsize, unsigned int numdbs, unsigned int numthreads);
MDB_env *get_env(const char *tableName);
bool env_exists(const char *tableName);
void close_env(MDB_env *env);

bool is_all_open(OpenEnv *oEnv);
void open_all(MDB_txn *txn, OpenEnv *oEnv);
int open_db(MDB_txn *txn, OpenEnv *oEnv, const char *indexName, MDB_dbi *dbi);
void close_db(const char *tableName, const char *indexName);
void close_all(const char *tableName);

int get_dbi(const char *tableName, const char *indexName, MDB_dbi *dbi);

OpenEnv *get_env_from_cur(MDB_cursor *cur);
OpenEnv *get_env_from_txn(MDB_txn *txn);
OpenEnv *get_env_from_tablename(const char *tableName);
OpenEnv *get_env_from_path(const char *path);

#ifdef NOTLS
void write_lock(OpenEnv *oEnv);
void write_unlock(OpenEnv *oEnv);
#endif

#endif //ENV_H