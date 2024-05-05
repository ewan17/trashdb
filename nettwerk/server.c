#include <sys/types.h>                                                                                                      
#include <sys/socket.h>
#include <stdbool.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <event2/event.h>

#include "buffy.h"
#include "server.h"

struct event_base *serverBase;
pthread_mutex_t base_mutex;

static struct List listeners;

struct Listener {
    struct IList il;

    int fd;
	bool active;
	struct event *ev;
    struct TrashAddr localaddr;
};

static void clean_listening_sockets();
static int init_server_sock(sa_family_t sockFam, const char *hostName, char *port, bool isSockAbstract);
static int create_tcp_sock(int *fd, struct TrashAddr *addr, sa_family_t sockFam, const char *hostName, char *port);
static int create_unix_sock(int *fd, struct TrashAddr *addr, const char *sockDir, bool isSockAbstract);
static void start_accepting_connections();
static void on_connect(evutil_socket_t sock, short flags, void *arg);

/**
 * @todo    add sig handler
*/
void run_server() {
    /**
     *  libevent sighandler stuff goes 
    */ 
    serverBase = event_base_new();
    if(serverBase != NULL) {
        start_accepting_connections();

        event_base_dispatch(serverBase);
    }
}

/**
 * @todo    change parameters
 * @todo    make void method calls, do not make them return ints
*/
int init_server(sa_family_t sockFam, const char *hostname, char *port, bool isSockAbstract) {
    INIT_LIST(&listeners);

    if(init_server_sock(sockFam, hostname, port, isSockAbstract) != 0) {
        return TRASH_ERROR;
    }

    return TRASH_SUCCESS;
}

void kill_server() {
    event_base_loopexit(serverBase, NULL);
    event_base_free(serverBase);
}

/**
 * @todo    needs fixing
*/
static void clean_listening_sockets() {
    struct IList *ilist;
    while((ilist = pop(&listeners)) != NULL) {
        struct Listener *ls;
        ls = CONTAINER_OF(ilist, struct Listener, il);

        event_del(ls->ev);
        evutil_closesocket(ls->fd);

        ls->fd = -1;
        ls->active = false;

        if(is_unix_socket(&ls->localaddr)) {
            struct sockaddr_un *addr = (struct sockaddr_un *) &ls->localaddr.addr;
            if(addr->sun_path[0] != '\0') {
                unlink(addr->sun_path);
            }
        }
        
        free(ls);
    }
}

static int init_server_sock(sa_family_t sockFam, const char *hostName, char *port, bool isSockAbstract) {
    struct Listener *ls;  
    int fd = -1;

    ls = (struct Listener *)calloc(1, sizeof(struct Listener));
    if(ls == NULL) {
        goto error;
    }            
    ls->active = false;

    switch (sockFam) {
        case AF_UNIX:
            ls->localaddr.addr.ss_family = AF_UNIX;
            if(create_unix_sock(&fd, &ls->localaddr, hostName, isSockAbstract) != 0) {
                goto error;
            };
            break;
        case AF_INET:
            ls->localaddr.addr.ss_family = AF_INET;
            if(create_tcp_sock(&fd, &ls->localaddr, sockFam, hostName, port) != 0) {
                goto error;
            };
        case AF_INET6:
            /* @todo    will try to support later */ 
            break;
        default:
            goto error;
    }                   

    if(listen(fd, DEFAULT_BACK_LOG) != 0) {
        goto error;
    }                                                           

    ls->fd = fd;
    append(&listeners, &ls->il);

    return TRASH_SUCCESS;

error:
    if(fd != -1) {
        evutil_closesocket(fd);
    }
    return TRASH_ERROR;
}

