#ifndef UTILITIES_H
#define UTILITIES_H

#define KEEP_ALIVE  2
#define KEEP_IDLE   2
#define KEEP_INTERVAL   2
#define KEEP_COUNT  10

bool is_file(const char *path);

void *trash_realloc(void *sendBuff, size_t needed);

bool sock_buff_size(int sock, int send_buffy_size, int recv_buffy_size);
bool keep_sock_alive(int sock, int alive, int idle, int interval, int maxpkt);
int opt_sock(int sock, bool isUnixSock);

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
    ilist = { &(ilist), &(ilist) } \

#define RESET_ILIST(ilist) ilist->next = ilist->prev = ilist; 

#define INIT_LIST(list) \
    list = {{&(list).head, &(list).head} , 0} \

#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#define for_each(head, curr) \
    for(curr = (head)->next; curr != head; curr = (curr)->next)
    
#endif //UTILITIES_H