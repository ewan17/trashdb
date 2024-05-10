#include <stddef.h>

#include "include/lmdb.h"

#define DB_SIZE 10485760

int open_env(MDB_env *env, const char *path) {
    if(mdb_env_open(env, path, MDB_NOTLS | MDB_WRITEMAP, 0664) != 0) {
        mdb_env_close(env);
        return -1;
    };

    return 0;
}

int main(int argc, char *argv[]) {
    MDB_env *env;
    mdb_env_create(&env);
    open_env(env, "/home/comet/trashdb/testdir");

    MDB_env *env2;
    mdb_env_create(&env);
    open_env(env2, "/home/comet/trashdb/testdir2");
}