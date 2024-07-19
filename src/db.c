#include "trashdb.h"

#define METADATA "metadata"
#define METADATA_FLAGS MDB_CREATE

#define META_META "meta_meta"
#define META_META_LEN strlen(META_META)

#define DB_PREFIX "dbs:"
#define DB_PREFIX_LEN 4
#define DB_METADATA_KEY_FORMAT DB_PREFIX "%s"

#define INVALID_DB_ID -1

enum DbAction {
    open,
    close,
    empty,
    delete
};

// this is metadata that is stored in the lmdb file
// can be accessed with lmdb txn and then deserialized
struct EnvMeta {
    /**
     * @todo    add metadata fields later
     */
};

// this is metadata that is stored in the lmdb file
// can be accessed with lmdb txn and then deserialized
struct DbMeta {
    // the flags used to open the db
    unsigned int flags;
    /**
     * @todo add additional values when neeeded
     */
};

// Tracks all the currently open envs
/**
 * @todo    the filename should not be the key, it should be some uuid.
 * @todo    hash the filename for the id
*/
struct OpenEnv {
    MDB_env *env;
    // flags applied to open db
    unsigned int envFlags;
    // list of open dbs
    struct LL dbs;
    pthread_rwlock_t envLock;
};

// the open databases that are mainly for store the db id
// the db id can be different if you re open the env
struct OpenDb {
    IL move;
    // db name
    const char *name;
    // flags applied to open the db
    unsigned int flags;
    // db handler id
    MDB_dbi id;
    
    // actions that can be applied to the db
    enum DbAction action;
    
    // num of threads currently using db id
    unsigned int count;
    pthread_mutex_t odbMutex;
};

struct Readers {
    MDB_cursor *iCurr;
    
    // although we only have one txn per thread, we may use nested txns
    TrashTxn *txns[TXN_POOL_CAPACITY];
    size_t numTxns;
};

/** INTERNAL FUNCTIONS **/
static void init_metadata();

static void internal_create_open_env(struct OpenEnv **oEnv, MDB_env *env);
static int internal_set_env_fields(MDB_env *env, size_t dbsize, unsigned int numdbs, unsigned int numthreads);

static int db_match(struct OpenDb *db, const char *dbname);
static struct OpenDb *internal_add_db(const char *dbname, MDB_dbi id, struct DbMeta *meta);
static void internal_close_db(const char *dbname);

static void internal_begin_txn(TrashTxn **tt, int rd);
static TrashTxn *internal_get_read_txn();
static void internal_create_trash_txn(TrashTxn **tt, MDB_txn *txn, int rdwr);

// environment that is open
static struct OpenEnv *oEnv = NULL;
// tls for the reader txns
static __thread struct Readers *rdrPool = NULL;

// called by the thread when it is first created
void init_thread_local_readers() {
    if(rdrPool != NULL) {
        return;
    }

    rdrPool = (struct Readers *)malloc(sizeof(struct Readers));
    assert(rdrPool != NULL);

    rdrPool->numTxns = TXN_POOL_CAPACITY;

    for (size_t i = 0; i < TXN_POOL_CAPACITY; i++)
    {
        TrashTxn *tt;
        MDB_txn *txn;
        
        /**
         * @todo    build an error handling function for lmdb error messages
        */
        assert(mdb_txn_begin(oEnv->env, NULL, MDB_RDONLY, &txn) == 0);
        mdb_txn_reset(txn);
        internal_create_trash_txn(&tt, txn, 1);

        rdrPool->txns[i] = tt;
    }
}

/**
 * @todo    this function needs logging for errors
 * @todo    check the length of the file name or pass the length of the filename as a parameter in the function
*/
int open_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    MDB_env *env;
    struct stat st;
    char filepath[256];
    int rc;

    if(oEnv != NULL) {
        return TRASH_SUCCESS;
    }

    if(validate_filename(filename) != 0) {
        return TRASH_ERROR;
    }   

    snprintf(filepath, sizeof(filepath), "%s%s/", ENV_DIR, filename);
    
    if(stat(filepath, &st) < 0) {
        if(errno = ENOENT) {
            size_t len = strlen(filepath);
            if(trash_mkdir(filepath, len, 0755) != 0) {
                return TRASH_ERROR;
            }
        } else {
            return TRASH_ERROR;
        }
    }

    mdb_env_create(&env);
    if((rc = internal_set_env_fields(env, dbsize, numdbs, numthreads)) != TRASH_SUCCESS) {
        return TRASH_ERROR;
    }

    if((rc = mdb_env_open(env, filepath, MDB_NOSYNC | MDB_NOMETASYNC, 0664)) != 0) {
        /**
         * @todo    handle the different possible errors for an invalid open
        */ 
        mdb_env_close(env);
        assert(0);
    };

    // setup open env struct
    internal_create_open_env(&oEnv, env);
    // open/create metadata db
    init_metadata(env);

    return TRASH_SUCCESS;
}

