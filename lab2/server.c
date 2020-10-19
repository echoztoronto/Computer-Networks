#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

struct Packet{

	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];

};

int main(int argc, char *argv[])
{
    // Ensure the appropriate number of args were included in the call	
    if(argc != 2) {
	perror("Incorrect Usage");
        printf("Correct usage: server <UDP listen port>\n Exiting...\n");
        exit(0);
    }
    
    // Initialize our variables
    int sockfd, port, b, byte_count, s;
    socklen_t client_size;
    const int BUF_SIZE = 50;


    // Convert the port value from string to int
    port = atoi(argv[1]);
    
    // Create the UDP socket
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        perror("Error creating socket");
	printf("Exiting...\n");
        exit(1);
    };
    
    // Store the socket IP address (local) and port in server_addr
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // Bind to the socket in order to listen for connections
    b = bind(sockfd, (struct sockaddr*)&server_addr, sizeof server_addr);
    if(b == -1) {
        perror("Error binding");
	printf("Exiting...\n");
        exit(1);
    }
    
    char buf[BUF_SIZE];
    bzero(buf,BUF_SIZE);
    
    struct sockaddr_in client_addr;
    client_size = sizeof client_addr;
    
    // Receive message from client by listening on correct port
    // Store message received into buf 
    byte_count = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_size);
    if(byte_count == -1) {
        perror("Error receiving");
	printf("Exiting...\n");
        exit(1);
    }

    // If the message received from the client is 'ftp', reply to the client with 'yes'
    // If the message received is anything else, reply to the client with 'no'
    if(strcmp(buf, "ftp") == 0) {
        s = sendto(sockfd, "yes", strlen("yes")+1, 0, (struct sockaddr *)&client_addr, client_size);
        if(s == -1) {
            perror("Error sending 'yes'");
	    printf("Exiting...\n");
            exit(1);
        }
    } else {
        s = sendto(sockfd, "no", strlen("no")+1, 0, (struct sockaddr *)&client_addr, client_size);
        if(s == -1) {
            perror("Error sending 'no'\n");
            printf("Exiting...\n");
	    exit(1);
        }
    }
    
    // Close the socket
    close(sockfd);
    return 0;
}
