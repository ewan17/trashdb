#include "trash.h"

static INIT_LIST(oEnvs);
static INIT_LIST(oDbs);

static pthread_mutex_t oMutex = PTHREAD_MUTEX_INITIALIZER;

// Tracks all the currently open envs
struct OpenEnv {
    struct IList il;
    MDB_env *env;
    const char *envpath
};

// Tracks all the curently open dbs
struct OpenDb {
    struct IList il;
    MDB_env *env;
    const char *dbname;
    unsigned int flags;
    MDB_dbi *dbi;
};

static bool is_env_open(const char *path);
static struct OpenEnv *remove_env(const char *path);

static bool is_db_open();

/**
 * @todo    check to make sure env is not already open
 * @todo    handle invalid setters differently
 * @todo    validate num threads
*/
MDB_env *create_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    /**
     * @note    check to make sure the path is not already linked to an env    
     * @note    if path does not already exist, create one
    */
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

/**
 * @todo    do not open the same database multiple times, only once    
*/
int open_env(MDB_env *env, const char *path) {
    if(env == NULL) {
        return -1;
    }

    if(is_env_open(path)) {
        return 0;
    }

    if(!is_file(path)) {
        /**
         * @todo    create file if it does not exist
        */
    }

    int rc;
    if((rc = mdb_env_open(env, path, MDB_NOTLS | MDB_WRITEMAP, 0664)) != 0) {
        mdb_env_close(env);

        /**
         * @todo    handle the different possible errors with rc
        */ 
        return -1;
    };

    struct OpenEnv *oenv;
    oenv = (struct OpenEnv *)malloc(1, sizeof(struct OpenEnv));

    oenv->env = env;
    oenv->envpath = path;

    append(&oEnvs, &oenv->il);

    return 0;
}

void close_env(MDB_env *env) {
    if(env != NULL) {
        struct OpenEnv *oEnv;
        const char *path;

        mdb_env_get_path(env, &path);
        oEnv = remove_env(path);

        if(oEnv != NULL) {
            mdb_env_close(env);
            free(oEnv);
        }
    }
}

/**
 * @todo    strcmp can cause buffer overflow if not careful. fix later
*/ 
static bool is_env_open(const char *path) {
    struct IList *curr;
    for_each(&oEnvs.head, curr) {
        struct OpenEnv *oEnv;
        oEnv = CONTAINER_OF(curr, struct OpenEnv, il);

        int rc = strcmp(path, oEnv->envpath);
        if(rc == 0) {
           return true; 
        }
    }
    return false;
}

static struct OpenEnv *remove_env(const char *path) {
    struct IList *curr;
    for_each(&oEnvs.head, curr) {
        struct OpenEnv *oEnv;
        oEnv = CONTAINER_OF(curr, struct OpenEnv, il);

        int rc = strcmp(path, oEnv->envpath);
        if(rc == 0) {
           return ilist_remove(curr); 
        }
    }
    return NULL;
}

/**
 * @todo    loop through open dbs and check if db is open
*/
static bool is_db_open(const char *path) {
    struct IList *curr;
    for_each(&oEnvs.head, curr) {
        struct OpenDb *oDb;
        oDb = CONTAINER_OF(curr, struct OpenDb, il);

        int rc = strcmp(path, oDb->dbname);
        if(rc == 0) {
           return true; 
        }
    }
    return false;
}