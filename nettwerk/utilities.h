#ifndef UTILITIES_H
#define UTILITIES_H

#define KEEP_ALIVE  2
#define KEEP_IDLE   2
#define KEEP_INTERVAL   2
#define KEEP_COUNT  10

#define DEFAULT_PORT 50000

void *trash_realloc(void *sendBuff, size_t needed);

bool sock_buff_size(evutil_socket_t sock, int send_buffy_size, int recv_buffy_size);
bool keep_sock_alive(evutil_socket_t sock, int alive, int idle, int interval, int maxpkt);
int opt_sock(evutil_socket_t sock, bool isUnixSock);

struct TrashAddr {
    struct sockaddr_storage addr;
    socklen_t addrlen;
};

bool is_unix_socket(struct TrashAddr *addr);
void get_sock_path(struct TrashAddr *taddr, char *path, size_t pathLen);
void set_unix_trash_addr(struct TrashAddr *taddr, const char *path);
void set_ip_trash_addr(struct TrashAddr *taddr, const struct sockaddr *addr, sa_family_t sockFam);
const char *parse_sock_addr(struct sockaddr *addr);

struct IList {
    struct IList *next;
    struct IList *prev;
};

struct List {
    struct IList head;
    size_t count;
};

void append(struct List *list, struct IList *ilist);
struct IList *pop(struct List *list);

void reset_ilist(struct IList *list);
void ilist_append(struct IList *list, struct IList *item);
struct IList *ilist_remove(struct IList *list);

#define INIT_ILIST(ilist) \
    do { \
        (ilist).next = &(ilist); \
        (ilist).prev = &(ilist); \
    } while (0)

#define RESET_ILIST(ilist) ilist->next = ilist->prev = ilist; 

#define INIT_LIST(list) do {\
    INIT_ILIST((list)->head); \
    (list)->count = 0; \
} while(0)

#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#define for_each(head, curr) \
    for(curr = (head)->next; curr != head; curr = (curr)->next)
    
#endif //UTILITIES_H