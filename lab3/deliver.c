#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#define MIN(a, b) ((a<b) ? a : b)
#define MAX(a, b) ((a>b) ? a : b)

// define the packet structure as specified in lab handout
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
    int sockfd, address, port, sent_count, received_count, select_count, current_frag;
    socklen_t sockaddr_size, send_attempts;
    fd_set readfds;
    // This is the timeout value for resending packets in seconds
    double t1 = 1.5;
    struct timeval timeout;
    float file_size;
    bool ACK_RECEIVED;
    const int BUF_SIZE = 50;
    FILE *file;
    
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

    clock_t start_t, end_t;
    double time_elapsed, response_time;
    // start the clock for RTT calculation
    start_t = clock();

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

    // end the clock for RTT calculation
    end_t = clock();
    time_elapsed = ((double) end_t - start_t) / CLOCKS_PER_SEC;
    printf("Elapsed time between sending message to server and receiving response from server: %f seconds\n", time_elapsed);
    
    if( strcmp(buf, "yes") == 0 ){
	    printf("A file transfer can start.\n");
    } else{
        printf("Did not receive 'yes'\n Exiting...\n");
	    exit(1);
    }

    bzero(buf, BUF_SIZE);	

    // open the file specified by the user
    file = fopen(fname, "rb");
    // use fseek() to scan to the end and determine the size of the file, then seek back to the beginning
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    // using the file size, determine the number of 1000 length fragments necessary and create an array of packets of appropriate length
    int num_frags  = ceil(file_size/1000);
    struct Packet* packets = malloc(num_frags * sizeof(struct Packet));
    int data_remaining = ((int) file_size);

    // for each packet, assign the fields their correct values
    // for the size field, the last packet will likely be smaller than 1000, so use the MIN function
    // dynamically allocate the filename field
    for(int i=0; i < num_frags; i++){
	    packets[i].total_frag = num_frags;
	    packets[i].frag_no = i+1;
	    packets[i].size = MIN(data_remaining, 1000);
	    packets[i].filename = malloc(strlen(fname) + 1);
	    strcpy(packets[i].filename, fname);
        // read "size" amount of the file contents into the filedata field for this packet
	    fread(packets[i].filedata, packets[i].size, 1, file);
	    data_remaining -= packets[i].size;
    }

    // this packet_string will store the entire concatenated packet to be sent to the server
    // its size is set to 1100 to ensure it will be long enough for even the longest packets
    int packet_size = 1100;
    char packet_string[packet_size];
    int pkt_str_pos;

    // clear all entries from the set of sockets to be used in the call to select(). then
    // add sockfd to this set of sockets
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    // setting the proper timeout value for the call to select using t1
    timeout.tv_sec = t1 - fmod(t1, 1.0);
    timeout.tv_usec = 1000000*fmod(t1, 1.0);
   
    for(int i=0; i<num_frags; i++){

        ACK_RECEIVED = false;
        send_attempts = 0;

        pkt_str_pos = 0;

        // reset the packet string to be reused each time
	    bzero(packet_string, packet_size);

        // we can use sprintf to convert the unsigned int fields to chars in the packet_string
        // use the pkt_str_pos variable to keep track of the index in packet_string
        // add colons between each field
	    sprintf(packet_string, "%u", packets[i].total_frag);
	    pkt_str_pos += sizeof(long);
	    sprintf(packet_string + pkt_str_pos, ":");
	    pkt_str_pos += 1;
	    sprintf(packet_string + pkt_str_pos, "%u", packets[i].frag_no);
	    pkt_str_pos += sizeof(long);
	    sprintf(packet_string + pkt_str_pos, ":");
        pkt_str_pos += 1;
	    sprintf(packet_string + pkt_str_pos, "%u", packets[i].size);
	    pkt_str_pos += sizeof(int);
	    sprintf(packet_string + pkt_str_pos, ":");
        pkt_str_pos += 1;

	    // use memcpy to copy the filename and filedata fields into packet_string in case they contain binary data
	    memcpy(packet_string + pkt_str_pos, packets[i].filename, BUF_SIZE);
	    pkt_str_pos += BUF_SIZE;
	    memcpy(packet_string + pkt_str_pos, ":", 1);
        pkt_str_pos += 1;
	    memcpy(packet_string + pkt_str_pos, packets[i].filedata, 1000);

        // loop while waiting to receive an ACK for the current packet
        while(!ACK_RECEIVED){
            // send the current packet to the server
	        sent_count = sendto( sockfd, packet_string, packet_size, 0, (struct sockaddr *)&server_addr, sockaddr_size );
            if(sent_count == -1){
		        perror("Error");
                printf("Error sending packet fragment %u\n", packets[i].frag_no);
                printf("Exiting...\n");
                exit(1);
            }
            send_attempts++;

            // use select() to check if there is data waiting to be received on sockfd over the next "timeout" seconds
            select_count = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
            if(select_count == -1){
                perror("Error");
                printf("Error calling select()");
                printf("Exiting...\n");
                exit(1);
            } else if(select_count == 0){
                // select() returns 0 when the call times out
                if(send_attempts < 10){
                    // the maximum number of packet send attempts is arbitrarily set to 10
                    printf("Did not receive 'ACK' within %f seconds\n Attempting to resend file fragment %u of %u\n", t1, packets[i].frag_no, packets[i].total_frag);
                } else{
                    printf("Reached maximum number of send attempts for fragment %u\n Exiting...\n", packets[i].frag_no);
                    exit(1);
                }
            } else{
                //receive the expected reply message from the server
                received_count = recvfrom( sockfd, buf, BUF_SIZE, 0, (struct sockaddr *)&server_addr, &sockaddr_size );
                if(received_count == -1){
                    perror("Error receiving message");
                    printf("Exiting...\n");
                    exit(1);
                }
                // printf("response_time: %f\n", response_time);
                // if the message received from the server is ACK, send the next packet
                // any other message is cause to exit the program and stop the file transfer
                if( strcmp(buf, "ACK") == 0 ){
                    ACK_RECEIVED = true;
                } else{
                    printf("Received message from server that was not an ACK.\n Exiting...\n");
                }
            }
        }
        // free the dynamically allocated memory
        free(packets[i].filename);
    }

    printf("File transfer complete!\n");

    free(packets);
   
    // Close the file and the socket
    fclose(file);
    close(sockfd);
    return 0;
}
