#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <packet.h>

void login(int sockfd, unsigned char source[], unsigned char data[]);
void logout(unsigned char source[]);
void join(unsigned char source[], unsigned char data[]);
void leave_sess(unsigned char source[]);
void new_sess(unsigned char source[], unsigned char data[]);
void message(unsigned char source[], unsigned char data[]);
void list();

void add_client(struct sockaddr * addr);
struct Node * find_client(int sockfd, char * client_ID, struct Node * head);
int verify_login(char * client_ID, char * password);

typedef struct{
	char * ID;
	char * pwd;
	char * session_ID;
} User;

typedef struct{
	int sockfd;
	int session_id;
	uint32_t IP;
	unsigned short int port;
	User usr;	
} Client_Info;

struct Node{
	Client_Info client;
	struct Node * next;
};

struct Node * head = NULL;

int num_users = 4;

User usr1;
usr1.ID = "Yuwen";
usr1.pwd = "lp473f9"

User usr2;
usr2.ID = "Ryan";
usr2.pwd = "qXp62v3"

User usr3;
usr3.ID = "Jason";
usr3.pwd = "bt4zAp8"

User usr4;
usr4.ID = "Mary";
usr4.pwd = "wR5mx72"

User approved_users[] = {usr1 usr2 usr3 usr4};

