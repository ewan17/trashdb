#include "trash.h"

/**
 * @todo    check to make sure env is not already open
 * @todo    handle invalid setters differently
 * @todo    validate num threads
*/
MDB_env *init_env(const char *path, size_t dbsize, unsigned int numdbs, unsigned int numthreads) {
    /**
     * @note    check to make sure the path is not already linked to an env    
     * @note    if path does not already exist, create one
    */
    MDB_env *env;
    mdb_env_create(env);

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
    
    if(mdb_env_open(env, path, MDB_NOTLS | MDB_WRITEMAP, 0664) != 0) {
        mdb_env_close(env);
        return -1;
    };

    return 0;
}

void close_env(MDB_env *env) {
    /**
     * @todo    check if env is currently stored in data structure, if so remove it
    */
    mdb_env_close(env);
}

/**
 * Creates a directory where the db files exist.
 * 
 * @param   path    Path where the database file will be stored  
*/
// static int env_file(char *path) {
//     if(path == NULL) {
//         path = DB_DIR;
//     }

//     char dir[MAX_DIR_SIZE];
//     char *path_separator;

//     if(getcwd(dir, MAX_DIR_SIZE) == NULL) {
//         return -1;
//     };
//     path_separator = "/";

//     int i;
//     for(i = strlen(dir); i >= 0; i--) {
//         if(dir[i] == *path_separator) {
//             break;
//         }
//     }
//     if((MAX_DIR_SIZE - strlen(dir)) <= strlen(path)) {
//         return -1;
//     } else {
//         int j;
//         for(j = 0; j < strlen(path); j++) {
//             dir[i+j] = path[j];
//         }
//         dir[i+j] = '\0';
//     }

//     return mkdir(dir);
// };

