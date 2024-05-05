#ifndef SERVER_H
#define SERVER_H

#include "../trash/global.h"

#define DEFAULT_BACK_LOG 100
#define MAX_LISTENER_SOCKETS 1

// should be larger, change later
#define MAX_CONNECTIONS 100

#define SOCKET_NAME "%s/trash.socket"

void run_server();
void kill_server();
int init_server(sa_family_t sockFam, const char *hostname, char *port, bool isSockAbstract);

#endif //SERVER_H