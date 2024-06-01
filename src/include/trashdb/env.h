#ifndef ENV_H
#define ENV_H

#define INDICES "indices"

#define MAX_TABLE_NAME_LEN 64

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
OpenEnv *get_env_from_tablename(const char *tableName);
OpenEnv *get_env_from_path(const char *path);

void tablename_from_path(const char *path, char **tableName);

void begin_write_txn(OpenEnv *oEnv);
void end_write_txn(OpenEnv *oEnv);

#endif //ENV_H