/**
 * @todo    this needs to be completed.
 * @note    all txns, dbs, cursors must be closed before calling
 * @note    only one thread can call this function
 */
void close_env() {

}

int begin_txn(TrashTxn **tt, const char *dbname, int rd) {
    int rc;

    internal_begin_txn(tt, rd);
    rc = change_txn_db(*tt, dbname);
    return rc;
}

int change_txn_db(TrashTxn *tt, const char *dbname) {
    struct OpenDb *db = NULL;
    int rc;

    rc = db_match(tt->db, dbname);
    if(rc != TRASH_ERROR) {
        return rc;
    }

    // check if the db is in oEnv db list
    pthread_rwlock_rdlock(&oEnv->envLock);
    IL *curr;
    for_each(&oEnv->dbs.head, curr) {
        struct OpenDb *tempdb;
        tempdb = CONTAINER_OF(curr, struct OpenDb, move);
        if(db_match(tempdb, dbname) == TRASH_SUCCESS) {
            db = tempdb;
            break;
        }
    }
    pthread_rwlock_unlock(&oEnv->envLock);

    if(db == NULL) {
        // return if the db does not exist
        return DB_DNE;
    }

    pthread_mutex_lock(&db->odbMutex);
    // count the number of txns that are using this db
    db->count++;
    pthread_mutex_unlock(&db->odbMutex);

    tt->db = db;
    
    return TRASH_SUCCESS;
}

/**
 * @note    lets work with these later
 */
// int nest_txn(TrashTxn **tt, const char *db, TrashTxn *pTxn) {
//     MDB_env *env;
//     MDB_txn *txn;
//     int rc;
//     env = get_env_from_txn(pTxn->txn);
//     rc = mdb_txn_begin(env, pTxn, MDB_RDONLY, txn);
//     assert(rc == 0);
//     internal_create_trash_txn(tt, txn, TRASH_RD_TXN);
//     rc = change_txn_db(*tt, db);
//     return rc;
// }

void return_txn(TrashTxn *tt) {
    struct OpenDb *db;

    if(tt->flags & TRASH_TXN_COMMIT) {
        mdb_txn_commit(tt->txn);
    }

    if(tt->flags & TRASH_WR_TXN) {
        // write txn
        // abort if the txn was not commited
        if(!(tt->flags & TRASH_TXN_COMMIT)) {
            mdb_txn_abort(tt->txn);
        }

        free(tt);
    } else {
        // read txn
        mdb_txn_reset(tt->txn);
        tt->flags &= ~TRASH_TXN_COMMIT;

        if(tt->flags & TRASH_TXN_FIN) {
            if(rdrPool->numTxns != TXN_POOL_CAPACITY) {
                rdrPool->txns[rdrPool->numTxns++] = tt;
            }
        }   
    }

    db = tt->db;
    pthread_mutex_lock(&db->odbMutex);
    // txn is no longer using db
    db->count--;
    pthread_mutex_unlock(&db->odbMutex);
}

void create_cursor(MDB_cursor **curr, TrashTxn *tt) {
    int rc;

    if(tt == NULL) {
        tt = internal_get_read_txn();
    }

    if(tt->db == NULL) {
        // defualts to the metadata db if not set
        change_txn_db(tt, METADATA);
    }

    // if read txn and metadata db is set, renew the cursor
    if(rdrPool != NULL && (tt->flags & TRASH_RD_TXN) && db_match(tt->db, METADATA) == 0) {
        // currently only renew txns that are read only for the metadata db
        assert(mdb_cursor_renew(tt->txn, rdrPool->iCurr) == 0);
        *curr = rdrPool->iCurr;
    } else {
        rc = mdb_cursor_open(tt->txn, tt->db->id, curr);
        assert(rc == 0);
    }
}

/**
 * If the db flags are in the metadata db, then those will be used.
 * If the db is not created yet, then the flags will have the MDB_CREATE and whatever other flags are passed in
 * 
 * @todo    make sure only one thread is opening the requested database at a time
 * @todo    make sure the database is not being closed and opened at the same time as well
 */
