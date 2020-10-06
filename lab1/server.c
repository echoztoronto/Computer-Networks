#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
    if(argc != 2) {
        perror("Usage: server <UDP listen port>\n");
        exit(0);
    }
    
    int sockfd, port, b, byte_count, client_size, s;
    const int BUF_SIZE = 50;
    
    port = atoi(argv[1]);
    
    //creating socket
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        perror("Error: creating socket\n");
        exit(1);
    };
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //binding to socket
    b = bind(sockfd, (struct sockaddr*)&server_addr, sizeof server_addr);
    if(b == -1) {
        perror("Error: binding\n");
        exit(1);
    }
    
    char buf[BUF_SIZE];
    bzero(buf,BUF_SIZE);
    
    struct sockaddr_in client_addr;
    client_size = sizeof client_addr;
    
    //receiving from client 
    byte_count = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_size);
    if(byte_count == -1) {
        perror("Error: receiving\n");
        exit(1);
    }
    
    //checking "ftp" then send "yes" or "no"
    if(strcmp(buf, "ftp") == 0) {
        s = sendto(sockfd, "yes", strlen("yes")+1, 0, (struct sockaddr *)&client_addr, client_size);
        if(s == -1) {
            perror("Error: sending 'yes'\n");
            exit(1);
        }
    } else {
        s = sendto(sockfd, "no", strlen("no")+1, 0, (struct sockaddr *)&client_addr, client_size);
        if(s == -1) {
            perror("Error: sending 'no'\n");
            exit(1);
        }
    }
    
    close(sockfd);
    return 0;
}