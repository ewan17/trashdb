#include "trashdb.h"
#include <semaphore.h>

#define VALID(rc) \
    valid(rc, __LINE__) \

#define INDICES "indices"

struct OpenDbi {
    const char *name;
    MDB_dbi dbi;

    UT_hash_handle hh;
};

// Tracks all the currently open envs
struct OpenEnv {
    const char *path;
    MDB_env *env;

    // allows only one write txn for an env
    sem_t writeSem;

    // are all the sub databases for an env open?
    bool allOpen;

    // map of the open db handlers
    struct OpenDbi *oDbi;

    UT_hash_handle hh;
};

static struct OpenEnv *oEnvs = NULL;

// read-write locks for the env and db handler mappings
static pthread_rwlock_t envLock, dbiLock = PTHREAD_RWLOCK_INITIALIZER;

/**
 * @note    this function needs work
 * @todo    add logging for errors and figure out what the return errors should be if any
*/
static int valid(int rc, int line);

static void open_all_dbs(MDB_txn *txn);

static int internal_set_env_fields(MDB_env *env, size_t dbsize, unsigned int numdbs, unsigned int numthreads);

static void internal_create_open_env(struct OpenEnv **oEnv, MDB_env *env);
static void internal_create_handle(struct OpenDbi **oDbi, const char *index, MDB_dbi dbi);

static struct OpenEnv *internal_get_env_txn(MDB_txn *txn);
static struct OpenEnv *internal_get_env_path(const char *path);
static struct OpenEnv *internal_remove_env(const char *path);

/**
 * @note    this function will open all the dbis if there is room
 * @todo    work on a function that can determine if there is room for dbis to be open
*/
MDB_env *open_env(char *path, size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    MDB_env *env;
    MDB_txn *txn;
    MDB_dbi unDbi;

    struct OpenEnv *oEnv;
    struct OpenDbi *oDbi;

    struct stat st;

    int rc;

    if((env = get_env(path)) != NULL) {
        return env;
    }
    
    if(path == NULL || stat(path, &st) < 0) {
        return NULL;
    }

    if(!S_ISDIR(st.st_mode)) {
        /**
         * @note    if it made it here, the path could be a file and not a dir
         * @todo    create dir
         * @todo    need to make a dir where the users table files are stored
        */
    }

    mdb_env_create(&env);
    if((rc = internal_set_env_fields(env, dbsize, numdbs, numthreads)) != TRASH_SUCCESS) {
        return NULL;
    }

    if((rc = mdb_env_open(env, path, MDB_NOTLS, 0664)) != 0) {
        /**
         * @todo    handle the different possible errors with rc
        */ 
        mdb_env_close(env);
        return NULL;
    };

    // setup open env struct
    internal_create_open_env(&oEnv, env);

    // open the unnamed db
    VALID(mdb_txn_begin(env, NULL, 0, &txn));
    VALID(mdb_dbi_open(txn, NULL, 0, &unDbi));

    // create unnamed db key/value
    internal_create_handle(&oDbi, INDICES, unDbi);
    HASH_ADD_STR(oEnv->oDbi, name, oDbi);

    // add open env struct to env hash map
    pthread_rwlock_wlock(&envLock);
    HASH_ADD_STR(oEnvs, path, oEnv);
    pthread_rwlock_unlock(&envLock);

    open_all_dbs(txn);

    return env;
}

void close_env(MDB_env *env) {
    if(env != NULL) {
        struct OpenEnv *oEnv;
        const char *path;

        mdb_env_get_path(env, &path);
        oEnv = internal_remove_env(path);

        struct OpenDbi *curr, *tmp;
        curr = oEnv->oDbi;

        HASH_ITER(hh, oEnv->oDbi, curr, tmp) {
            mdb_dbi_close(env, curr->dbi);
            free(curr);
        }

        mdb_env_close(env);
    }
}

MDB_env *get_env(const char *path) {
    struct OpenEnv *result;
    result = internal_get_env_path(path);

    if(result != NULL) {
        return result->env;
    }

    return NULL;
}

int get_dbi(MDB_env *env, const char *indexName, MDB_dbi *dbi) {
    if(env == NULL) {
        return -1;
    }

    struct OpenEnv *oEnv;
    struct OpenDbi *oDbi;
    const char *path;

    mdb_env_get_path(env, &path);
    oEnv = internal_get_env(path);
    
    internal_get_dbi(oEnv->oDbi, indexName, dbi);

    if(*dbi == 0) {
        return TRASH_ERROR;
    }

    return TRASH_SUCCESS;
}