void open_db(TrashTxn *tt, const char *dbname, unsigned int flags) {
    MDB_dbi id;
    MDB_cursor *curr;
    TrashTxn *temp = tt;
    MDB_val key, data;

    struct OpenDb *db;
    struct DbMeta dbMeta;

    char dbKeyName[256];

    int rc;

    // ensure the txn passed in is set to the metadata db to read the dbs
    change_txn_db(temp, METADATA);
    create_cursor(&curr, temp);

    // check if the db is in the metadata db 
    // if not in db, create it
    rc = mdb_cursor_get(curr, &key, &data, MDB_SET);
    if(rc == MDB_NOTFOUND) {
        if(tt->flags & TRASH_RD_TXN) {
            /**
             * @note    txn cannot be open during write, reset then renew
             * @todo    confirm the reset and renew is ok
             */
            mdb_txn_reset(tt->txn);
            // create write txn with the metadata db set
            begin_txn(&temp, METADATA, TRASH_WR_TXN);
        }
        // since this is first time opening db, malloc dbMeta and set create flag
        dbMeta.flags = flags;
        flags |= MDB_CREATE;
    } else if(rc == 0) {
        // db was found
        // deserialize the key/value pair from the db metadata
        memcpy(&dbMeta, data.mv_data, data.mv_size);
        flags &= dbMeta.flags;
    } else {
        // should not get to this point
        assert(0);
    }

    // creates the desired db
    assert(mdb_dbi_open(temp->txn, dbname, flags, &id) == 0);

    // if dbMeta is not NULL,
    // new db needs to be created and committed within the write txn
    if(flags & MDB_CREATE) {
        // remove the MDB_CREATE flag before storing the flags in the metadata db
        flags &= ~MDB_CREATE;
        // append the new db information into the metadata db
        snprintf(dbKeyName, sizeof(dbKeyName), DB_METADATA_KEY_FORMAT, dbname);
        key.mv_data = (void *) dbKeyName;
        key.mv_size = strlen(dbKeyName);
        rc = trash_put(temp, key.mv_data, key.mv_size, (void *)&dbMeta, sizeof(dbMeta), 0);
        assert(rc == 0);
        // commit the created db if the calling function did not pass a write txn
        if(temp != tt) {
            return_txn(temp);
        }
        // renew the read txn that we had to reset
        mdb_txn_renew(tt->txn);
    }

    // add db to the env map
    db = internal_add_db(dbname, id, &dbMeta);
    tt->db = db;
}

/**
 * @todo    flags need to be checked with how the dbi was opened initially
 */
int trash_put(TrashTxn *tt, void *key, size_t keyLen, void *val, size_t valLen, unsigned int flags) {
    MDB_val keyData, valData;

    int rc;
    
    if(tt->flags & TRASH_RD_TXN) {
        return TRASH_ERROR;
    }

    keyData.mv_size = keyLen;
    keyData.mv_data = key;
    valData.mv_size = valLen;
    valData.mv_data = val;

    rc = mdb_put(tt->txn, tt->db->id, &keyData, &valData, flags);
    if(rc == 0) {
        tt->flags |= TRASH_TXN_COMMIT;
    }
    return rc;
}

int trash_get(TrashTxn *tt, void *key, size_t keyLen, MDB_val *data) {
    MDB_val keyData;
    int rc;

    keyData.mv_size = keyLen;
    keyData.mv_data = key;
    
    rc = mdb_get(tt->txn, tt->db->id, &keyData, data);
    return rc;
}

/**
 * Should only be called on startup.
 */
static void init_metadata() {
    TrashTxn *tt;

    MDB_dbi dbi;
    MDB_val key, val;
    MDB_cursor *curr;

    struct EnvMeta envMeta;
    struct DbMeta dbMeta;

    int rc;

    // create write txn
    rc = begin_txn(&tt, METADATA, TRASH_WR_TXN);
    // the db should not currently be in the list of open dbs
    if(rc == DB_DNE) {
        dbMeta.flags = METADATA_FLAGS;
        assert(mdb_dbi_open(tt->txn, METADATA, dbMeta.flags, &dbi) == 0);
        internal_add_db(METADATA, dbi, &dbMeta);
    }

    /**
     * @note    add env metadata here
     */

    // if this is first time opening the env, create the metadata key/val pair
    // do not overwrite if it already exists
    rc = trash_put(tt, META_META, META_META_LEN, (void *) &envMeta, sizeof(envMeta), MDB_NOOVERWRITE);
    if(rc == 0) {
        // added meta fields
        // return txn
        return_txn(tt);
    }

    if(rc != MDB_KEYEXIST) {
        // should not get here
        assert(0);
    }

    /**
     * @todo    set the db range keys
     */
    key.mv_data = (void *) DB_PREFIX;
    key.mv_size = DB_PREFIX_LEN;

    // open all dbs if the key exists 
    // meaning the env has already been configured and we might have dbs created
    create_cursor(&curr, tt);
    while((rc = mdb_cursor_get(curr, &key, &val, MDB_SET_RANGE)) == 0) {
        char *dbKeyName;
        char *dbname;

        dbKeyName = (char *)key.mv_data;
        // check that the dbkeyname starts with the db prefix
        rc = memcmp(dbKeyName, DB_PREFIX, DB_PREFIX_LEN);
        if(rc != 0) {
            break;
        }

        // the dbname is after the db prefix
        dbname = dbKeyName + DB_PREFIX_LEN;
        memcpy(&dbMeta, val.mv_data, val.mv_size);

        assert(mdb_dbi_open(tt->txn, dbname, dbMeta.flags, &dbi) == 0);
        internal_add_db(dbname, dbi, &dbMeta);
    }
}

