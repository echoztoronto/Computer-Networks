#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
    // Ensure the appropriate number of args were included in the call
    if(argc != 3) {
        perror("Incorrect Usage");
	printf("Call should be of form: deliver <server address> <server port number>\n Exiting...\n");
        exit(0);
    }
    
    // Initialize our variables
    int sockfd, address, port, sent_count, received_count;
    socklen_t sockaddr_size;
    const int BUF_SIZE = 50;
    
    // Convert string arguments to ints
    address = atoi(argv[1]);
    port = atoi(argv[2]);

    // Create the UDP socket
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        perror("Error creating socket");
	printf("Exiting...\n");
        exit(1);
    }
    
    // Gather socket address and port info from args, then store in server_addr
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if(inet_aton(argv[1], &server_addr.sin_addr) == 0) {
        perror("Error converting IP address");
        printf("Exiting...\n");
	exit(1);
    }
    
    sockaddr_size = sizeof(server_addr);

    printf("Enter the message: ftp <file name>\n");
    
    char buf[BUF_SIZE], fname[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    bzero(fname, BUF_SIZE);
    
    // Get input from user
    fgets(buf, BUF_SIZE-1, stdin);
    
    // If user input begins with anything other than 'ftp ', throw and error and exit
    if(buf[0] != 'f' || buf[1] != 't' || buf[2] != 'p' || buf[3] != ' ') {
        perror("Invalid message");
	printf("Input should be of form: ftp <file name>\n Exiting...\n");
        exit(1);
    }        
        
    // Split the user input at the space between 'ftp' and the file name, remove the trailing \n, then copy the file name into fname
    char *token;
    char delim[] = " ";
    token = strtok(buf, delim);
    token = strtok(NULL, delim);
    token = strtok(token, "\n");
    strncpy(fname, token, strlen(token) );
    
    // Check if the file exists
    if(access(fname, F_OK) == -1) {
        perror("Error finding file");
	printf("Exiting...\n");
        exit(1);
    }
    
    // Reset the buffer to empty
    bzero(buf, BUF_SIZE);
	
    //timer
    clock_t start = clock();
    
    // Once the file is confirmed to exist, senf 'ftp' to the server
    sent_count = sendto(sockfd, "ftp", sizeof("ftp"), 0, (struct sockaddr *)&server_addr, sockaddr_size );    
    if(sent_count == -1){
	perror("Error sending 'ftp'");
	printf("Exiting...\n");
	exit(1);
    }
    
    // Check the socket for a reply from the server
    // A 'yes' means a file transfer can start. Anything else results in an exit.
    received_count = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *)&server_addr, &sockaddr_size );
    if(received_count == -1){
	perror("Error receiving message");
        printf("Exiting...\n");
	exit(1);
    }
	
    //timer
    clock_t end = clock();
    double time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Round-trip time: %f seconds\n", time_used);
    
    if( strcmp(buf, "yes") == 0 ){
	printf("A file transfer can start.\n");
    } else{
        printf("Did not receive 'yes'\n Exiting...\n");
	exit(1);
    }	

    // Close the socket
    close(sockfd);
    return 0;
}
