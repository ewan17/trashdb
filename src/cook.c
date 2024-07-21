#include "trashdb.h"

// signle write txn
static TGroup *wr = NULL;
// multiple read txns
static LL rdrs;

struct CGrps {
    IL move;

    TGroup *tg;
};

void init_thread_pool() {
    /**
     * @note    fix later and do not hard code it
     * @note    on a 16 core machine
     * 
     * -- Other --
     * @note    1 thread listening for connections
     * @note    1 thread for logging
     * 
     * -- Pool --
     * @note    1 thread pool manager
     * @note    1 thread for writing
     * @note    12 threads for readers
     */
    init_pool(tp, 14);
}

void init_thread_groups() {
    // create the writer group
    wr = add_group(tp, 1, 1, GROUP_FIXED);
    // setup the list of reader groups
    init_list(&rdrs);

    /**
     * @todo    fix later so it is not hard coded values
     */
    for (int i = 0; i < 3; i++) {
        struct CGrps *cg;
        /**
         * @todo    change the min and max later
         * @todo    do not hard code the group count either
         */
        cg = (struct CGrps *)malloc(sizeof(*cg));
        assert(cg != NULL);

        init_il(&cg->move);

        // create group of threads
        cg->tg = add_group(tp, 2, 4, GROUP_DYNAMIC);
        // append group to the 
        list_append(&rdrs, &cg->move);
    }
}

void cook_func(void *arg) {
    // setup the reader txns for the thread
    init_thread_local_readers();
}