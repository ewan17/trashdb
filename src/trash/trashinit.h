#ifndef TRASHINIT_H
#define TRASHINIT_H

MDB_env *init_env(const char *path, size_t dbsize, unsigned int numdbs, unsigned int numthreads);

int open_env(MDB_env *env, const char *path);

void close_env(MDB_env *env);

#endif //TRASHINIT_H
