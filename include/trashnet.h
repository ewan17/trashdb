#ifndef TRASHNET_H
#define TRASHNET_H

#include <sys/socket.h>
#include <signal.h>
#include <event2/event.h>

#include "global.h"

#include "server.h"
#include "message.h"
#include "protocol.h"
#include "buffy.h"

extern struct event_base *serverBase;
extern pthread_mutex_t base_mutex;

#endif //TRASHNET_H