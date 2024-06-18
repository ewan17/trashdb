#include "trashdb.h"
#include "utilities.h"

struct Readers {
    const char *tablename;
    /**
     * @todo    add support for cursors
     */
    TrashTxn *txns[TXN_POOL_CAPACITY];
    size_t numTxns;

    #ifdef NOTLS
    pthread_spinlock_t spinner;
    #endif

    UT_hash_handle hh;
};

struct TrashTxn {
    MDB_txn *txn;
    bool commit;
    struct Readers *rdrs;
};

static TrashTxn *internal_get_read_txn(const char *tablename);

static void internal_create_trash_txn(TrashTxn **tt, MDB_txn *txn, struct Readers *rdrs);

static void internal_create_readers(struct Readers **rdrs, const char *tablename);

static void rdrs_lock(struct Readers *rdrs);
static void rdrs_unlock(struct Readers *rdrs);
static void pool_lock(bool write);
static void pool_unlock();

#ifdef NOTLS
static struct Readers *rdrPool = NULL;
static pthread_rwlock_t rdrPoolLock = PTHREAD_RWLOCK_INITIALIZER;
#else
static __thread struct Readers *rdrPool = NULL;
#endif

TrashTxn *begin_txn(const char *tablename, TrashTxn *parent, bool write) {
    MDB_env *env;
    MDB_txn *txn, *ptxn;
    TrashTxn *tt;
    int rc, flags;

    env = get_env(tablename);
    if(env == NULL) {
        env = open_env(tablename, DB_SIZE, NUM_DBS, MAX_READERS);
        assert(env != NULL);
    }
    
    flags = 0;
    ptxn = NULL;
    if(!write) {
        if(parent != NULL) {
            /**
             * @todo    add support for nested txns later
             */
            assert(0);
            // nested txns do not get stored in the readers struct
            ptxn = parent->txn;
            flags = MDB_RDONLY;
        } else {
            tt = internal_get_read_txn(tablename);
            goto bottom;
        }
    }

    rc = mdb_txn_begin(env, ptxn, flags, &txn);
    assert(rc == 0);

    internal_create_trash_txn(&tt, txn, NULL);
    
bottom:
    return tt;
}

void return_txn(TrashTxn *tt) {
    struct Readers *rdrs = tt->rdrs;

    if(rdrs == NULL) {
        // write txn
        if(tt->commit) {
            mdb_txn_commit(tt->txn);
        } else {
            mdb_txn_abort(tt->txn);
        }

        free(tt);
        return;
    }

    bool isFull = true;

    mdb_txn_reset(tt->txn);
    tt->commit = false;

    rdrs_lock(rdrs);
    if(rdrs->numTxns != TXN_POOL_CAPACITY) {
        isFull = false;
        rdrs->txns[rdrs->numTxns++] = tt;
    }
    rdrs_unlock(rdrs);

    if(isFull) {
        // this will only be true if we want to declare a txn when there are no more available read txns
        /**
         * @note    idk if this is a bad idea though
        */
        mdb_txn_abort(tt->txn);
        free(tt);
    }
}

/**
 * @todo    flags need to be checked with how the dbi was opened initially
 */
int trash_put(TrashTxn *tt, MDB_dbi dbi, MDB_val *key, MDB_val *val, unsigned int flags) {
    int rc;
    
    if(tt->rdrs != NULL) {
        return TRASH_ERROR;
    }

    rc = mdb_put(tt->txn, dbi, key, val, flags);
    assert(rc == 0);

    tt->commit = true;

    return TRASH_SUCCESS;
}

int trash_get(TrashTxn *tt, MDB_dbi dbi, MDB_val *key, MDB_val *val) {
    int rc;
    
    rc = mdb_get(tt->txn, dbi, key, val);
    assert(rc == 0);

    return TRASH_SUCCESS;
}   

static TrashTxn *internal_get_read_txn(const char *tablename) {
    struct Readers *rdrs;

    rdrs = internal_get_readers(tablename);
    if(rdrs == NULL) {
        internal_create_readers(&rdrs, tablename);
    }

    TrashTxn *tt;

    rdrs_lock(rdrs);
    if(rdrs->numTxns == 0) {
        rdrs_unlock(rdrs);
        /**
         * @note    this should never happen
         */
        assert(0);
    } else {
        tt = rdrs->txns[--rdrs->numTxns];
        rdrs_unlock(rdrs);

        mdb_txn_renew(tt->txn);
    }

    return tt;
}

static void internal_create_trash_txn(TrashTxn **tt, MDB_txn *txn, struct Readers *rdrs) {
    *tt = (TrashTxn *) malloc(sizeof(tt));
    if(*tt == NULL) {
        /**
         * @todo    handle out of memory error here
        */
        exit(1);
    }

    (*tt)->txn = txn;
    (*tt)->rdrs = rdrs;
    (*tt)->commit = false;
}

static struct Readers *internal_get_readers(const char *tablename) {
    struct Readers *rdrs;

    pool_lock(false);
    HASH_FIND_STR(rdrPool, tablename, rdrs);
    pool_unlock();

    return rdrs;
}

static void internal_create_readers(struct Readers **rdrs, const char *tablename) {
    MDB_env *env;
    
    *rdrs = (struct Readers *) malloc(sizeof(struct Readers));
    if(*rdrs == NULL) {
        /**
         * @todo    handle error
        */
        exit(1);
    }

    (*rdrs)->tablename = tablename;
    (*rdrs)->numTxns = TXN_POOL_CAPACITY;

    env = get_env(tablename);

    for (size_t i = 0; i < TXN_POOL_CAPACITY; i++)
    {
        TrashTxn *tt;
        MDB_txn *txn;
        
        /**
         * @todo    build an error handling function for lmdb error messages
        */
        mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
        mdb_txn_reset(txn);
        internal_create_trash_txn(&tt, txn, *rdrs);

        (*rdrs)->txns[i] = tt;
    }

    pool_lock(true);
    HASH_ADD_STR(rdrPool, tablename, *rdrs);
    pool_unlock();
}


static void rdrs_lock(struct Readers *rdrs) {
    #ifdef NOTLS
    pthread_spin_lock(&rdrs->spinner);
    #endif
}

static void rdrs_unlock(struct Readers *rdrs) {
    #ifdef NOTLS
    pthread_spin_unlock(&rdrs->spinner);
    #endif
}

static void pool_lock(bool write) {
    #ifdef NOTLS
    if(write) {
        pthread_rwlock_wrlock(&rdrPoolLock);
    } else {
        pthread_rwlock_rdlock(&rdrPoolLock);
    }
    #endif
};

static void pool_unlock() {
    #ifdef NOTLS
    pthread_rwlock_unlock(&rdrPoolLock);
    #endif
};

#ifdef DEBUG
MDB_txn *get_trash_txn(TrashTxn *tt) {
    return tt->txn;
}
#endif