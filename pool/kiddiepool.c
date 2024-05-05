#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "kiddiepool.h"

struct Node {
    work_func wf;
    void *work_arg;
    struct Node *prev;
};

struct Q {
    struct Node *head;
    struct Node *tail;
    size_t length;
    size_t capacity;
};

typedef struct KiddieGroup {
    pthread_mutex_t mutex_kg;
    pthread_cond_t cond_kg;
    pthread_cond_t kill_cond_kg;
    kg_id id;
    size_t thread_count;
    size_t active_threads;
    int kill;
    pthread_t *threads;
    struct Q *q;
    KiddiePool *pool;
} KiddieGroup;

typedef struct KiddiePool {
    pthread_mutex_t mutex_kp;
    size_t num_groups;
    KiddieGroup **kiddie_groups;
} KiddiePool;

static void destroy_group(KiddieGroup **kg);
static int init_group(KiddieGroup **kg, KiddiePool *kp);
static void *thread_function(void *pool);

static int q_init(struct Q **q, size_t capacity);
static void q_destroy(struct Q **q);
static void q_clear(struct Q *q);
static void q_append(struct Q *q, struct Node *node);
static struct Node *q_fetch(struct Q *q);
static int q_length(struct Q *q);

static void node_kill(struct Node *node);

/* ----- Pool ----- */
int init_pool(KiddiePool **pool, size_t num_groups) {
    if(pool == NULL) {
        return -1;
    }

    if(num_groups == 0) {
        num_groups = DEFAULT_NUM_GROUPS;
    }

    *pool = (KiddiePool *)malloc(sizeof(KiddiePool));
    if(*pool == NULL) {
        fprintf(stderr, "Failed to allocate memory for KiddiePool\n");
        return -1;
    }

    KiddiePool *kp = *pool;
    kp->kiddie_groups = (KiddieGroup **)malloc(num_groups * sizeof(KiddieGroup *));
	if (kp->kiddie_groups == NULL){
        fprintf(stderr, "Failed to allocate memory for KiddieGroups\n");
		free(kp);
        kp = NULL;
		return -1;
	}
    kp->num_groups = num_groups;

    pthread_mutex_init(&(kp->mutex_kp), NULL);
    
    for (size_t i = 0; i < num_groups; i++)
    {    
        int rc;
        rc = init_group(&(kp->kiddie_groups[i]), kp);

        if(rc != 0) {
            /**
             * @note    all groups get initialized or none of them
            */
            destroy_pool(*pool);
            *pool = NULL;
            return -1;
        };

        kp->kiddie_groups[i]->id = i;
    }
    return 0;
}

int add_work(KiddiePool *pool, kg_id id, work_func wf, void *work_arg) {
    if(wf == NULL) {
        return -1;
    }
    pthread_mutex_lock(&(pool->mutex_kp));
    if(id == 0 || id > (pool->num_groups - 1)) {
        pthread_mutex_unlock(&(pool->mutex_kp));
        return -1; 
    }
    KiddieGroup *kg = pool->kiddie_groups[id];
    pthread_mutex_unlock(&(pool->mutex_kp));

    struct Node *node;
    node = (struct Node *)malloc(sizeof(struct Node));
    if(node == NULL) {
        fprintf(stderr, "Failed to allocate memory for KiddieGroups\n");
        return -1;
    }
    node->wf = wf;
    node->work_arg = work_arg;
    node->prev = NULL;

    pthread_mutex_lock(&(kg->mutex_kg));
    q_append(kg->q, node);
    pthread_cond_signal(&(kg->cond_kg));
    pthread_mutex_unlock(&(kg->mutex_kg));

    return 0;
}

void destroy_pool(KiddiePool *pool) {
    if(pool != NULL) {
        pthread_mutex_lock(&(pool->mutex_kp));
        for (size_t i = 0; i < pool->num_groups; i++)
        {
            destroy_group(&(pool->kiddie_groups[i]));
        }
        free(pool->kiddie_groups);
        pool->kiddie_groups = NULL;

        pthread_mutex_destroy(&(pool->mutex_kp));

        free(pool);
    }
}

static int init_group(KiddieGroup **kg, KiddiePool *kp) {
    if(kg == NULL) {
        return -1;
    }

    *kg = (KiddieGroup *)malloc(sizeof(KiddieGroup));
    if(*kg == NULL) {
        return -1;
    }
    KiddieGroup *tmp_kg = *kg;

    tmp_kg->threads = (pthread_t *)malloc(DEFAULT_THREADS_PER_GROUP * sizeof(pthread_t));
	if (tmp_kg->threads == NULL){
		free(*kg);
        *kg = NULL;
		return -1;
	}
    
    /**
     * @note    queue size can be changed later. Picked random size.
    */
    struct Q *q;
    if(q_init(&q, 1024) != 0) {
        free(tmp_kg->threads);
        free(*kg);
        *kg = NULL;
        return -1;
    };

    tmp_kg->q = q;
    tmp_kg->kill = 0;
    tmp_kg->pool = kp;
    tmp_kg->thread_count = DEFAULT_THREADS_PER_GROUP;
    tmp_kg->active_threads = 0;
    pthread_mutex_init(&(tmp_kg->mutex_kg), NULL);
    pthread_cond_init(&(tmp_kg->kill_cond_kg), NULL);
    pthread_cond_init(&(tmp_kg->cond_kg), NULL);

    for (size_t i = 0; i < DEFAULT_THREADS_PER_GROUP; i++)
    {
        if(pthread_create(&(tmp_kg->threads[i]), NULL, thread_function, tmp_kg) != 0) {
            return -1;
        }
    }
    return 0;
}

