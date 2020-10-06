#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
    if(argc != 3) {
        perror("Usage: deliver <server address> <server port number>\n");
        exit(0);
    }
    
    int sockfd, address, port;
    const int BUF_SIZE = 50;
    
    address = atoi(argv[1]);
    port = atoi(argv[2]);
    
    //creating socket
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        perror("Error: creating socket\n");
        exit(1);
    };
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if(inet_aton(argv[1], &server_addr.sin_addr) == 0) {
        perror("Error: converting IP address\n");
        exit(1);
    }
    
    printf("Enter the message: ftp <file name>\n");
    
    char buf[BUF_SIZE], fname[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    bzero(fname, BUF_SIZE);
    
    fgets(buf, BUF_SIZE-1, stdin);
    
    if(buf[0] != 'f' || buf[1] != 't' || buf[2] != 'p' || buf[3] != ' ') {
        perror("Invalid message, usage: ftp <file name>\n");
        exit(1);
    }        
        
    char *token;
    char delim[] = " ";
    token = strtok(buf, delim);
    token = strtok(NULL, delim);
    strncpy(fname, token, sizeof token);
    
    //check file exists
    if(access(fname, F_OK) == -1) {
        perror("File doesn't exist\n");
        exit(1);
    }
    
    //TODO: send message "ftp" to server:  sendto()
    
    
    //TODO: receive message from server:   recvfrom()
    //"yes" -> print "A file transfer can start"
    //"no" -> exit
    
    close(sockfd);
    return 0;
}