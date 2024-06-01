#include "trashdb.h"
#include "utilities.h"

#define VALID(rc) \
    valid(rc, __LINE__) \

#ifdef DEBUG
int flag = 0;
#endif

struct OpenDbi {
    const char *name;
    MDB_dbi dbi;

    UT_hash_handle hh;
};

// Tracks all the currently open envs
struct OpenEnv {
    const char *tablename;
    MDB_env *env;

    // allows only one write txn for an env
    pthread_mutex_t writeMutex;
    pthread_cond_t writeCond;
    int writer;

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

static int internal_validate_tablename(char *tableName);

static int internal_set_env_fields(MDB_env *env, size_t dbsize, unsigned int numdbs, unsigned int numthreads);

static int internal_get_dbi(struct OpenDbi *oDbi, const char *indexName, MDB_dbi *dbi);

static void internal_create_open_env(struct OpenEnv **oEnv, MDB_env *env);
static void internal_create_handle(struct OpenDbi **oDbi, const char *index, MDB_dbi dbi);

static struct OpenEnv *internal_remove_env(const char *path);

/**
 * @todo    this function needs logging for errors
 * @todo    check the length of the table name or pass the length of the tablename as a parameter in the function
*/
MDB_env *open_env(char *tableName, size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    MDB_env *env;
    MDB_txn *txn;
    MDB_dbi unDbi;
    struct OpenEnv *oEnv;
    struct OpenDbi *oDbi;
    struct stat st;
    char filepath[256];
    int rc;

    if(internal_validate_tablename(tableName) != 0) {
        return NULL;
    }   

    if((env = get_env(tableName)) != NULL) {
        return env;
    }

    snprintf(filepath, sizeof(filepath), "%s%s/", TABLE_DIR, tableName);
    
    if(stat(filepath, &st) < 0) {
        if(errno = ENOENT) {
            size_t len = strlen(filepath);
            if(trash_mkdir(filepath, len, 0755) != 0) {
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    mdb_env_create(&env);
    if((rc = internal_set_env_fields(env, dbsize, numdbs, numthreads)) != TRASH_SUCCESS) {
        return NULL;
    }

    if((rc = mdb_env_open(env, filepath, 0, 0664)) != 0) {
        /**
         * @todo    handle the different possible errors for an invalid open
        */ 
        mdb_env_close(env);
        return NULL;
    };

    // setup open env struct
    internal_create_open_env(&oEnv, env);

    // open the unnamed db
    VALID(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
    VALID(mdb_dbi_open(txn, NULL, 0, &unDbi));
    mdb_txn_abort(txn);

    // create unnamed db key/value
    internal_create_handle(&oDbi, INDICES, unDbi);
    HASH_ADD_STR(oEnv->oDbi, name, oDbi);

    // add open env struct to env hash map
    pthread_rwlock_wrlock(&envLock);
    HASH_ADD_STR(oEnvs, tablename, oEnv);
    pthread_rwlock_unlock(&envLock);

    return env;
}

MDB_env *get_env(const char *tableName) {
    struct OpenEnv *result;

    result = get_env_from_tablename(tableName);

    if(result != NULL) {
        return result->env;
    }

    return NULL;
}

/**
 * @note    this function is not correct
 * @todo    make sure all transactions, databases, and cursors are closed before calling this function
*/
void close_env(MDB_env *env) {
    if(env == NULL) {
        return;
    }
    
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

bool is_all_open(OpenEnv *oEnv) {
    return oEnv->allOpen;
}

void open_all(MDB_txn *txn, OpenEnv *oEnv) {
    MDB_cursor *cur;
    MDB_dbi dbi;
    MDB_val key, val;
    int rc;

    internal_get_dbi(oEnv->oDbi, INDICES, &dbi);
    VALID(mdb_cursor_open(txn, dbi, &cur));

    pthread_rwlock_wrlock(&dbiLock);
    while((rc = mdb_cursor_get(cur, &key, &val, MDB_NEXT)) == 0) {
        struct OpenDbi *handle;
        char *indexName;
        
        indexName = (char *)key.mv_data;

        HASH_FIND_STR(oEnv->oDbi, indexName, handle);

        if(handle == NULL) {
            VALID(mdb_dbi_open(txn, indexName, 0, &dbi));

            internal_create_handle(&handle, indexName, dbi);
            HASH_ADD_STR(oEnv->oDbi, name, handle);
        }
    }
    pthread_rwlock_unlock(&dbiLock);

    oEnv->allOpen = true;
    mdb_cursor_close(cur);
}

int open_db(MDB_txn *txn, OpenEnv *oEnv, const char *indexName, MDB_dbi *dbi) {
    struct OpenDbi *oDbi;
    MDB_cursor *cur;
    MDB_txn *tempTxn = txn;
    MDB_val key, data;
    MDB_dbi result;
    int flags = 0;
    int rc;

    // dbi may already exist
    rc = internal_get_dbi(oEnv->oDbi, indexName, &result);
    if(rc == 0) {
        #ifdef DEBUG
        flag = 1;
        #endif
        goto bottom;
    }

    internal_get_dbi(oEnv->oDbi, INDICES, &result);
    VALID(mdb_cursor_open(txn, result, &cur));
    
    key.mv_size = strlen(indexName);
    key.mv_data = indexName;

    rc = mdb_cursor_get(cur, &key, &data, MDB_SET);
    if(rc == MDB_NOTFOUND) {
        // stop read txn since we are creating a write txn
        mdb_txn_reset(txn);

        begin_write_txn(oEnv);
        // create write txn
        VALID(mdb_txn_begin(oEnv->env, NULL, 0, &tempTxn));
        flags |= MDB_CREATE;
    }

    VALID(mdb_dbi_open(tempTxn, indexName, flags, &result));
    if(flags & MDB_CREATE) {
        // push changes to db
        mdb_txn_commit(tempTxn);
        end_write_txn(oEnv);
    }

    // add the OpenDbi struct to the hashmap
    internal_create_handle(&oDbi, indexName, result);
    pthread_rwlock_wrlock(&dbiLock);
    HASH_ADD_STR(oEnv->oDbi, name, oDbi);
    pthread_rwlock_unlock(&dbiLock);

    // start up the read txn again if we had to pause for a write txn
    if(flags & MDB_CREATE) {
        mdb_txn_renew(txn);
        mdb_cursor_renew(txn, cur);
    }

bottom:
    *dbi = result;
    return TRASH_SUCCESS;
}

int get_dbi(const char *tableName, const char *indexName, MDB_dbi *dbi) {
    struct OpenEnv *oEnv;
    int rc;

    oEnv = get_env_from_tablename(tableName); 

    rc = internal_get_dbi(oEnv->oDbi, indexName, dbi);
    return rc;
}

OpenEnv *get_env_from_cur(MDB_cursor *cur) {
    MDB_txn *txn;
    OpenEnv *result;
    
    txn = mdb_cursor_txn(cur);
    result = get_env_from_txn(txn);
    return result;
}

OpenEnv *get_env_from_txn(MDB_txn *txn) {
    MDB_env *env;
    const char *path;
    
    env = mdb_txn_env(txn);
    mdb_env_get_path(env, &path);
    
    return get_env_from_path(path);
}

OpenEnv *get_env_from_tablename(const char *tableName) {
    OpenEnv *result;
    
    pthread_rwlock_rdlock(&envLock);
    HASH_FIND_STR(oEnvs, tableName, result);
    pthread_rwlock_unlock(&envLock);

    return result;
}

OpenEnv *get_env_from_path(const char *path) {
    char *tableName;

    tablename_from_path(path, &tableName);

    return get_env_from_tablename(tableName);
}

/**
 * @todo    path must be null terminated
*/
void tablename_from_path(const char *path, char **tablename) {
    char temp[MAX_DIR_SIZE];
    size_t len;
    
    len = strlen(path);
    if(len > MAX_DIR_SIZE - 1 || len < 3) {
        goto error;
    }
    snprintf(temp, MAX_DIR_SIZE, "%s", path);

    size_t index = len - 1;
    if(temp[index] == '/') {
        index--;
    }

    size_t tableNameLen = 0;
    while(temp[index] != '/' && index > 0) {
        tableNameLen++;
        index--;
    }
    index++;

    if (index == 0 && temp[index] != '/') {
        goto error;
    }

    *tablename = (char *) malloc(tableNameLen * sizeof(char));
    if(*tablename == NULL) {
        goto error;
    }

    memcpy(*tablename, &temp[index], tableNameLen);
    (*tablename)[tableNameLen] = '\0';
    return;

error:
    *tablename = NULL;
}

void begin_write_txn(OpenEnv *oEnv) {
    pthread_mutex_lock(&oEnv->writeMutex);
    while (oEnv->writer) {
        pthread_cond_wait(&oEnv->writeCond, &oEnv->writeMutex);
    }
    oEnv->writer = 1;
    pthread_mutex_unlock(&oEnv->writeMutex);
}

void end_write_txn(OpenEnv *oEnv) {
    pthread_mutex_lock(&oEnv->writeMutex);
    oEnv->writer = 0;
    /**
     * @note    for now we will broadcast even though signal would be more efficient right now
     * @todo    add other conditions that the writers need to follow. Such as labeling certain writers as higher priority with flags
    */
    pthread_cond_broadcast(&oEnv->writeCond);
    pthread_mutex_unlock(&oEnv->writeMutex);
}

/**
 * @note    currently broken
*/
static int internal_validate_tablename(char *tableName) {
    regex_t regex;
    int regExp;

    /**
     * @note    this does not work "^[A-Za-z._-]+$"
     * @todo    fix this later
    */
    regExp = regcomp(&regex, "", 0);
    if(regExp != 0) {
        return TRASH_ERROR;
    }

    regExp = regexec(&regex, tableName, 0, NULL, 0);
    if(tableName == NULL || regExp == REG_NOMATCH) {
        regfree(&regex);
        return TRASH_ERROR;
    }

    regfree(&regex);

    return TRASH_SUCCESS;
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

static void internal_create_open_env(OpenEnv **oEnv, MDB_env *env) {
    *oEnv = malloc(sizeof(OpenEnv));
    if(*oEnv == NULL) {
        /**
         * @todo    handle this error some how
        */
    }

    const char *path;
    char *tablename;
    mdb_env_get_path(env, &path);
    tablename_from_path(path, &tablename);

    (*oEnv)->env = env;
    (*oEnv)->tablename = tablename;
    (*oEnv)->oDbi = NULL;
    (*oEnv)->allOpen = false;

    pthread_mutex_init(&(*oEnv)->writeMutex, NULL);
    pthread_cond_init(&(*oEnv)->writeCond, NULL);
    (*oEnv)->writer = 0;
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
    pthread_rwlock_unlock(&dbiLock);
}

static int internal_get_dbi(struct OpenDbi *oDbi, const char *indexName, MDB_dbi *dbi) {
    struct OpenDbi *out;
    internal_find_dbi(oDbi, indexName, &out);

    if(out == NULL) {
        return TRASH_ERROR;
    }

    *dbi = out->dbi;
    return TRASH_SUCCESS;
}

static OpenEnv *internal_remove_env(const char *path) {
    OpenEnv *result;

    result = get_env_from_path(path);
    if(result != NULL) {
        pthread_rwlock_wrlock(&envLock);
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
    
    return 0;
}