/**
 * @todo    make dynamic so you only destroy some threads.
 * @note    currently destroys all threads.
*/
static void destroy_group(KiddieGroup **kg) {
    if(kg != NULL) {
        KiddieGroup *tmp_kg = *kg;
        size_t total_thread_count;

        pthread_mutex_lock(&(tmp_kg->mutex_kg));
        total_thread_count = tmp_kg->thread_count;
        tmp_kg->kill = 1;

        while(tmp_kg->thread_count != 0) {
            pthread_cond_broadcast(&(tmp_kg->cond_kg));
            pthread_cond_wait(&(tmp_kg->kill_cond_kg), &(tmp_kg->mutex_kg));
        }

        for (size_t i = 0; i < total_thread_count; i++) {
            pthread_join(tmp_kg->threads[i], NULL);
        }

        free(tmp_kg->threads);
        tmp_kg->threads = NULL;

        q_destroy(&(tmp_kg->q));

        pthread_mutex_destroy(&(tmp_kg->mutex_kg));
        pthread_cond_destroy(&(tmp_kg->cond_kg));
        pthread_cond_destroy(&(tmp_kg->kill_cond_kg));

        free(*kg);
        *kg = NULL;
    };
}

static void *thread_function(void *arg) {
    KiddieGroup *kg;
    kg = (KiddieGroup *)arg;

    while(1) {
        struct Node *node;

        pthread_mutex_lock(&(kg->mutex_kg));    
        int tasks = q_length(kg->q);
        while(tasks == 0 && !kg->kill) {
            pthread_cond_wait(&(kg->cond_kg), &(kg->mutex_kg));
            tasks = q_length(kg->q);
        }

        if(kg->kill && tasks == 0) {
            kg->thread_count--;
            if(kg->thread_count == 0) {
                pthread_cond_signal(&(kg->kill_cond_kg));
            }
            pthread_mutex_unlock(&(kg->mutex_kg));
            break;
        }
        
        node = q_fetch(kg->q);
        kg->active_threads++;
        pthread_mutex_unlock(&(kg->mutex_kg));

        work_func func = node->wf;
        void *arg = node->work_arg;
        func(arg);
        node_kill(node);
        node = NULL;

        pthread_mutex_lock(&(kg->mutex_kg));
        kg->active_threads--;
        pthread_mutex_unlock(&(kg->mutex_kg));
    }

    pthread_exit(NULL);
}

/*----- Q ----- */
static int q_init(struct Q **q, size_t capacity) {
    if(q == NULL) {
        return -1;
    }

    *q = (struct Q *)malloc(sizeof(struct Q));
    if(*q == NULL) {
        perror("malloc error at queue");
        return -1;
    }

    struct Q *tmp_q = *q;
    tmp_q->head = NULL;
    tmp_q->tail = NULL;
    tmp_q->length = 0;
    tmp_q->capacity = capacity;

    return 0;
};

static void q_destroy(struct Q **q) {
    if(q != NULL) {
        q_clear(*q);
        free(*q);
        *q = NULL;
    }
};

/**
 * @todo    needs to signal to the threads to execute the items in the q.
*/
static void q_clear(struct Q *q) {

};

static void q_append(struct Q *q, struct Node *node) {
    if(q != NULL && node != NULL) {
        if(q->length < (q->capacity - 1)) {
            node->prev = NULL;
            if(q->head == NULL) {
                q->head = q->tail = node;
            } else {
                q->tail->prev = node;
                q->tail = node;
            }
            (q->length)++;
        }
    }
};

static struct Node *q_fetch(struct Q *q) {
    if(q != NULL || q->head != NULL) {
        if(q->head == q->tail) {
            q->tail = NULL;
        }
        struct Node *tmp = q->head;
        q->head = q->head->prev;
        tmp->prev = NULL;
        (q->length)--;

        return tmp;
    }
    return NULL;
};

static int q_length(struct Q *q) {
    return q->length;
}

/**
 * @note    call the work function and then you can free the node
 * @note    it is the work functions job to free and set the work arg to NULL
*/
static void node_kill(struct Node *node) {
    if(node != NULL && node->prev == NULL) {
        free(node);
    }
}
