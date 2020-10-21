#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#define MIN(a, b) ((a<b) ? a : b)

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
    if(argc != 3) {
        perror("Incorrect Usage");
	    printf("Call should be of form: deliver <server address> <server port number>\n Exiting...\n");
        exit(0);
    }
    
    // Initialize our variables
    int sockfd, address, port, sent_count, received_count, current_frag;
    socklen_t sockaddr_size;
    float file_size;
    const int BUF_SIZE = 50;
    FILE *file;
    struct Packet pkt;
    
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
    
    if( strcmp(buf, "yes") == 0 ){
	    printf("A file transfer can start.\n");
    } else{
        printf("Did not receive 'yes'\n Exiting...\n");
	    exit(1);
    }

    bzero(buf, BUF_SIZE);	

    file = fopen(fname, "rb");
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    // Initialize a packet structure
    int num_frags  = ceil(file_size/1000);
    struct Packet packets[num_frags];
    int bytes_remaining = ((int) file_size);

    for(int i=0; i < num_frags; i++){
	    packets[i].total_frag = num_frags;
	    packets[i].frag_no = i+1;
	    packets[i].size = MIN(bytes_remaining, 1000);
	    packets[i].filename = malloc(strlen(fname) + 1);
	    strcpy(packets[i].filename, fname);
	    fread(packets[i].filedata, packets[i].size, 1, file);
	    bytes_remaining -= packets[i].size;
    }

    char tot_frag[5];
    char frag_num[5];
    char size[5];
    char packet_string[sizeof(struct Packet)];
    int pkt_str_pos;
    
    for(int i=0; i<num_frags; i++){

        pkt_str_pos = 0;

	    bzero(packet_string, sizeof(struct Packet));
	    sprintf(packet_string, "%u", packets[i].total_frag);
	    pkt_str_pos += (sizeof(packets[i].total_frag)/sizeof(char));
	    sprintf(packet_string + pkt_str_pos, ":");
	    pkt_str_pos += 1;
	    sprintf(packet_string + pkt_str_pos, "%u", packets[i].frag_no);
	    pkt_str_pos += (sizeof(packets[i].frag_no)/sizeof(char));
	    sprintf(packet_string + pkt_str_pos, ":");
        pkt_str_pos += 1;
	    sprintf(packet_string + pkt_str_pos, "%u", packets[i].size);
	    pkt_str_pos += (sizeof(packets[i].size)/sizeof(char));
	    sprintf(packet_string + pkt_str_pos, ":");
        pkt_str_pos += 1;
	
	    memcpy(packet_string + pkt_str_pos, packets[i].filename, strlen(packets[i].filename));
	    pkt_str_pos += BUF_SIZE;
	    memcpy(packet_string + pkt_str_pos, ":", 1);
        pkt_str_pos += 1;
	    memcpy(packet_string + pkt_str_pos, packets[i].filedata, packets[i].size);
	
    
	    /*for(int j=0; j<(sizeof(packet_string)/sizeof(char)); j++){
		    printf("packet_string[%d]: %c\n", j, packet_string[j]);
	    }*/
        
        printf("Sending packet fragment %u\n", packets[i].frag_no);
	    sent_count = sendto( sockfd, packet_string, sizeof(struct Packet), 0, (struct sockaddr *)&server_addr, sockaddr_size );
        if(sent_count == 0){
            printf("Successfully sent packet fragment %u\n", packets[i].frag_no);
        } else if(sent_count == -1){
		    perror("Error");
            printf("Error sending packet fragment %u\n", packets[i].frag_no);
            printf("Exiting...\n");
            exit(1);
        }

        received_count = recvfrom( sockfd, buf, BUF_SIZE, 0, (struct sockaddr *)&server_addr, &sockaddr_size );
        if(received_count == -1){
            perror("Error receiving message");
            printf("Exiting...\n");
            exit(1);
        }
    
        if( strcmp(buf, "ACK") == 0 ){
            printf("Server received packet %u.\n", packets[i].frag_no);
        } else{
            printf("Did not receive 'ACK'\n Exiting...\n");
            exit(1);
        }
    }
   
    // Close the socket
    close(sockfd);
    return 0;
}
