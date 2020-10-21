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
    char *packet_buf;//[sizeof(struct Packet)];
    bzero(buf,BUF_SIZE);
    //bzero(packet_buf, sizeof(struct Packet));
    
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

    int size_int = sizeof(unsigned int)/sizeof(char) + 1;
    int data_size;
    char *total_frag;//[size_int];
    char *frag_no;//[size_int];
    char *size;//[size_int];
    char *filename;//[BUF_SIZE];
    char *filedata;//[1000];
    FILE *file;

    // TO-DO: Figure out a way to get this snippet of code to cycle through all the packets sent by the client and write them to the correct file in order, sending
    // an ACK packet after each one. It currently works correctly for the first packet, but no subsequent packets. The do while loop may help.

    while(1){

        packet_buf = malloc(sizeof(struct Packet));

        byte_count = recvfrom(sockfd, packet_buf, sizeof(struct Packet), 0, (struct sockaddr *)&client_addr, &client_size);
        if(byte_count == -1) {
            perror("Error receiving");
            printf("Exiting...\n");
            exit(1);
        }

        /*printf("packet_buf:\n");
        for(int j=0; j<sizeof(struct Packet); j++){
            printf("%c", packet_buf[j]);
        }*/

        total_frag = malloc(size_int);
        frag_no = malloc(size_int);
        size = malloc(size_int);
        filename = malloc(BUF_SIZE);
        filedata = malloc(1000);

        memcpy(total_frag, &packet_buf, size_int);
        memcpy(frag_no, &packet_buf[size_int], size_int);
        memcpy(size, &packet_buf[2*size_int], size_int);
        data_size = atoi(size);
        memcpy(filename, &packet_buf[3*size_int], BUF_SIZE);
        memcpy(filedata, &packet_buf[3*size_int+BUF_SIZE+1], data_size);
        printf("total_frag: %s\n", total_frag);
        printf("frag_no: %s\n", frag_no);
        printf("size: %s\n", size);
        printf("filename: %s\n", filename);
        printf("filedata: %s\n", filedata);

        if(atoi(frag_no) == 1){
            file = fopen(filename, "wb");
            printf("File Opened\n");
        }

        printf("Writing to file: %s\n", filename);
        fprintf(file, "%s", filedata);
        
        s = sendto(sockfd, "ACK", strlen("ACK")+1, 0, (struct sockaddr *)&client_addr, client_size);
        if(s == -1) {
            perror("Error sending 'ACK'\n");
            printf("Exiting...\n");
            exit(1);
        }

        //bzero(packet_buf, sizeof(struct Packet));

        if(strcmp(frag_no, total_frag) == 0) {
            printf("Closing file: %s\n", filename);
            fclose(file);
            free(total_frag);
            free(frag_no);
            free(size);
            free(filename);
            free(filedata);
            free(packet_buf);
            break;
        }
        
        free(total_frag);
        free(frag_no);
        free(size);
        free(filename);
        free(filedata);
        free(packet_buf);

    } 

    
    // Close the socket
    close(sockfd);
    return 0;
}
