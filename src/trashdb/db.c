#include "trashdb.h"

static INIT_LIST(oEnvs);
static INIT_LIST(oDbs);

static pthread_mutex_t oMutex = PTHREAD_MUTEX_INITIALIZER;

// Tracks all the currently open envs
struct OpenEnv {
    struct IList il;
    MDB_env *env;
    const char *envpath;
    unsigned int numdbs;
    size_t dbsize;
};

// Tracks all the curently open dbs
struct OpenDb {
    struct IList il;
    MDB_env *env;
    const char *dbname;
    unsigned int flags;
    MDB_dbi *dbi;
};

static MDB_env *create_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads);

static struct OpenEnv *get_open_env(const char *path);
static struct OpenEnv *remove_env(const char *path);

static bool is_db_open(const char *path);

int start_trash_txn(MDB_env *env, unsigned int flags) {
    mdb_txn_begin(, NULL, );
}

/**
 * @todo    do not open the same database multiple times, only once    
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
    oEnv = (struct OpenEnv *)malloc(1, sizeof(struct OpenEnv));

    oEnv->env = env;
    oEnv->envpath = path;
    oEnv->dbsize = dbsize;
    oEnv->numdbs = numdbs;

    append(&oEnvs, &oEnv->il);

    return env;
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

static MDB_env *create_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
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
 * @todo    strcmp can cause buffer overflow if not careful. fix later
*/ 
static struct OpenEnv *get_open_env(const char *path) {
    struct IList *curr;
    for_each(&oEnvs.head, curr) {
        struct OpenEnv *oEnv;
        oEnv = CONTAINER_OF(curr, struct OpenEnv, il);

        int rc = strcmp(path, oEnv->envpath);
        if(rc == 0) {
           return oEnv; 
        }
    }
    return NULL;
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

static struct OpenDb *remove_db(const char *path) {
    struct IList *curr;
    for_each(&oDbs.head, curr) {
        struct OpenDb *oDb;
        oDb = CONTAINER_OF(curr, struct OpenDb, il);

        int rc = strcmp(path, oDb->dbname);
        if(rc == 0) {
           return ilist_remove(curr); 
        }
    }
    return NULL;
}