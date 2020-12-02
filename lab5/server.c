#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include "packet.h"

void login(int sockfd, unsigned char source[], unsigned char data[]);
void logout(unsigned char source[]);
void join(int sockfd, unsigned char source[], unsigned char data[]);
void leave_sess(unsigned char source[], unsigned char data[]);
void new_sess(int sockfd, unsigned char source[], unsigned char data[]);
void message(unsigned char source[], unsigned char data[]);
void list(int sockfd);
void invite(int sockfd, unsigned char source[], unsigned char data[]);
void inv_accept(int sockfd, unsigned char data[]);
void inv_reject(int sockfd, unsigned char data[]);

void add_client(int new_fd, struct sockaddr_in addr_in);
struct Node * find_client(int sockfd, char * client_ID);
int verify_login(char * client_ID, char * password);
int verify_session(char * session_ID);


typedef struct User{
	char ID[MAX_NAME];
	char pwd[MAX_CHAR];
	char session_ID[MAX_CHAR];
} User;

typedef struct Client_Info{
	int sockfd;
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

User approved_users[] = { {"Yuwen", "lp473f9", NULL}, {"Ryan", "qXp62v3", NULL}, {"Jason", "bt4zAp8", NULL}, {"Mary", "wR5mx72", NULL} };

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
    
    // Create the socket
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
    FD_ZERO(&fd_list);
    FD_ZERO(&readfds);
	FD_SET(listener, &fd_list);
	max_fd = listener;

	timeout.tv_sec = t1 - fmod(t1, 1.0);
	timeout.tv_usec = 1000000*fmod(t1, 1.0);

    // packet_buf will be used to point to the packets received from the client during the file transfer
    char packet_buf[PACKET_SIZE];
    bzero(packet_buf, PACKET_SIZE);
    
    struct sockaddr_in client_addr;
    client_size = sizeof client_addr;


    while(1){
    	readfds = fd_list;

    	printf("Calling select()");
    	if(select(max_fd+1, &readfds, NULL, NULL, NULL) == -1){
    		perror("Error calling select()");
    		printf("Exiting...\n");
    		exit(1);
    	}

    	printf("Returned from select()\n");
    	printf("listener: %d\n", listener);
    	printf("max_fd: %d\n", max_fd);

    	for(int i=0; i<=max_fd; i++){
    		if(FD_ISSET(i, &readfds)){
    			printf("socket in readfds: %d\n", i);
    		}
    	}

    	for(int i=0; i<=max_fd; i++){
    		if(FD_ISSET(i, &readfds)){
    			if(i == listener){
    				printf("New message on listener port\n");
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
    					add_client(new_fd, client_addr);
    					printf("added client with new_fd: %d\n", new_fd);
    				}
    			} else{
    				printf("New message from client\n");
    				byte_count = recv(i, packet_buf, PACKET_SIZE, 0);
    				if(byte_count == -1){
    					perror("Error calling recv()");
    					printf("Exiting...\n");
    				} else if(byte_count == 0){
    					close(i);
    					FD_CLR(i, &fd_list);
    				} else{ //this means there is data sent from a client which must be processed
    					struct message * temp_message = string_to_message(packet_buf);
    					switch(temp_message->type)
    					{
    						case LOGIN:
    							login(i, temp_message->source, temp_message->data);
    							break;
    						case EXIT:
    							logout(temp_message->source);
    							close(i);
    							FD_CLR(i, &fd_list);
    							if(i == max_fd){
    								max_fd = recalculate_max_fd();
    							}
    							break;
    						case JOIN:
    							join(i, temp_message->source, temp_message->data);
    							break;
    						case LEAVE_SESS:
    							leave_sess(temp_message->source, temp_message->data);
    							break;
    						case NEW_SESS:
    							new_sess(i, temp_message->source, temp_message->data);
    							break;
    						case MESSAGE:
    							message(temp_message->source, temp_message->data);
    							break;
    						case QUERY:
    							list(i);
    							break;
    						case INVITE:
    							invite(i, temp_message->source, temp_message->data);
    							break;
    						case INV_ACCEPT:
    							inv_accept(i, temp_message->data);
    							break;
    						case INV_REJECT:
    							inv_reject(i, temp_message->data);
    							break;
    						default:
    							printf("In default case.\n");
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

void add_client(int new_fd, struct sockaddr_in addr_in){
	struct Node * new = (struct Node*)malloc(sizeof(struct Node));
	new->next = NULL;
	new->client.sockfd = new_fd;
	new->client.IP = addr_in.sin_addr.s_addr;
	new->client.port = addr_in.sin_port;
	if(head == NULL){
		head = new;
		return;
	}
	struct Node * temp = head;
	while(temp->next != NULL){
		temp = temp->next;
	}
	temp->next = new;
	printf("In add_client\n new->client.sockfd: %d\n", new->client.sockfd);
	return;
}

struct Node * find_client(int sockfd, char * client_ID){
	printf("In find_client\n sockfd: %d, client_ID: %s\n", sockfd, client_ID);
	struct Node * temp = head;
	if(head == NULL){
		printf("head is NULL\n");
	}
	while(temp != NULL){
		if(client_ID != NULL && strcmp(temp->client.usr.ID, client_ID) == 0){
			printf("temp client_ID: %s\n", temp->client.usr.ID);
			return temp;
		} else if(sockfd != 0 && temp->client.sockfd == sockfd){
			printf("temp sockfd: %d\n", temp->client.sockfd);
			return temp;
		} else{
			temp = temp->next;
		}
	}
	return NULL;
}

int remove_client(char * client_ID){
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

int recalculate_max_fd(){
	printf("Recalculating max_fd\n");
	struct Node * temp = head;
	int max = 0;
	while(temp != NULL){
		if(temp->client.sockfd > max){
			max = temp->client.sockfd;
		}
		temp = temp->next;
	}
	return max;
}

int verify_login(char * client_ID, char * password){
	printf("In verify_login. client_ID: %s, password: %s\n", client_ID, password);
	for(int i=0; i<num_users; i++){
		if(strcmp(client_ID, approved_users[i].ID) == 0 && strcmp(password, approved_users[i].pwd) == 0){
			return 1;
		}
	}
	return 0;
}

int verify_session(char * session_ID){
	printf("In verify_session!!\n");
	struct Node * temp = head;
	char delim = ',';
	char * session_name;
	session_name = strtok(temp->client.usr.session_ID, ",");
	while(temp != NULL){
		while(session_name != NULL){
			printf("session_name: %s", session_name);
			if(strcmp(session_name, session_ID) == 0){
				printf("Done verify_session\n");
				return 1;
			}
			session_name = strtok(NULL, &delim);
		}
		temp = temp->next;
	}

	//the session named "session_ID" does not exist yet or has been terminated
	printf("Done verify_session\n");
	return 0;
}

void login(int sockfd, unsigned char source[], unsigned char data[]){
	char packet_string[MAX_CHAR];
	char delim = ',';
	char * client_ID = strtok(data, &delim);
	char * password = strtok(NULL, &delim);
	printf("In login. sockfd: %d\n", sockfd);
	struct Node * temp = find_client(sockfd, NULL);
	if(temp == NULL){
		printf("Not finding established connection\n");
	}
	if(find_client(0, client_ID) != NULL){
		struct message * m = create_message(LO_NAK, "", "Specified user is already logged in\n");
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	} else if(verify_login(client_ID, password) == 1){
		printf("Send successful login message\n");
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
		printf("In else clause\n");
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
	if(remove_client(source) == 0){
		printf("Error removing client from linked list.");
	}
}

void join(int sockfd, unsigned char source[], unsigned char data[]){
	//check to see if any of the current clients are in the specified session_ID
	//if so, change the source client's session_ID to the one requested and send JN_ACK
	//if not, or if the client has previously been in a session, send a JN_NAK
	printf("In join\n");
	struct Node * client_node = find_client(0, source);
	if(client_node == NULL){
		printf("user not found\n");
	}
	struct message * m;
	char reply_data[MAX_CHAR];
	char packet_string[MAX_CHAR];

	if(strstr(client_node->client.usr.session_ID, data) != NULL){
		strcpy(reply_data, "You are already in the specified session.");
		m = create_message(JN_ACK, "", reply_data);
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	} else if(verify_session(data)){
		//the session exists, so the current user will now be made part of the session
		strcat(client_node->client.usr.session_ID, data);
		strcat(client_node->client.usr.session_ID, ",");
		strcpy(reply_data, data);
		m = create_message(JN_ACK, "", reply_data);
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	} else{
		//the session does not exist
		strcpy(reply_data, data);
		strcat(reply_data, ",");
		strcat(reply_data, "The requested session_ID does not exist.");
		m = create_message(JN_NAK, "", reply_data);
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	}

	printf("Done join\n");

}

void leave_sess(unsigned char source[], unsigned char data[]){
	//if current session ID is null, no change
	//if current session ID is not null, set to "attended" to indicate that the client has been in a session and will not be able to join others
	printf("in leave_sess\n");
	struct Node * client_node = find_client(0, source);
	if(client_node == NULL){
		printf("user not found\n");
	}
	char new_sess_list[MAX_CHAR];
	char * current_sess_list = client_node->client.usr.session_ID;
	char * sess;
	char delim = ',';

	//go through the source client's list of active sessions
	//move all session_IDs that the client does not wish to leave into a new char array
	//replace the source client's session list with the new list
	if(sess = strtok(current_sess_list, &delim) != NULL){
		while(sess != NULL){
			if(strcmp(sess, data) != 0){
				strcat(new_sess_list, sess);
				strcat(new_sess_list, ",");
			}
		}
	}
	strcpy(client_node->client.usr.session_ID, new_sess_list);
	printf("done leave_sess\n");

}

void new_sess(int sockfd, unsigned char source[], unsigned char data[]){
	//check to make sure session is not already in progress
	//set source client's session ID to the specified value and send a NS_ACK
	printf("in new_sess\n");
	struct Node * client_node = find_client(0, source);
	if(client_node == NULL){
		printf("user not found\n");
	}
	struct message * m;
	char reply_data[MAX_CHAR];
	char packet_string[MAX_CHAR];
	strcpy(reply_data, data);
	if(verify_session(data)){
		//session already in progress
		strcat(reply_data, ",");
		strcat(reply_data, "The specified session already exists");
		m = create_message(NS_NAK, "", reply_data);
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	} else{
		strcat(client_node->client.usr.session_ID, data);
		strcat(client_node->client.usr.session_ID, ",");
		printf("Session IDs: %s\n", client_node->client.usr.session_ID);
		m = create_message(NS_ACK, "", reply_data);
		strcpy(packet_string, message_to_string(m));
		if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
			perror("Error calling send()");
			printf("Exiting...\n");
			exit(1);
		}
	}
	printf("done new_sess\n");
}

void message(unsigned char source[], unsigned char data[]){
	//check current session ID for source client
	//iterate through online clients and, if their session ID matches, send the data to the corresponding sockfd
	printf("in message\n");
	struct Node * source_client = find_client(0, source);
	if(source_client == NULL){
		printf("user not found\n");
	}
	struct message * m;
	char packet_string[MAX_CHAR];
	char * session;
	char * source_session;
	char delim = ',';

	//iterate through each session attended by the source client
	source_session = strtok(source_client->client.usr.session_ID, &delim);
	while(source_session != NULL){
		//send messages to all clients also currently attending the same session(s)
		struct Node * dest_client = head;
		while(dest_client != NULL){
			//iterate through each 
			session = strtok(dest_client->client.usr.session_ID, &delim);
			while(session != NULL){
				//if the dest_client is not the source client and a common session has been found
				if(strcmp(dest_client->client.usr.ID, source) != 0 && strcmp(session, source_session) == 0){
					//the current dest_client is in the same session and is not the sender, so they should receive the message
					m = create_message(MESSAGE, source, data);
					strcpy(packet_string, message_to_string(m));
					if(send(dest_client->client.sockfd, packet_string, sizeof(packet_string), 0) == -1){
						perror("Error calling send()");
						printf("Exiting...\n");
						exit(1);
					}
				}
				session = strtok(NULL, &delim);
			}
			dest_client = dest_client->next;
		}
		source_session = strtok(NULL, &delim);
	}
	printf("Done message\n");
}


void list(int sockfd){
	//send QU_ACK
	//iterate through client linked list and send all user IDs and session IDs, being sure to check for duplicates
	printf("In list\n");
	struct message * m;
	char packet_string[10*MAX_CHAR];

	char delim = ',';
	char online_users[5*MAX_NAME];
	char active_sessions[5*MAX_CHAR];
	char reply_data[10*MAX_CHAR];
	char * session_ID;
	strcpy(online_users, "Users currently online: ");
	strcpy(active_sessions, "\nSessions currently active: ");
	struct Node * client = head;

	while(client != NULL){
		strcat(online_users, client->client.usr.ID);
		strcat(online_users, " ");
		if(client->client.usr.session_ID != NULL){
			session_ID = strtok(client->client.usr.session_ID, &delim);
			while(session_ID != NULL){
				if(strstr(active_sessions, session_ID) == NULL){
					//the user is currently in a session that has not been added to the list yet
					strcat(active_sessions, client->client.usr.session_ID);
					strcat(active_sessions, " ");
				}
				session_ID = strtok(NULL, &delim);
			}
		}
		client = client->next;
	}
	strcat(active_sessions, "\n");

	strcpy(reply_data, online_users);
	strcat(reply_data, active_sessions);
	m = create_message(QU_ACK, "", reply_data);
	strcpy(packet_string, message_to_string(m));
	if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
		perror("Error calling send()");
		printf("Exiting...\n");
		exit(1);
	}
	printf("done list\n");
}

void invite(int sockfd, unsigned char source[], unsigned char data[]){
	printf("in invite\n");
	struct Node * source_client = find_client(0, source);
	if(source_client == NULL){
		printf("user not found\n");
	}

	int NACK = 0;
	struct message * m;
	char reply_data[MAX_CHAR];
	char packet_string[MAX_CHAR];

	char delim = ',';
	char * dest_client_ID = strtok(data, &delim);
	char * session_ID = strtok(NULL, &delim);
	struct Node * dest_client = find_client(0, dest_client_ID);
	if(dest_client == NULL){
		strcpy(reply_data, "There is no user <");
		strcat(reply_data, dest_client_ID);
		strcat(reply_data, "> or they are not currently signed in.");
		NACK = 1;
	} else if(!verify_session(session_ID)){
		strcpy(reply_data, "There is no active session called <");
		strcat(reply_data, session_ID);
		strcat(reply_data, ">.");
		NACK = 1;
	} else if(strstr(source_client->client.usr.session_ID, session_ID) == NULL){
		strcpy(reply_data, "You are not in session <");
		strcat(reply_data, session_ID);
		strcat(reply_data, ">.");
		NACK = 1;
	} else if(strstr(dest_client->client.usr.session_ID, session_ID) != NULL){
		strcpy(reply_data, "User <");
		strcat(reply_data, dest_client_ID);
		strcat(reply_data, "> is already in session <");
		strcat(reply_data, session_ID);
		strcat(reply_data, ">.");
		NACK = 1;
	}

	if(NACK){
		m = create_message(INVITE_NAK, "", reply_data);
	} else{
		m = create_message(INVITE_ACK, "", reply_data);
	}

	//send invitation ack or nack back to sender
	strcpy(packet_string, message_to_string(m));
	if(send(sockfd, packet_string, sizeof(packet_string), 0) == -1){
		perror("Error calling send()");
		printf("Exiting...\n");
		exit(1);
	}

	//send invitation to intended recipient
	m = NULL;
	m = create_message(INVITATION, source, data);
	strcpy(packet_string, message_to_string(m));
	if(send(dest_client->client.sockfd, packet_string, sizeof(packet_string), 0) == -1){
		perror("Error calling send()");
		printf("Exiting...\n");
		exit(1);
	}

	printf("done invite\n");

}


void inv_accept(int sockfd, unsigned char data[]){
	printf("in inv_accept\n");
	struct Node * source_client = find_client(sockfd, NULL);
	if(source_client == NULL){
		printf("user not found\n");
	}

	char delim = ',';
	char * recipient = strtok(data, &delim);
	char * sessionID = strtok(NULL, &delim);

	strcat(source_client->client.usr.session_ID, sessionID);
	strcat(source_client->client.usr.session_ID, ",");

	struct Node * dest_client = find_client(0, recipient);
	if(dest_client == NULL){
		printf("user not found\n");
	}

	char packet_string[MAX_CHAR];
	struct message * m = create_message(INV_RESPONSE, "", "accepted");
	strcpy(packet_string, message_to_string(m));
	if(send(dest_client->client.sockfd, packet_string, sizeof(packet_string), 0) == -1){
		perror("Error calling send()");
		printf("Exiting...\n");
		exit(1);
	}

	printf("done inv_accept\n");

}


void inv_reject(int sockfd, unsigned char data[]){

	printf("in inv_reject\n");
	struct Node * source_client = find_client(sockfd, NULL);
	if(source_client == NULL){
		printf("user not found\n");
	}

	char delim = ',';
	char * recipient = strtok(data, &delim);
	char * sessionID = strtok(NULL, &delim);

	strcat(source_client->client.usr.session_ID, sessionID);
	strcat(source_client->client.usr.session_ID, ",");

	struct Node * dest_client = find_client(0, recipient);
	if(dest_client == NULL){
		printf("user not found\n");
	}

	char packet_string[MAX_CHAR];
	struct message * m = create_message(INV_RESPONSE, "", "rejected");
	strcpy(packet_string, message_to_string(m));
	if(send(dest_client->client.sockfd, packet_string, sizeof(packet_string), 0) == -1){
		perror("Error calling send()");
		printf("Exiting...\n");
		exit(1);
	}

	printf("done inv_reject\n");
}