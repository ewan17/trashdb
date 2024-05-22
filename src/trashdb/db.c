#include "trashdb.h"

void close_indexes(MDB_txn *txn) {

}

/**
 * @todo    add logging to this function
*/
static int valid(int rc, int line) {
    if(rc != 0) {
        exit(1);
    }
}

// static INIT_LIST(oDbs);

// static pthread_mutex_t oMutex = PTHREAD_MUTEX_INITIALIZER;

// // Tracks all the curently open dbs
// struct OpenDb {
//     struct IList il;
//     MDB_env *env;
//     const char *dbname;
//     unsigned int flags;
//     MDB_dbi *dbi;
// };

// static bool is_db_open(const char *path);

// int start_trash_txn(MDB_env *env, unsigned int flags) {
//     mdb_txn_begin(, NULL, );
// }

// /**
//  * @todo    loop through open dbs and check if db is open
// */
// static bool is_db_open(const char *path) {
//     struct IList *curr;
//     for_each(&oDbs.head, curr) {
//         struct OpenDb *oDb;
//         oDb = CONTAINER_OF(curr, struct OpenDb, il);

//         int rc = strcmp(path, oDb->dbname);
//         if(rc == 0) {
//            return true; 
//         }
//     }
//     return false;
// }

// static struct OpenDb *remove_db(const char *path) {
//     struct IList *curr;
//     for_each(&oDbs.head, curr) {
//         struct OpenDb *oDb;
//         oDb = CONTAINER_OF(curr, struct OpenDb, il);

//         int rc = strcmp(path, oDb->dbname);
//         if(rc == 0) {
//            return ilist_remove(curr); 
//         }
//     }
//     return NULL;
// }