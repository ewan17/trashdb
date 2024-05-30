#include "trashdb.h"
#include "utilities.h"
#include <assert.h>

int main(int argc, char *argv[]) {
    assert(trash_mkdir(TABLE_DIR, TABLE_DIR_LEN, 0755) == 0);

    // MDB_env *env;
    // env = open_env("test", 10485760, 10, 10);

    // assert(env != NULL);

    return 0;
}