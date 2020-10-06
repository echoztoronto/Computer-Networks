#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>




int main(int argc, char *argv[])
{
    if(argc != 2) {
        perror("Usage: server <UDP listen port>\n");
        exit(0);
    }
    
    int sockfd, port;
    struct sockaddr_in server_addr, client_addr;
    
    port = atoi(argv[1]);
    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) {
        perror("Error: creating socket\n");
        exit(1);
    };
    
    return 0;
}