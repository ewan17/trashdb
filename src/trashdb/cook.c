#include "trashdb.h"

/**
 * @note    pool of readonly transactions
*/
struct RTxns {
    struct IList il;

    MDB_txn *txn;
    char *envname;
};

/**
 * @note    this the thread function.
 * @todo    figure out the structure of the arg
*/
void cook(void *arg) {
    
}