static int create_tcp_sock(int *fd, struct TrashAddr *taddr, sa_family_t sockFam, const char *hostName, char *port) {
    struct addrinfo hints, *res, *server_info;

    memset(&hints, 0, sizeof(struct addrinfo));                                                                                                                                                                             
    hints.ai_family = sockFam;                                                                                              
    hints.ai_socktype = SOCK_STREAM;                                                                                        
    hints.ai_flags = AI_PASSIVE;

    if ((evutil_getaddrinfo(hostName, port, &hints, &res)) != 0) {    
        return TRASH_ERROR;
    }

    server_info = res;
    while(server_info != NULL) {
        *fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
        evutil_make_socket_nonblocking(*fd);                                 
        if (*fd == -1) {
            server_info = server_info->ai_next;
            continue;   
        }


        if(server_info->ai_family == AF_INET) {
            int enable = 1;
            if(setsockopt(*fd, IPPROTO_TCP, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
                evutil_closesocket(*fd); 
                continue;
            };                                           
        }

        if(bind(*fd, server_info->ai_addr, server_info->ai_addrlen) == 0) {
            break;                 
        } else {
            server_info = server_info->ai_next;
            evutil_closesocket(*fd); 
        }                                                                                        
    }

    evutil_freeaddrinfo(res);

    if(server_info == NULL) {
        return TRASH_ERROR;
    }

    set_ip_trash_addr(taddr, server_info->ai_addr, sockFam);

    return TRASH_SUCCESS;
}

/* @todo    check the lock file for the unix socket before creating a new one */
static int create_unix_sock(int *fd, struct TrashAddr *taddr, const char *sockDir, bool isSockAbstract) {
    struct sockaddr_un addr;
    int addrlen;

    if((*fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return TRASH_ERROR;
    }
    evutil_make_socket_nonblocking(*fd);
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    int sunIndex = 0;
    if(isSockAbstract) {
        addr.sun_path[sunIndex] = '\0';
        sunIndex++;
    }
    
    evutil_snprintf(&addr.sun_path[sunIndex], sizeof(addr.sun_path), SOCKET_NAME, sockDir);

    addrlen = (!isSockAbstract)
        ? sizeof(addr)
        : offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    if (!isSockAbstract) {
		/*
            @todo   check if in use later
        */
		unlink(addr.sun_path);
	}

    if(bind(*fd, (const struct sockaddr *) &addr, addrlen) != 0) {
        return TRASH_ERROR;   
    }

    memcpy(&taddr->addr, &addr, addrlen);
    taddr->addrlen = addrlen;

    if(!isSockAbstract) {
        if(chmod(addr.sun_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1)
            return TRASH_ERROR;
    }

    return TRASH_SUCCESS;
}

static void start_accepting_connections() {
    struct event *ev;
    struct IList *curr;

    for_each(&listeners.head, curr) {
        struct Listener *ls;
        ls = CONTAINER_OF(curr, struct Listener, il);
        
        ev = event_new(serverBase, ls->fd, EV_READ|EV_PERSIST, on_connect, (void *)ls);
        if(event_add(ev, NULL) == 0) {
            ls->ev = ev;
            ls->active = true;
        }
    }
}

static void on_connect(evutil_socket_t sock, short flags, void *arg) {
    TrashSocket *ts;
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    int clientFd;
    struct Listener *ls = (struct Listener *)arg;
    
    while(1) {
        clientFd = accept(sock, &addr, &addrlen);
        if(clientFd < 0) {
            if (errno == EAGAIN || errno == ECONNABORTED) {
                return;
            }else {
                /**
                 * @todo    handle other cases
                */
                return;
            }
        }
        
        bool isUnixSock = is_unix_socket(&(ls->localaddr));
        if(opt_sock(clientFd, isUnixSock) == -1) {
            goto error;
        }

        if(!sock_buff_size(sock, SEND_BUFFY_SIZE, RECV_BUFFY_SIZE)) {
            goto error;
        }

        ts = (TrashSocket *)calloc(1, sizeof(TrashSocket));
        if(ts == NULL) {
            goto error;
        }
        
        memcpy(&ts->localAddr, &ls->localaddr, sizeof(struct TrashAddr));
        if(isUnixSock) {
            char path[ls->localaddr.addrlen];
            get_sock_path(&ls->localaddr, &path, sizeof(path));
            set_unix_trash_addr(&ts->remoteAddr, &path);
        } else{
            set_ip_trash_addr(&ts->remoteAddr, &addr, ls->localaddr.addr.ss_family);
        }
        
        if((construct_client(clientFd, &ts->td, isUnixSock)) == 0) {
            printf("%s connected successfully\n", parse_sock_addr(&addr));
        }
    }

error:
    evutil_closesocket(sock);
    return;
}   
