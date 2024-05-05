#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdbool.h>
#include <sys/socket.h>
#include <stdio.h>

#define TRASH_SUCCESS (0)
#define TRASH_ERROR (-1)
#define TRASH_EOF (-2)

extern struct event_base *serverBase;
extern pthread_mutex_t base_mutex;

#endif //GLOBAL_H
