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

    file = fopen(fname, "rb");
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    // Initialize a packet structure
    int num_frags  = ceil(file_size/1000);
//    printf("filename = %s\n", fname);
//    printf("num_frags = %d\n", num_frags);
    struct Packet packets[num_frags];
    int bytes_remaining = ((int) file_size);

    for(int i=0; i < num_frags; i++){
//	printf("bytes_remaining: %d\n", bytes_remaining);
	packets[i].total_frag = num_frags;
	packets[i].frag_no = i+1;
	packets[i].size = MIN(bytes_remaining, 1000);
	packets[i].filename = fname;
	fread(packets[i].filedata, packets[i].size, 1, file);
//	printf("Fragment %d: %s\n", i, packets[i].filedata);
	bytes_remaining -= packets[i].size;
    }

    char tot_frag[100];
    char frag_num[100];
    char size[100];
    char *packet_string;
    int pkt_str_pos = 0;
    for(int i=0; i< num_frags; i++){

        printf("1\n");	
	snprintf(tot_frag, 99, "%d", packets[i].total_frag);
	snprintf(frag_num, 99, "%d", packets[i].frag_no);
	snprintf(size, 99, "%d", packets[i].size);
	printf("tot_frag: %s, size of tot_frag: %lu", tot_frag, sizeof(tot_frag));

	memcpy(packet_string, tot_frag, sizeof(*tot_frag));
	pkt_str_pos += sizeof(*tot_frag);
	printf("2\n");
	memcpy(packet_string + pkt_str_pos, ":", 1);
        pkt_str_pos += 1;
	printf("3\n");
	memcpy(packet_string + pkt_str_pos, frag_num, sizeof(*frag_num));
	pkt_str_pos += sizeof(*frag_num);
	memcpy(packet_string + pkt_str_pos, ":", 1);
        pkt_str_pos += 1;
	memcpy(packet_string + pkt_str_pos, size, sizeof(*size));
	pkt_str_pos += sizeof(*size);
	memcpy(packet_string + pkt_str_pos, ":", 1);    
        pkt_str_pos += 1;
	memcpy(packet_string + pkt_str_pos, packets[i].filename, sizeof(*packets[i].filename));
        pkt_str_pos += sizeof(*packets[i].filename);
	memcpy(packet_string + pkt_str_pos, ":", 1);
        pkt_str_pos += 1;
	memcpy(packet_string + pkt_str_pos, packets[i].filedata, sizeof(packets[i].filedata));

	printf("packet string: %s\n", packet_string);

	sent_count = sendto(sockfd, packet_string, sizeof(packet_string), 0, (struct sockaddr *)&server_addr, sockaddr_size );
        if(sent_count == -1){
                printf("Error sending packet fragment %d\n", pkt.frag_no);
                printf("Exiting...\n");
                exit(1);
        }
    }
   
    // Close the socket
    close(sockfd);
    return 0;
}