static void internal_create_open_env(struct OpenEnv **oEnv, MDB_env *env) {
    *oEnv = malloc(sizeof(struct OpenEnv));
    assert((*oEnv) != NULL);

    (*oEnv)->env = env;
    init_list(&(*oEnv)->dbs);
}

/**
 * @note    this function will need to check if their is room for the desired db size
 * @todo    make this function better    
*/
static int internal_set_env_fields(MDB_env *env, size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    if(mdb_env_set_maxdbs(env, numdbs) != 0) {
        return TRASH_ERROR;
    }
    if(mdb_env_set_maxreaders(env, numthreads) != 0) {
        return TRASH_ERROR;
    }
    if(mdb_env_set_mapsize(env, dbsize) != 0) {
        return TRASH_ERROR;
    }
    return TRASH_SUCCESS;
}

static int db_match(struct OpenDb *db, const char *dbname) {
    if(db == NULL) {
        return TRASH_ERROR;
    }

    int rc;
    rc = (strcmp(db->name, dbname) == 0) ? TRASH_SUCCESS : TRASH_ERROR;

    return rc;
}

/**
 * @note    this code needs to lock the env before adding the db
 */
static struct OpenDb *internal_add_db(const char *dbname, MDB_dbi id, struct DbMeta *meta) {
    struct OpenDb *db;

    db = (struct OpenDb *)malloc(sizeof(struct OpenDb));
    assert(db != NULL);

    init_il(&(db)->move);

    db->name = dbname;
    db->id = id;
    db->flags = meta->flags;

    db->count = 0;
    pthread_mutex_init(&db->odbMutex, NULL);

    // append the db to the current open env
    pthread_rwlock_wrlock(&oEnv->envLock);
    list_append(&oEnv->dbs, &db->move);
    pthread_rwlock_unlock(&oEnv->envLock);

    return db;
}

/**
 * @todo    add this later
 */
static void internal_close_db(const char *dbname) {
    
}

static void internal_begin_txn(TrashTxn **tt, int rdwr) {
    MDB_txn *txn;
    int rc, flags;
    
    flags = 0;
    if(rdwr == TRASH_RD_TXN) {
        *tt = internal_get_read_txn();
    } else {
        rc = mdb_txn_begin(oEnv->env, NULL, flags, &txn);
        assert(rc == 0);

        internal_create_trash_txn(tt, txn, TRASH_WR_TXN);
    }
}

static TrashTxn *internal_get_read_txn() {
    TrashTxn *tt;
    if(rdrPool == NULL) {
        // the rdr pool should never be NULL unless the calling function is on env set up
        // in that case return a reader txn
        MDB_txn *txn;
        
        // create and rd txn and return
        mdb_txn_begin(oEnv->env, NULL, MDB_RDONLY, &txn);
        internal_create_trash_txn(&tt, txn, TRASH_RD_TXN);
        return tt;
    }

    if(rdrPool->numTxns == 0) {
        /**
         * @note    this should never happen
         */
        assert(0);
    } else {
        tt = rdrPool->txns[--rdrPool->numTxns];
        assert(mdb_txn_renew(tt->txn) == 0);
    }

    return tt;
}

static void internal_create_trash_txn(TrashTxn **tt, MDB_txn *txn, int rdwr) {
    unsigned int flags = 0;

    *tt = (TrashTxn *) malloc(sizeof(tt));
    assert(*tt != NULL);

    (*tt)->txn = txn;
    (*tt)->db = NULL;

    flags |= rdwr; 
    (*tt)->flags = flags;
}