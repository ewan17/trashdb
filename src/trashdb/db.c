#include "trashdb.h"
#include "utilities.h"

struct Readers {
    const char *tablename;
    TrashTxn *txns[TXN_POOL_CAPACITY];
    size_t numTxns;

    #ifdef NOTLS
    pthread_spinlock_t spinner;
    #endif

    UT_hash_handle hh;
};

struct TrashTxn {
    MDB_txn *txn;
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
    
    flags = 0;
    ptxn = NULL;
    if(!write) {
        if(parent != NULL) {
            // nested txns do not get stored in the readers struct
            ptxn = parent->txn;
            flags = MDB_RDONLY;
        } else {
            tt = internal_get_read_txn(tablename);
            goto bottom;
        }
    }

    rc = mdb_txn_begin(env, ptxn, flags, &txn);
    if(rc != 0) {
        /**
         * @todo    handle error
        */
        exit(1);
    }
    internal_create_trash_txn(&tt, txn, NULL);
    

bottom:
    return tt;
}

void end_txn(TrashTxn *tt, bool commit) {
    if(commit) {
        mdb_txn_commit(tt->txn);
    } else {
        mdb_txn_abort(tt->txn);
    }

    free(tt);
}

void return_read_txn(TrashTxn *tt) {
    struct Readers *rdrs;
    bool isFull;

    if((rdrs = tt->rdrs) == NULL) {
        // this is a write txn
        return;
    }
    isFull = true;

    mdb_txn_reset(tt->txn);

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

int trash_put(MDB_dbi dbi, MDB_val *key, MDB_val *val, unsigned int flags) {
    
}

int trash_get() {

}

static TrashTxn *internal_get_read_txn(const char *tablename) {
    struct Readers *rdrs;
    TrashTxn *tt = NULL;

    rdrs = internal_get_readers(tablename);
    if(rdrs == NULL) {
        if(!env_exists(tablename)) {
            goto bottom;
        }
        internal_create_readers(&rdrs, tablename);
    }

    if(rdrs->numTxns == 0) {
        /**
         * @todo    create a read txn
         * @note    how many additional txns can we create? this will effect the max readers that was set for the env on open
        */
    } else {
        tt = rdrs->txns[--rdrs->numTxns];
        mdb_txn_renew(tt->txn);
    }

bottom:
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