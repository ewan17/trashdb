#ifndef DB_H
#define DB_H

/**
 * @note    needs to be a write transaction with the MDB_CREATE flag set
*/
int create_index();

/**
 * @note    opens all indexes within a table by getting the keys from the unnamed database
 * @note    returns the read only transaction so the thread can reuse the txn for other reads
*/
MDB_txn *open_indexes(MDB_env *env);

/**
 * @note    opens the unnamed database and loops throught the keys to close them
*/
void close_indexes(MDB_txn *txn);

int start_trash_txn(MDB_env *env, unsigned int flags);
int end_trash_txn();
int abort_txn();
int open_db();

#endif //DB_H