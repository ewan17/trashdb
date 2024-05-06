#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

#include "utilities.h"

void *trash_realloc(void *ptr, size_t needed) {
    void *newPtr;
    newPtr = realloc(ptr, needed);
    if(newPtr == NULL) {
        free(ptr);
    }
    return newPtr;
}

bool sock_buff_size(int sock, int send_buffy_size, int recv_buffy_size) {
    if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buffy_size, sizeof(send_buffy_size)) != 0) {
        return false;
    }

    if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recv_buffy_size, sizeof(recv_buffy_size)) != 0) {
        return false;
    }
    return true;
}

bool keep_sock_alive(int sock, int alive, int idle, int interval, int maxpkt) {
#ifdef SO_KEEPALIVE    
    if(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof(alive)) != 0) {
        return false;
    }
#endif
#ifdef TCP_KEEPIDLE    
    if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) != 0) {
        return false;
    }
#endif
#ifdef TCP_KEEPINTVL    
    if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) != 0) {
        return false;
    }
#endif
#ifdef TCP_KEEPCNT    
    if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(maxpkt)) != 0) {
        return false;
    }
#endif
    return true;
}

int opt_sock(int sock, bool isUnixSock) {
    if(evutil_make_socket_nonblocking(sock) != 0) {
        return 1;
    }   

    if(!isUnixSock) {
        if(!keep_sock_alive(sock, KEEP_ALIVE, KEEP_IDLE, KEEP_INTERVAL, KEEP_COUNT)) {
            return 1;
        }
    }
    return 0;
}

bool is_unix_socket(struct TrashAddr *addr) {
    return (addr->addr.ss_family == AF_UNIX);
}

/**
 * @todo    fix this. what if the socket path is absolute? 
*/
void get_sock_path(struct TrashAddr *taddr, char *path, size_t pathLen) {
    if(taddr == NULL || taddr->addr.ss_family != AF_UNIX) {
        return;
    }

    struct sockaddr_un *addr;
    addr = (struct sockaddr_un *) &taddr->addr;
    if(pathLen >= sizeof(addr->sun_path)) {
        memcpy(path, addr->sun_path, sizeof(addr->sun_path));
    }
}

void set_unix_trash_addr(struct TrashAddr *taddr, const char *path) {
    if(path != NULL && taddr != NULL) {
        memset(taddr, 0, sizeof(struct TrashAddr));
        taddr->addr.ss_family = AF_UNIX;

        struct sockaddr_un *addr = (struct sockaddr_un *) &taddr->addr;
        addr->sun_family = AF_UNIX;
        
        uint16_t index = 0;
        if(path[0] == '@') {
            addr->sun_path[0] = '\0';
            index = 1;
        }

        memcpy(&addr->sun_path[index], path + index, sizeof(addr->sun_path));
    }
}

void set_ip_trash_addr(struct TrashAddr *taddr, const struct sockaddr *addr, sa_family_t sockFam) {
    taddr->addr.ss_family = sockFam;
    switch (sockFam) {
        case AF_INET:
            taddr->addrlen = sizeof(struct sockaddr_in);
            memcpy(&taddr->addr, addr, taddr->addrlen);
            break;
        
        case AF_INET6:
            taddr->addrlen = sizeof(struct sockaddr_in6);
            memcpy(&taddr->addr, addr, taddr->addrlen);
            break;

        default:
            return;
    }
}

const char *parse_sock_addr(struct sockaddr *addr) {
    const char *pAddr;

    switch (addr->sa_family) {
        case AF_UNIX:
            pAddr = "unix socket";
            break;

        case AF_INET:
            char pAddrBuff[INET_ADDRSTRLEN];
            memset(pAddrBuff, 0, sizeof(pAddrBuff));

            pAddr = inet_ntop(AF_INET, addr->sa_data, pAddrBuff, INET_ADDRSTRLEN);
            break;
        
        default:
            pAddr = "rip";
            break;
    }

    return pAddr;
}

struct IList *pop(struct List *list) {
    if(list != NULL) {
        struct IList *rc;
        if((rc = ilist_remove(&list->head)) == NULL) {
            list->count--;
        };
        return rc;
    } else {
        return NULL;
    }
}

void append(struct List *list, struct IList *ilist) {
    ilist_append(&list->head, ilist);
    list->count++;
}

void reset_ilist(struct IList *ilist) {
    ilist->next = ilist->prev = ilist;
}

void ilist_append(struct IList *list, struct IList *item) {
    list->prev->next = item;
    item->prev = list->prev;
    list->prev = item;
    item->next = list;
}

struct IList *ilist_remove(struct IList *list) {
    if(list == NULL) {
        return NULL;
    }

    if(list->next == NULL || list->prev == NULL)
        goto reset;

    list->prev->next = list->next;
    list->next->prev = list->prev;

reset:
    reset_ilist(list);

    return list;
}

