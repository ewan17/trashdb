#include "trashdb.h"
#include "utilities.h"
#include <assert.h>
    
MDB_env *env;
char *tableName = "test";

void open_new_db_test() {
    OpenEnv *oenv;
    MDB_txn *rtxn;
    MDB_dbi dbi;
    int rc;

    const char *indexName = "testy";

    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &rtxn);
    assert(rc == 0);

    oenv = get_env_from_tablename(tableName);

    rc = open_db(rtxn, oenv, indexName, &dbi);
    assert(rc == 0);
    assert(dbi != 0);

    rc = open_db(rtxn, oenv, indexName, &dbi);
    assert(rc == 0);
    assert(dbi != 0);

    // this code will always be run in debug mode, just removes the red squiggle
    #ifdef DEBUG
    assert(flag == 1);
    flag = 0;
    #endif

    mdb_txn_abort(rtxn);
}

void open_all_test() {
    OpenEnv *oenv;
    MDB_txn *wtxn, *rtxn;
    MDB_dbi dbi;
    int rc;

    const char *indexName = "testy";

    rc = mdb_txn_begin(env, NULL, 0, &wtxn);
    assert(rc == 0);
    rc = mdb_dbi_open(wtxn, indexName, MDB_CREATE, &dbi);
    assert(rc == 0);
    mdb_txn_commit(wtxn);

    mdb_dbi_close(env, dbi);

    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &rtxn);
    assert(rc == 0);
    oenv = get_env_from_txn(rtxn);
    open_all(rtxn, oenv);
    rc = get_dbi(tableName, indexName, &dbi);
    assert(rc == 0);
    mdb_txn_abort(rtxn);
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
    mdb_txn_abort(txn);
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

int main(int argc, char *argv[]) {
    assert(trash_mkdir(TABLE_DIR, TABLE_DIR_LEN, 0755) == 0);

    env = open_env(tableName, 10485760, 10, 10);
    assert(env != NULL);

    get_env_test();
    get_dbi_test();
    open_all_test();
    open_new_db_test();
    close_env_test();

    return 0;
}
