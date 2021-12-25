#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define PEER_NUM 2

// get sockaddr, IPv4 or IPv6:

struct client {
    int host;
    in_port_t port;
};

int main(int argc, char* argv[]) {
    char *s_port = NULL;

    if (argc <= 1)
        s_port = "6969";
    else
        s_port = argv[1];

    struct addrinfo hints, *servinfo = NULL;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int err;
    if ((err = getaddrinfo(NULL, s_port, &hints, &servinfo)) < 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("sockfd");
        return 1;
    }

    int ok;
    // if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) == -1) {
    //     close(sockfd);
    //     perror("setsockopt");
    //     return 1;
    // }

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("server: bind");
        return 1;
    }

    freeaddrinfo(servinfo);

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof addr;

    struct client peer[PEER_NUM];
    memset(peer, 0, sizeof peer);


    for (int i = 0; i < PEER_NUM; i++) {
        printf("Waiting peer %d...\n", i+1);

        char buf[1024];
        int size;
        size = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr*)&addr, &addr_len);

        if (size <= 0) {
            perror("recvfrom");
            return 1;
        }
        char* ip = inet_ntoa(addr.sin_addr);
        int port = addr.sin_port;

        printf("got packet from %s:%d\n", ip, ntohs(port));
        for (int i = 0; i < size; i++)
            putchar(buf[i]);

        peer[i].host = addr.sin_addr.s_addr;
        peer[i].port = addr.sin_port;
    }

    printf("Estabilishing hole-punch...\n");

    for (int i = 0; i < PEER_NUM; i++) {
        addr.sin_addr.s_addr = peer[i].host;
        addr.sin_port = peer[i].port;

        for (int j = 0; j < PEER_NUM; j++) {
            if (i == j) continue;

            if (sendto(sockfd, &peer[j], sizeof peer[j], 0, (struct sockaddr*)&addr, addr_len) == -1) {
                perror("sendto");
                return 1;
            }
        }
    }

    return 0;
}
