#include "trashdb.h"
#include "utilities.h"
#include <assert.h>
    
MDB_env *env;
char *tableName = "test";

int main(int argc, char *argv[]) {
    assert(trash_mkdir(TABLE_DIR, TABLE_DIR_LEN, 0755) == 0);

    env = open_env(tableName, 10485760, 10, 10);
    assert(env != NULL);

    get_env_test();
    get_dbi_test();
    open_all_test();
    close_env_test();

    return 0;
}

void open_all_test() {
    OpenEnv *oenv;
    MDB_txn *txn;
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    oenv = get_env_from_txn(txn);
    open_all(txn, oenv);
}

void get_dbi_test() {
    MDB_txn *txn;
    MDB_stat stat;
    MDB_dbi dbi = 14;
    int rc;

    rc = get_dbi(tableName, INDICES, &dbi);
    assert(rc == 0);

    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    rc = mdb_stat(txn, dbi, &stat);
    assert(rc == 0);
}

void get_env_test() {
    MDB_env *tenv;
    tenv = get_env(tableName);
    assert(tenv != NULL);

    assert(tenv == env);
}

/**
 * @note    bad test. fix later
*/
void close_env_test() {
    MDB_env *tenv;

    close_env(env);
    env = NULL;

    tenv = get_env(tableName);
    assert(tenv == NULL);
}