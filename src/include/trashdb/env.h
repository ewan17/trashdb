#ifndef ENV_H
#define ENV_H


MDB_env *open_env(size_t dbsize, unsigned int numdbs, unsigned int numthreads);


void close_all_dbs_env();

#endif //ENV_H