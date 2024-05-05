#include "db.h"
#include "pool/kiddiepool.h"
#include "nettwerk/server.h"
#include "../log/logger.h"

#define TRASH_PATH "../../../var/local/trashdb/"
#define DEFAULT_DATABASE_DIR TRASH_PATH "data/"
#define DEFAULT_LOG_DIR TRASH_PATH "logs/"

#define MAX_DIR_SIZE 1024
#define DEFAULT_DB_SIZE 10485760
#define DEFAULT_READERS 126
#define MAX_DBS 1

static int init_trashdb(TrashDB **trashDb, MDB_env *env, const char *path, char *port, size_t num_threads);
static int db_file(char *path);

/**
 * @todo    check to make sure env is not already open
 * @todo    handle invalid setters differently
 * @todo    validate num threads
 * @todo    handle error response from init
*/
void start_trash(TrashDB **db, Settings *settings) {
    MDB_env *env;
    mdb_env_create(env);

    if(mdb_env_set_maxdbs(env, settings->num_dbs) != 0) {
        env = NULL;
    }
    if(mdb_env_set_maxreaders(env, settings->num_threads) != 0) {
        env = NULL;
    }
    if(mdb_env_set_mapsize(env, settings->db_size) != 0) {
        env = NULL;
    }

    memset(*db, 0, sizeof(TrashDB));
    if(init_db(db, env, settings->path, settings->num_threads) != 0) {
        exit(1);
    };
}

/**
 * @todo    free the pool and all the content within it
*/
void kill_trash(TrashDB *db) {

}

static int init_trashdb(TrashDB **db, MDB_env *env, const char *path, char *port, size_t num_threads) {
    if(mdb_env_open(env, path, MDB_NOTLS | MDB_WRITEMAP, 0664) != 0) {
        mdb_env_close(env);
        return -1;
    };

    *db = (TrashDB *)malloc(sizeof(TrashDB));
    if (*db == NULL) {
        return -1;
    }

    Logger *logger;
    if(init_logger(&logger) != 0) {
        return -1;
    };

    KiddiePool *pool;
    if(init_pool(&pool, num_threads) != 0) {
        return -1;
    };

    TrashDB *tmp = db;
    tmp->env = env;
    tmp->logger = logger;
    tmp->pool = pool;
    tmp->nettwerk = NULL;

    if(init_server(db) != 0) {
        return -1;
    };

    return 0;
}

/**
 * Creates a directory where the db files exist.
 * 
 * @param   path    Path where the database file will be stored  
*/
static int db_file(char *path) {
    if(path == NULL) {
        path = DEFAULT_DATABASE_DIR;
    }

    char dir[MAX_DIR_SIZE];
    char *path_separator;

    #ifdef _WIN32
        if(_getcwd(dir, MAX_DIR_SIZE) == NULL) {
            return -1;
        };
        path_separator = "\\";
    #else
        if(getcwd(dir, MAX_DIR_SIZE) == NULL) {
            return -1;
        };
        path_separator = "/";
    #endif

    int i;
    for(i = strlen(dir); i >= 0; i--) {
        if(dir[i] == *path_separator) {
            break;
        }
    }
    if((MAX_DIR_SIZE - strlen(dir)) <= strlen(path)) {
        return -1;
    } else {
        int j;
        for(j = 0; j < strlen(path); j++) {
            dir[i+j] = path[j];
        }
        dir[i+j] = '\0';
    }

    #ifdef _WIN32
        return _mkdir(dir);
    #else
        return mkdir(dir);
    #endif
};

