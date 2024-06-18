#include "trashdb.h"
#include "utilities.h"
#include <assert.h>

MDB_env *env;
char *tableName = "test";

const char *indexName = "testy";
MDB_dbi testDbi;

void begin_txn_test() {
    TrashTxn *tt;
    MDB_txn *txn = NULL;
    int rc;

    tt = begin_txn(tableName, NULL, true);

    assert(tt != NULL);
    
    #ifdef DEBUG
    txn = get_trash_txn(tt);
    #endif

    assert(txn != NULL);

    MDB_val keyP, dataP;
    dataP.mv_data = "yo";
    dataP.mv_size = strlen("yo");
    keyP.mv_size = strlen("huh");
    keyP.mv_data = (void *)"huh";
    
    assert(mdb_put(txn, testDbi, &keyP, &dataP, 0));
}

int main(int argc, char *argv[]) {    
    assert(trash_mkdir(TABLE_DIR, TABLE_DIR_LEN, 0755) == 0);

    env = open_env(tableName, 10485760, 10, 10);
    assert(env != NULL);


    MDB_txn *wtxn;
    int rc;

    rc = mdb_txn_begin(env, NULL, 0, &wtxn);
    assert(rc == 0);
    rc = mdb_dbi_open(wtxn, indexName, MDB_CREATE, &testDbi);
    assert(rc == 0);
    mdb_txn_commit(wtxn);

    begin_txn_test();

    return 0;
}