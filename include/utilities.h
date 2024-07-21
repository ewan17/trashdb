#ifndef UTILITIES_H
#define UTILITIES_H

#define KEEP_ALIVE  2
#define KEEP_IDLE   2
#define KEEP_INTERVAL   2
#define KEEP_COUNT  10

#define MAX_DIR_SIZE 256

#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>

void *trash_realloc(void *sendBuff, size_t needed);

int validate_filename(const char *filename);

int trash_mkdir(const char *path, size_t len, mode_t mode);
int trash_no_file_dir(const char *filepath);

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
    
#endif //UTILITIES_H