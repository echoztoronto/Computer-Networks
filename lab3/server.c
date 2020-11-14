#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

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
    
    // buf will hold the first message sent from the client (usually "ftp ...")
    char buf[BUF_SIZE];
    // packet_buf will be used to point to the packets received from the client during the file transfer
    char *packet_buf;
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

    // size_int stores the size of the unsigned int fields of the packet
    // packet_size is set to 1100 to ensure it is long enough to receive even the longest packets sent from the client
    // the char arrays will be used to store the corresponding field of the received packet
    int size_long = 8;
    int size_int = 4;
    int packet_size = 1100;
    char *total_frag;
    char *frag_no;
    char *size;
    char *filename;
    char *filedata;
    // the file pointer to be used when writing to the file being transferred
    FILE *file;

    while(1){

        // allocate a packet_size section of memory to the packet_buf pointer
        packet_buf = malloc(packet_size);

        // receive the first packet from the client into packet_buf
        byte_count = recvfrom(sockfd, packet_buf, packet_size, 0, (struct sockaddr *)&client_addr, &client_size);
        if(byte_count == -1) {
            perror("Error receiving");
            printf("Exiting...\n");
            exit(1);
        }

        // allocate the correct amounts of memory to each packet field char array (filename is set to BUF_SIZE in deliver.c as well)
        total_frag = malloc(sizeof(long));
        frag_no = malloc(sizeof(long));
        size = malloc(sizeof(int));
        filename = malloc(BUF_SIZE);
        filedata = malloc(1000);

        // copy data from packet_buf into the correct char arrays
        // memcpy is used to avoid string functions (strcpy) because data could be in binary format
        // note that the packet_buf is broken down based on the sizes of each field, so as to avoid using strtok() to split by a delimiter
        memcpy(total_frag, &packet_buf[0], sizeof(long));
        memcpy(frag_no, &packet_buf[size_long+1], sizeof(long));
        memcpy(size, &packet_buf[2*size_long+2], sizeof(int));
        memcpy(filename, &packet_buf[2*size_long+size_int+3], BUF_SIZE);
        memcpy(filedata, &packet_buf[2*size_long+size_int+BUF_SIZE+4], 1000);
        
        // if this is the first packet being received, open or create a file called "filename"
        if(atoi(frag_no) == 1){
            file = fopen(filename, "wb");
            printf("Creating file: %s\n", filename);
        }

        // use the size field to determine the amount of data to be written to the file from the filedata field
        // printf("Writing packet fragment %s of %s to file: %s\n", frag_no, total_frag, filename);
        fwrite(filedata, sizeof(char), atoi(size), file);
        
        // send an ACK back to the client to indicate that the packet has been received
        s = sendto(sockfd, "ACK", strlen("ACK")+1, 0, (struct sockaddr *)&client_addr, client_size);
        if(s == -1) {
            perror("Error sending 'ACK'\n");
            printf("Exiting...\n");
            exit(1);
        }
        
        // if the current frag_no is equal to total_frag this is the last packet to be received
        // free all dynamically allocated memory and close the file
        if(strcmp(frag_no, total_frag) == 0) {
            //printf("Closing file: %s\n", filename);
            fclose(file);
            free(total_frag);
            free(frag_no);
            free(size);
            free(filename);
            free(filedata);
            free(packet_buf);
            break;
        }
    }

    printf("File transfer complete!\n");

    // Close the socket
    close(sockfd);
    return 0;
}
