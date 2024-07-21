#include "trashdb.h"

char *filename = "bruh";

void big_test() {
    TrashTxn *tt;
    MDB_val key,val;
    char result[256];
    int rc;

    rc = begin_txn(&tt, "test", TRASH_WR_TXN);
    assert(rc == DB_DNE);

    open_db(tt, "test", MDB_CREATE);

    rc = trash_put(tt, "testkey", 7, "testval", 7, 0);
    assert(rc == TRASH_SUCCESS);

    key.mv_data = "testkey";
    key.mv_size = 7;
    rc = mdb_get(tt->txn, tt->db->id, &key, &val);
    assert(rc == 0);
    
    return_txn(tt);

    memcpy(result, val.mv_data, val.mv_size);
    result[val.mv_size] = '\0';
    rc = strcmp("testval", result);
    assert(rc == 0);

    rc = begin_txn(&tt, "test", TRASH_WR_TXN);
    assert(rc == 0);

    // opening an existing db
    open_db(tt, "test", MDB_CREATE);
    rc = change_txn_db(tt, METADATA);
    assert(rc == 0);
    rc = strcmp(METADATA, tt->db->name);
    assert(rc == 0);

    rc = change_txn_db(tt, "not_db");
    assert(rc == DB_DNE);
}

void tls_single() {
    TrashTxn *tt;
    MDB_val key,val;
    // just so I do not have to malloc
    char tlsResult[256];
    int rc;

    init_thread_local_readers();

    rc = begin_txn(&tt, "test", TRASH_WR_TXN);
    assert(rc == DB_DNE);

    open_db(tt, "test", MDB_CREATE);

    rc = trash_put(tt, "testkey", 7, "testval", 7, 0);
    assert(rc == TRASH_SUCCESS);

    return_txn(tt);

    rc = begin_txn(&tt, "test", TRASH_RD_TXN);
    assert(rc != DB_DNE);

    key.mv_data = "testkey";
    key.mv_size = 7;
    rc = mdb_get(tt->txn, tt->db->id, &key, &val);
    assert(rc == 0);
    
    return_txn(tt);

    memcpy(tlsResult, val.mv_data, val.mv_size);
    tlsResult[val.mv_size] = '\0';
    rc = strcmp("testval", tlsResult);
    assert(rc == 0);
}

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

    // big_test();
    tls_single();

    return 0;
}