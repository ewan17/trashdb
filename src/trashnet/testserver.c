#include "server.h"
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char *argv[]) {
    sa_family_t sockFam;
    char *host;
    char *port;
    bool isSockAbstract = false;

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
                port = optarg;
                break;
            case 'a':
                if(strcmp(optarg, "1")) {
                    isSockAbstract = true;
                }
                break;
        }
    }

    int rc;
    rc = init_server(sockFam, host, port, isSockAbstract);
    if(rc != 0) {
        printf("Error starting server\n");
        return -1;
    }

    printf("Server listening...\n");
    run_server();
}