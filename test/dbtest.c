#include "trashdb.h"

char *filename = "bruh";

// MDB_dbi testDbi;

// void begin_write_txn_test() {
//     TrashTxn *tt;
//     MDB_txn *txn = NULL;
//     int rc;

//     tt = begin_txn(tableName, NULL, true);

//     assert(tt != NULL);
    
//     #ifdef TEST
//     txn = get_trash_txn(tt);
//     #endif

//     assert(txn != NULL);

//     MDB_val keyP, dataP;
//     dataP.mv_data = "testy";
//     dataP.mv_size = strlen("testy");
//     keyP.mv_size = strlen("testkey");
//     keyP.mv_data = (void *)"testkey";
    
//     assert(mdb_put(txn, testDbi, &keyP, &dataP, 0) == 0);

//     mdb_txn_abort(txn);
// }

// void begin_read_txn_test() {
//     TrashTxn *tt;
//     MDB_txn *txn = NULL;

//     tt = begin_txn(tableName, NULL, false);

//     assert(tt != NULL);
    
//     #ifdef TEST
//     txn = get_trash_txn(tt);
//     #endif

//     MDB_val key, data;
//     key.mv_data = "huh";
//     key.mv_size = strlen("huh");
//     assert(mdb_get(txn, testDbi, &key, &data) == 0);

//     char *result = (char *) data.mv_data;
//     assert(strcmp(result, "yo") == 0);

//     size_t readerCount;
//     #ifdef TEST
//     readerCount = get_reader_count(tt);
//     #endif

//     assert(readerCount < TXN_POOL_CAPACITY);
//     return_txn(tt);

//     #ifdef TEST
//     readerCount = get_reader_count(tt);
//     #endif
//     assert(readerCount == TXN_POOL_CAPACITY);
// }

// void trash_put_test() {
//     TrashTxn *tt;

//     MDB_val key, data;
//     data.mv_data = "put test";
//     data.mv_size = strlen("put test");
//     key.mv_size = strlen("pt");
//     key.mv_data = (void *)"pt";
    
//     tt = begin_txn(tableName, NULL, true);
    
//     assert(trash_put(tt, testDbi, &key, &data, 0) == 0);

//     return_txn(tt);

//     /*  read the data to make sure it is in db  */
//     MDB_txn *rtxn;
//     int rc;

//     rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &rtxn);
//     assert(rc == 0);

//     MDB_val keyR, dataR;
//     keyR.mv_data = "pt";
//     keyR.mv_size = strlen("pt");

//     assert(mdb_get(rtxn, testDbi, &keyR, &dataR) == 0);

//     char *result = (char *) data.mv_data;
//     assert(strcmp(result, "put test") == 0);

//     mdb_txn_abort(rtxn);
//     /*  done reading data   */
// }

// void trash_get_test() {
//     TrashTxn *tt;
//     MDB_txn *txn;

//     tt = begin_txn(tableName, NULL, false);

//     MDB_val keyR, dataR;
//     keyR.mv_data = "huh";
//     keyR.mv_size = strlen("huh");

//     assert(trash_get(tt, testDbi, &keyR, &dataR) == 0);

//     return_txn(tt);

//     char *result = (char *) dataR.mv_data;
//     assert(strcmp(result, "yo") == 0);
// }

int main(int argc, char *argv[]) {    
    assert(trash_mkdir(ENV_DIR, ENV_DIR_LEN, 0755) == 0);
    assert(open_env(DB_SIZE, NUM_DBS, 1) == 0);

    return 0;
}