MDB_dbi *open_db(const char *table, const char *indexName) {
    struct OpenEnv *oEnv;
    struct OpenDbi *oDbi;

    MDB_cursor *cur;
    MDB_txn *txn;
    MDB_val key, data;
    MDB_dbi unDbi, result;
    
    int flags = 0;
    int rc;

    if(indexName == NULL) {
        return TRASH_ERROR;
    }

    if((oEnv = internal_get_env_path(table)) == NULL) {
        return TRASH_ERROR;
    }

    internal_find_dbi(oEnv->oDbi, indexName, &oDbi);
    
    if(oDbi != NULL) {
        result = oDbi->dbi;
        return result;
    }
    
    /**
     * @todo    validate that it is safe to get the len of this
    */
    key.mv_size = strlen(indexName);
    key.mv_data = indexName;

    VALID(mdb_txn_begin(oEnv->env, NULL, MDB_RDONLY, &txn));
    internal_get_dbi(oEnv->oDbi, INDICES, &unDbi);
    VALID(mdb_cursor_open(txn, unDbi, &cur));

    rc = mdb_cursor_get(cur, &key, &data, MDB_SET);
    if(rc == MDB_NOTFOUND) {
        /**
         * @todo       since this is a write txn, we need to set the semaphore
        */
        VALID(mdb_txn_begin(oEnv->env, NULL, 0, &txn));
        flags = MDB_CREATE;
    }

    VALID(mdb_dbi_open(txn, indexName, flags, &result));
    if(flags & MDB_CREATE) {
        mdb_txn_commit(txn);
    } else {
        mdb_txn_abort(txn);
    }

    internal_create_handle(&oDbi, indexName, result);
    pthread_rwlock_wlock(&dbiLock);
    HASH_ADD_STR(oEnv->oDbi, name, oDbi);
    pthread_rwlock_unlock(&dbiLock);
    
    return result;
}

static void open_all_dbs(MDB_txn *txn) {
    struct OpenEnv *oEnv;

    MDB_cursor *cur;
    MDB_dbi dbi;
    MDB_val key, val;
    
    int rc;

    oEnv = internal_get_env_txn(txn);
    internal_get_dbi(oEnv->oDbi, INDICES, &dbi);
    VALID(mdb_cursor_open(txn, dbi, &cur));

    pthread_rwlock_wlock(&dbiLock);
    while((rc = mdb_cursor_get(cur, &key, &val, MDB_NEXT)) == 0) {
        char *indexName = (char *)key.mv_data;

        VALID(mdb_dbi_open(txn, indexName, 0, &dbi));

        struct OpenDbi *handle;
        internal_create_handle(&handle, indexName, dbi);
        HASH_ADD_STR(oEnv->oDbi, name, handle);
    }
    pthread_rwlock_unlock(&dbiLock);

    mdb_cursor_close(cur);
}

/**
 * @note    this function will need to check if their is room for the desired db size
 * @todo    make this function better    
*/
static int internal_set_env_fields(MDB_env *env, size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    if(mdb_env_set_maxdbs(env, dbsize) != 0) {
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

static void internal_create_open_env(struct OpenEnv **oEnv, MDB_env *env) {
    MDB_dbi unDbi;

    *oEnv = malloc(sizeof(struct OpenEnv));
    if(*oEnv == NULL) {
        /**
         * @todo    handle this error some how
        */
    }

    const char *path;
    mdb_env_get_path(env, &path);

    (*oEnv)->env = env;
    (*oEnv)->path = path;
    (*oEnv)->oDbi = NULL;
    (*oEnv)->allOpen = false;

    sem_init(&(*oEnv)->writeSem, 0, 1);
}

static void internal_create_handle(struct OpenDbi **oDbi, const char *indexName, MDB_dbi dbi) {
    (*oDbi) = malloc(sizeof(struct OpenDbi));
    if((*oDbi) == NULL) {
        /**
         * @todo    handle error message here
        */
        exit(1);
    }

    (*oDbi)->name = indexName;
    (*oDbi)->dbi = dbi;
}

static void internal_find_dbi(struct OpenDbi *oDbi, const char *indexName, struct OpenDbi **out){
    pthread_rwlock_rdlock(&dbiLock);
    HASH_FIND_STR(oDbi, indexName, *out);
    pthread_rwlock_rdlock(&dbiLock);
}

static void internal_get_dbi(struct OpenDbi *oDbi, const char *indexName, MDB_dbi *dbi) {
    struct OpenDbi *out;
    internal_find_dbi(oDbi, indexName, &out);

    if(out == NULL) {
        *dbi = 0;
    }

    *dbi = out->dbi;
}

static struct OpenEnv *internal_get_env_txn(MDB_txn *txn) {
    struct OpenEnv *result;
    MDB_env *env;
    const char *path;
    
    env = mdb_txn_env(txn);
    mdb_env_get_path(env, &path);
    result = internal_get_env_path(path);

    return result;
}

static struct OpenEnv *internal_get_env_path(const char *path) {
    struct OpenEnv *result;

    pthread_rwlock_rdlock(&envLock);
    HASH_FIND_STR(oEnvs, path, result);
    pthread_rwlock_unlock(&envLock);

    return NULL;
}

static struct OpenEnv *internal_remove_env(const char *path) {
    struct OpenEnv *result;

    result = internal_get_env_path(path);
    if(result != NULL) {
        pthread_rwlock_wlock(&envLock);
        HASH_DEL(oEnvs, result);
        pthread_rwlock_unlock(&envLock);
    }

    return result;
}

static int valid(int rc, int line) {
    if(rc != 0) {
        /**
         * @todo    add some error here
         * @todo    use a logging setup
        */
        exit(1);
    }
}
