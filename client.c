#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

// get sockaddr, IPv4 or IPv6:

struct client {
    int host;
    in_port_t port;
};

int main(int argc, char* argv[]) {
    char* ip_addr = NULL;
    char *s_port = NULL, *c_port = NULL;

    if (argc <= 1)
        ip_addr = "127.0.0.1";
    else
        ip_addr = argv[1];

    if (argc <= 2)
        s_port = "6969";
    else
        s_port = argv[2];

    if (argc <= 3)
        c_port = "12345";
    else
        c_port = argv[3];

    struct addrinfo hints, *servinfo = NULL;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int err;
    if ((err = getaddrinfo(ip_addr, s_port, &hints, &servinfo)) < 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    struct addrinfo *cliinfo = NULL;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((err = getaddrinfo(NULL, c_port, &hints, &cliinfo)) < 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    int sockfd = socket(cliinfo->ai_family, cliinfo->ai_socktype, cliinfo->ai_protocol);
    if (sockfd == -1) {
        perror("sockfd");
        return 1;
    }

    if (bind(sockfd, cliinfo->ai_addr, cliinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("server: bind");
        return 1;
    }

    char *msg = "Hello Server!\n";
    if (sendto(sockfd, msg, strlen(msg), 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("sendto");
        return 1;
    }

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof addr;

    struct client peer;
    int size;
    size = recvfrom(sockfd, &peer, sizeof peer, 0, (struct sockaddr*)&addr, &addr_len);
    if (size != sizeof peer) {
        fprintf(stderr, "failed to receive");
        return 1;
    }

    addr.sin_addr.s_addr = peer.host;
    addr.sin_port = peer.port;

    char* ip = inet_ntoa(addr.sin_addr);
    int port = ntohs(peer.port);

    msg = "Hello peer!";
    char* res = "Received a message!";

    for (int i = 0; i < 10; i++) {
        printf("Sending one packet to %s:%d\n", ip, port);
        if (sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*)&addr, addr_len) == -1) {
            perror("sendto");
            return 1;
        }
        sleep(1);
    }

    char buf[1024];
    while((size = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr*)&addr, &addr_len)) > 0) {
        char* ip = inet_ntoa(addr.sin_addr);
        int port = ntohs(peer.port);

        printf("got packet from %s:%d\n", ip, ntohs(port));
        for (int i = 0; i < size; i++)
            putchar(buf[i]);
        putchar('\n');
    }

    return 0;
}
