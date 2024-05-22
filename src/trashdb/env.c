#include "trashdb.h"
#include <semaphore.h>

#define VALID(rc) \
    valid(rc, __LINE__) \

struct OpenDbi {
    const char *index;
    MDB_dbi dbi;

    UT_hash_handle hh;
};

// Tracks all the currently open envs
struct OpenEnv {
    const char *path;
    MDB_env *env;

    sem_t writeSem;

    // set to NULL initially
    struct OpenDbi *oDbi;

    UT_hash_handle hh;
};

static struct OpenEnv *oEnvs = NULL;

static pthread_rwlock_t envLock, dbiLock = PTHREAD_RWLOCK_INITIALIZER;

/**
 * @note    this function needs work
 * @todo    add logging for errors and figure out what the return errors should be if any
*/
static int valid(int rc, int line);

static MDB_env *internal_create_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads);

static struct OpenDbi *internal_create_handle(const char *index, MDB_dbi dbi);

static struct OpenEnv *internal_get_env_txn(MDB_txn *txn);
static struct OpenEnv *internal_get_env_path(const char *path);
static struct OpenEnv *internal_remove_env(const char *path);

/**
 * @todo    need to open all the dbi's for the env as well.
 * @note    sub databases that do not exist will need to be created with a write txn. 
 * @note    if the env(table) is not new, than we can open all the sub databases using a read txn. 
*/
MDB_env *open_env(const char *path, size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    MDB_env *env;
    struct stat st;

    if(path == NULL || stat(path, &st) < 0) {
        return -1;
    }

    if(S_ISDIR(st.st_mode)) {
        if((env = get_open_env(path)) != NULL) {
            return env;
        }
    } else {
        /**
         * @note    if it made it here, the path could be a file and not a dir
         * @todo    create dir
         * @todo    need to make a dir where the users table files are stored
        */
    }

    env = create_env(dbsize, numdbs, numthreads);

    int rc;
    if((rc = mdb_env_open(env, path, MDB_WRITEMAP, 0664)) != 0) {
        mdb_env_close(env);

        /**
         * @todo    handle the different possible errors with rc
        */ 
        return -1;
    };

    struct OpenEnv *oEnv;
    oEnv = (struct OpenEnv *)malloc(1 * sizeof(struct OpenEnv));

    oEnv->env = env;

    append(&oEnvs, &oEnv->il);

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
    result = internal_get_env(path);

    if(result != NULL) {
        return result->env;
    }

    return NULL;
}

MDB_dbi *get_dbi(MDB_env *env, const char *index) {
    if(env == NULL) {
        return -1;
    }

    struct OpenEnv *oEnv;
    struct OpenDbi *oDbi;
    MDB_dbi dbi;
    const char *path;

    mdb_env_get_path(env, &path);
    oEnv = internal_get_env(path);
    
    pthread_rwlock_rdlock(&dbiLock);
    HASH_FIND_INT(oEnv->oDbi, path, oDbi);
    pthread_rwlock_rdlock(&dbiLock);

    dbi = (oDbi == NULL) ? -1 : oDbi->dbi;
    return dbi;
}

int open_all_dbs(MDB_txn *txn) {
    if(txn == NULL) {
        return -1;
    }

    struct OpenEnv *oEnv;
    struct OpenDbi *handle;

    MDB_cursor *curr;
    MDB_dbi unDbi, xDbi;
    MDB_val key, val;

    int rc;

    oEnv = internal_get_env_txn(txn);

    /**
     * @todo    figure out how to handle the error messages for the opening of a db
    */
    VALID(mdb_dbi_open(txn, NULL, 0, &unDbi));
    VALID(mdb_cursor_open(txn, unDbi, &curr));

    pthread_rwlock_wlock(&dbiLock);
    while((rc = mdb_cursor_get(curr, &key, &val, MDB_NEXT)) == 0) {
        char *index = (char *)key.mv_data;
        // for now there will be no flags set for opening an index
        VALID(mdb_dbi_open(txn, index, 0, &xDbi));

        handle = internal_create_handle(index, xDbi);
        if(handle == NULL) {
            /**
             * @todo    handle error 
             * @todo    add logging error message
            */
           exit(1);
        }
        HASH_ADD_STR(oEnv->oDbi, index, handle);
    }
    pthread_rwlock_unlock(&dbiLock);
}

MDB_dbi *open_db(MDB_txn *txn, const char *index, int flags) {
    // check the env to see if db is already open
    // if not create one
    pthread_rwlock_wlock(&dbiLock);

    pthread_rwlock_unlock(&dbiLock);
}

static MDB_env *internal_create_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    MDB_env *env;
    mdb_env_create(&env);

    if(mdb_env_set_maxdbs(env, dbsize) != 0) {
        env = NULL;
    }
    if(mdb_env_set_maxreaders(env, numthreads) != 0) {
        env = NULL;
    }
    if(mdb_env_set_mapsize(env, dbsize) != 0) {
        env = NULL;
    }

    return env;
}

static struct OpenDbi *internal_create_handle(const char *index, MDB_dbi dbi) {
    struct OpenDbi *handle;
    handle = malloc(sizeof(struct OpenDbi));
    if(handle == NULL) {
        return NULL;
    }

    handle->index = index;
    handle->dbi = dbi;

    return handle;
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
    HASH_FIND_PTR(oEnvs, path, result);
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