int main(int argc, char *argv[])
{
    // Ensure the appropriate number of args were included in the call	
    if(argc != 2) {
	    perror("Incorrect Usage");
        printf("Correct usage: server <UDP listen port>\n Exiting...\n");
        exit(0);
    }
    
    // Initialize our variables
    int listener, max_fd, new_fd, port, b, l, byte_count, s, select_count;
    socklen_t client_size;
    double t1 = 2.5;
    struct timeval timeout;
    const int backlog = 10;
    const int PACKET_SIZE = 100;
    fd_set fd_list, readfds;

    // Convert the port value from string to int
    port = atoi(argv[1]);
    
    // Create the UDP socket
    listener = socket(PF_INET, SOCK_STREAM, 0);
    if(listener == -1) {
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
    b = bind(listener, (struct sockaddr*)&server_addr, sizeof server_addr);
    if(b == -1) {
        perror("Error binding");
	    printf("Exiting...\n");
        exit(1);
    }


    if(listen(listener, backlog) == -1){
    	perror("Error listening");
    	printf("Exiting...\n");
    	exit(1);
    }

    // add the listener socket to the set
    // listener is currently the maximum (and only) file descriptor, so set max_fd accordingly
    FD_ZERO(fd_list);
    FD_ZERO(&readfds);
	FD_SET(listener, &fd_list);
	max_fd = listener;

	timeout.tv_sec = t1 - fmod(t1, 1.0);
	timeout.tv_usec = 1000000*fmod(t1, 1.0);

    // packet_buf will be used to point to the packets received from the client during the file transfer
    char packet_buf[PACKET_SIZE];
    bzero(packet_buf, PACKET_SIZE)
    
    struct sockaddr_in client_addr;
    client_size = sizeof client_addr;


    while(1){
    	readfds = fd_list;
    	if(select(max_fd+1, &readfds, NULL, NULL, timeout) == -1){
    		perror("Error calling select()");
    		printf("Exiting...\n");
    		exit(1);
    	}

    	for(int i=0; i<max_fd; i++){
    		if(FD_ISSET(i, &readfds)){
    			if(i == listener){
    				new_fd = accept(listener, (struct sockaddr *)&client_addr, &client_size);
    				if(new_fd == -1){
    					perror("Error calling accept()");
    					printf("Exiting...\n");
    					exit(1);
    				} else{
    					if(new_fd > max_fd){
    						max_fd = new_fd;
    					}
    					FD_SET(new_fd, &fd_list);
    					// add new client IP and port number to client linked list
    					add_client(new_fd, client_addr, head);
    				}
    			} else{
    				byte_count = recv(i, packet_buf, PACKET_SIZE, 0);
    				if(byte_count == -1){
    					perror("Error calling recv()");
    					printf("Exiting...\n");
    				} else if(byte_count == 0){
    					close(i);
    					FD_CLR(i, &fd_list);
    				} else{ //this means there is data sent from a client which must be processed
    					temp_message = string_to_message(packet_buf);
    					switch(temp_message->type)
    					{
    						case LOGIN:
    							login(i, temp_message->source, temp_message->data);
    							break;
    						case EXIT:
    							logout(temp_message->source);
    							close(i);
    							break;
    						case JOIN:
    							join(temp_message->source, temp_message->data);
    							break;
    						case LEAVE_SESS:
    							leave_sess(temp_message->source);
    							break;
    						case NEW_SESS:
    							new_sess(temp_message->source, temp_message->data);
    							break;
    						case MESSAGE:
    							message(temp_message->source, temp_message->data);
    							break;
    						case QUERY:
    							list();
    							break;
    						default:
    							break;

    					}
    				}
    			}
    		}
    	}
    }
    // Close the socket
    close(listener);
    return 0;
}

void add_client(int new_fd, struct sockaddr_in addr_in, struct Node * head){
	struct Node * temp = head;
	if(temp == NULL){
		temp = (struct Node*)malloc(sizeof(struct Node));
		temp->next = NULL;
		temp->sockfd = new_fd;
		temp->client.IP = addr_in.sin_addr.s_addr;
		temp->client.port = addr_in.sin_port;
		return;
	}
	temp = temp->next;
}

struct Node * find_client(int sockfd, char * client_ID, struct Node * head){
	struct Node * temp = head;
	while(temp != NULL){
		if(client_ID != NULL && temp->client.usr.ID == client_ID){
			return temp;
		} else if(sockfd != NULL && temp->client.sockfd == sockfd){
			return temp;
		} else{
			temp = temp->next;
		}
	}
	return NULL;
}

int remove_client(char * client_ID, struct Node * head){
	struct Node * temp = head;
	while(temp != NULL){
		if(strcmp(temp->client.usr.ID, client_ID) == 0){
			free(temp);
			return 1;
		} else if(temp->next != NULL && strcmp(temp->next->client.usr.ID, client_ID) == 0){
			struct Node * to_delete = temp->next;
			temp->next = temp->next->next;
			free(to_delete);
			return 1;
		}
	}
	return 0;
}

int verify_login(char * client_ID, char * password){
	for(int i=0; i<num_users; i++){
		if(strcmp(client_ID, approved_users[i].ID) == 0 && strcmp(password, approved_users[i].pwd) == 0){
			return 1;
		}
	}
	return 0;
}

void login(int sockfd, unsigned char source[], unsigned char data[]){
	char packet_string[MAX_CHAR];
	const char delim = ",";
	char * client_ID = strtok(data, delim);
	char * password = strtok(NULL, delim);
	struct Node * temp = find_client(sockfd, NULL, head);
	if(temp == NULL){
		printf("Not finding established connection\n");
	}
	if(find_client(NULL, client_ID, head) != NULL){
		struct message * m = create_message(LO_NAK, "", "Specified user is already logged in\n");
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
	} else if(verify_login(client_ID, password)){
		strcpy(temp->client.usr.ID, client_ID);
		strcpy(temp->client.usr.pwd, password);
		struct message * m = create_message(LO_ACK, "", "");
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	} else{
		struct message * m = create_message(LO_NAK, "", "Invalid username or password\n");
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	}
}

void logout(unsigned char source[]){
	if(remove_client(source, head) == 0){
		printf("Error removing client from linked list.");
	}
}

void join(unsigned char source[], unsigned char data[]){
	//check to see if any of the current clients are in the specified session_ID
	//if so, change the source client's session_ID to the one requested and send JN_ACK
	//if not, or if the client has previously been in a session, send a JN_NAK
}

void leave_sess(unsigned char source[]){
	//if current session ID is null, no change
	//if current session ID is not null, set to "-1" to indicate that the client has been in a session and will not be able to join others
}

void new_sess(unsigned char source[], unsigned char data[]){
	//check to make sure session is not already in progress
	//set source client's session ID to the specified value and send a NS_ACK
}

void message(unsigned char source[], unsigned char data[]){
	//check current session ID for source client
	//iterate through online clients and, if their session ID matches, send the data to the corresponding sockfd
}

void list(){
	//send QU_ACK
	//iterate through client linked list and send all user IDs and session IDs, being sure to check for duplicates
}

