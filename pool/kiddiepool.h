#ifndef KIDDIEPOOL_H
#define KIDDIEPOOL_H

#define DEFAULT_NUM_GROUPS 16
#define DEFAULT_THREADS_PER_GROUP 2 

#include <pthread.h>

typedef size_t kg_id;
typedef void (*work_func)(void *work_arg);

typedef struct KiddiePool KiddiePool;
typedef struct KiddieGroup KiddieGroup;
 
int init_pool(KiddiePool **pool, size_t num_groups);

int add_work(KiddiePool *pool, kg_id id, work_func wf, void *work_arg);

void destroy_pool(KiddiePool *pool);

#endif //KIDDIEPOOL_H