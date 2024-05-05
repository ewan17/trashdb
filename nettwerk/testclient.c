#include "server.h"
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define MSG_LEN_HEADER 4

static void run_client(int sockFam, char *host);
static int unix_client(char *sockDir);
static void construct_message(unsigned char *message_buffer, const char *message, uint32_t message_len);

/**
 * run -- ./clienttest -f 1 -h /home/comet/trashdb/nettwerk
*/
int main(int argc, char *argv[]) {
    sa_family_t sockFam;
    char *host;

    char c;
    while ((c = getopt(argc, argv, "f:h:p:a:")) != -1) {
        switch (c) {
            case 'f':
                if(strcmp(optarg, "1") == 0) {
                    sockFam = AF_UNIX;
                } else if(strcmp(optarg, "2") == 0) {
                    sockFam = AF_INET;
                } else if(strcmp(optarg, "10") == 0) {
                    sockFam = AF_INET6;
                } else {
                    return -1;
                }
                break;
            case 'h':
                host = optarg;
                break;
            case 'p':
                /**
                 * @todo    add later
                */
                break;
            case 'a':
                /**
                 * @todo    add later
                */
                break;
        }
    }

    run_client(sockFam, host);
}

static void run_client(int sockFam, char *host) {
    switch (sockFam) {
    case AF_INET:
        break;
    
    case AF_INET6:
        break;

    case AF_UNIX:
        unix_client(host);
        break;
    }
}

static int unix_client(char *sockDir) {
    unsigned char read_buffer[1024];
    struct sockaddr_un addr;
    int conn;
    int sock = -1;
    int flags;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Failed to create socket\n");
        goto error;
    }

    if ((flags = fcntl(sock, F_GETFL, 0)) == -1) {
        perror("fcntl");
        goto error;
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        goto error;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    snprintf(&addr.sun_path[0], sizeof(addr.sun_path), SOCKET_NAME, sockDir);

    conn = connect(sock, (const struct sockaddr *) &addr, sizeof(addr));
    if(conn == -1) {
        fprintf(stderr, "The server is down.\n");
        goto error;
    }

    const char *message = "This is echo server what the heck is going on nndnfklsldflkandflanflaknelnaldcn asekhldfbgweFBLKqwebfeQKLBFLKAbesfkbasdlkfjbeqkjfbklwqebfiu32i8429385143895y8349y1b9q8y4b98tc3495t348yc5-t91b4385yt3498y5ctr94238y59-y`1239b58ryt498rt9 4t rb c2435yt92  -83y5 92r438";
    uint32_t message_len = MSG_LEN_HEADER + strlen(message);

    unsigned char *buffer = (unsigned char *)malloc(message_len);
    if (!buffer) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    construct_message(buffer, message, message_len);
    printf("message: ");
    for (size_t i = 0; i < message_len; i++)
    {
        printf("%c", buffer[i]);
    }
    printf("\n");

    // strcpy(buffer, );
    if((write(sock, buffer, message_len)) == -1) {
        perror("write");
        goto error;
    }

    memset(read_buffer, 0, 1024);

    while(read(sock, read_buffer, sizeof(read_buffer)) == -1) {
        
    }

    read_buffer[sizeof(read_buffer) - 1] = 0;
    printf("Result = %s\n", read_buffer);
    
    close(sock);

    return 0;

error:
    if(sock != -1) {
        close(sock);
    }
    return -1;
}
 
static void construct_message(unsigned char *message_buffer, const char *message, uint32_t message_len) {
    printf("message_len: %d\n", message_len);
    unsigned char length_bytes[MSG_LEN_HEADER];

    uint32_t message_len_endian = htonl(message_len);

    for (size_t i = 0; i < MSG_LEN_HEADER; i++)
    {
        length_bytes[i] = (message_len_endian >> (i * 8)) & 0xFF;
    }

    memcpy(message_buffer, length_bytes, MSG_LEN_HEADER);
    printf("message_buffer lenget: ");
    for (int i = 3; i >= 0; i--) {
        printf("%02X ", message_buffer[i]);
    }
    printf("\n");

    memcpy(message_buffer + MSG_LEN_HEADER, message, message_len);

}