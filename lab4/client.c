#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "packet.h"


#define ERROR -1


//login <client ID> <password> <server-IP> <server-port>
//log into the server at the given address and port
void login(char input[], int *socketfd, bool *logged, char username[]);

//logout
//exit the server
void logout(int *socketfd, bool *logged, char username[]);

//joinsession <session ID> 
//join the session with given session id
void joinsession(char input[], int socketfd, bool *joined, char username[]);

//leavesession
//leave current session
void leavesession(int socketfd, bool *joined, char username[]);

//createsession <session ID>
//create a new session and join it
void createsession(char input[], int socketfd, bool *joined, char username[]);

//list
//get the list of connected clients and available sessions
void list(int socketfd, char username[]);

//send a message to current session
void sendtext(char input[], int socketfd, char username[]);

//receive messages from server (using a thread)
void *receivemessage(void* socketfd);  

//display all valid commands
void help();

//connect to server (helper for login)
int connect_server(char ip[], char port[]);



int main() {
	char input[MAX_CHAR], command[MAX_CHAR], username[MAX_CHAR];
	int socketfd;
    bool logged = false;
    bool joined = false;
    bool thread_on = false;
    pthread_t recv_thread;

	while(true) {
        
		fgets(input, MAX_CHAR, stdin);
        sscanf(input, "%s", &command);
        
        //turn on thread to receive messages while in session
        if(joined && !thread_on) {
            int* socketfd_p = &socketfd;
            pthread_create(&recv_thread, NULL, receivemessage, socketfd_p);
            thread_on = true;
        }
        
        //not in session but thread is on, should turn it off
        if(!joined && thread_on) {
            pthread_cancel(recv_thread);
            thread_on = false;
        }

        if (strcmp(command, "/login") == 0) {
            
            if(logged) {
                printf("you are already logged in!\n");
            } else {
                login(input, &socketfd, &logged, username);
            }
            
        } else if (strcmp(command, "/logout") == 0) {
            
            if(!logged) {
                printf("you are not logged in!\n");
            } else {
                logout(&socketfd, &logged, username);
            }
            
        } else if (strcmp(command, "/joinsession") == 0) {
            
            if(!logged) {
                printf("you are not logged in!\n");
            }
            else if(joined) {
                printf("you can only join one session!\n");
            } else {
                joinsession(input, socketfd, &joined, username);
            }
            
        } else if (strcmp(command, "/leavesession") == 0) {
            
            if(joined) {
                leavesession(socketfd, &joined, username);
            } else {
                printf("you are not in any session yet!\n");
            }
            
        } else if (strcmp(command, "/createsession") == 0) {
            if(!logged) {
                printf("you are not logged in!\n");
            }
            else if(joined) {
                printf("you are already in a session!\n");
            } else {
                createsession(input, socketfd, &joined, username);
            }
            
        } else if (strcmp(command, "/list") == 0) {
            
            list(socketfd, username);
            
        } else if (strcmp(command,"/quit") == 0) {
            
            logout(&socketfd, &logged, username);
            printf("quitting..\n");
            
            break;
            
        } else if (strcmp(command,"/help") == 0) {
            
            help();
            
        } else {
            
            if(!logged || !joined) {
                printf("Invalid command. Use /help to see all valid commands.\n");
            } else {
                sendtext(input, socketfd, username);
            }
                
        }
    }
    
	return 0;
}



//login <client ID> <password> <server-IP> <server-port>
//log into the server at the given address and port
//send LOGIN <client ID, password>
//receive LO_ACK or LO_NAK <reason> 
void login(char input[], int *socketfd, bool *logged, char username[]) {
    char command[MAX_CHAR], clientID[MAX_CHAR], password[MAX_CHAR], serverIP[MAX_CHAR], serverPort[MAX_CHAR];
    sscanf(input, "%s %s %s %s %s", &command, &clientID, &password, &serverIP, &serverPort);
    
    strcpy(username, clientID);
    *socketfd = connect_server(serverIP, serverPort);
    
    
    if(*socketfd == ERROR) {
        printf("cannot log you in, please try again\n");
    } else {
        
        //create message then convert it to string 
        char message_data[MAX_CHAR], packet_string[MAX_CHAR];
        
        strcpy(message_data, clientID);
        strcat(message_data, ",");
        strcat(message_data, password);
        struct message *m = create_message(LOGIN, clientID, message_data);
        strcpy(packet_string, message_to_string(m));
        
        //send packet_string to server
        if(send(*socketfd, packet_string, sizeof(packet_string), 0) == ERROR) {
            printf("failed to send '%s' to server\n", packet_string);
            close(*socketfd);
            *socketfd = ERROR;
            return;
        } 
        
        //receive from server
        char recv_message[MAX_CHAR];
        
        if(recv(*socketfd, (char*)recv_message, sizeof(recv_message), 0) == ERROR) {
           printf("failed to receive ACK from server\n");
           close(*socketfd);
           *socketfd = ERROR;
           return; 
        } 
        
        struct message *r = string_to_message(recv_message);
        
        //LO_ACK or LO_NAK <reason>
        if(r->type == LO_ACK) {
            *logged = true;
            strcpy(username, clientID);
            printf("you are now logged in..\n");
        }
        else if (r->type == LO_NAK) {
            printf("failed to logged in, reason: %s\n", r->data);
        }
    }
}

//connect to server 
//mostly borrowed from Beej's Guide
int connect_server(char ip[], char port[]) {
    struct addrinfo hints, *res;
    int status, socketfd, connectfd;
    
    //set socket 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(ip, port, &hints, &res)) != 0 ) {
        printf("failed to set socket\n");
        return ERROR;
    }
    
    //create a socket 
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(socketfd < 0) {
        printf("failed to create socket\n");
        return ERROR;
    }
    
    //connect to server
    connectfd = connect(socketfd, res->ai_addr, res->ai_addrlen);
    if(connectfd < 0 ) {
        close(socketfd);
        printf("failed to connect to server: %s: %s\n", ip, port);
        return ERROR;
    }
    
    return socketfd;
}

//logout
//exit the server
//send EXIT
void logout(int *socketfd, bool *logged, char username[]) {
    
    //create packet_string
    char packet_string[MAX_CHAR];
    struct message *m = create_message(EXIT, username, "");
    strcpy(packet_string, message_to_string(m));
    
    //send packet_string to server
    if(send(*socketfd, packet_string, sizeof(packet_string), 0) == ERROR) {
        printf("failed to send '%s' to server\n", packet_string);
        return;
    } 
    
    //close socket and reset variables
    close(*socketfd);
    *socketfd = ERROR;
    *logged = false;
    printf("you are logged out..\n");
}


//joinsession <session ID> 
//join the session with given session id
//send JOIN <session ID>
//receive JN_ACK <session ID> or JN_NAK <session ID, reason>
void joinsession(char input[], int socketfd, bool *joined, char username[]) {

    char command[MAX_CHAR], sessionID[MAX_CHAR];
    sscanf(input, "%s %s", &command, &sessionID);
    
    //create packet_string
    char packet_string[MAX_CHAR];
    struct message *m = create_message(JOIN, username, sessionID);
    strcpy(packet_string, message_to_string(m));
    
    //send packet_string to server
    if(send(socketfd, packet_string, sizeof(packet_string), 0) == ERROR) {
        printf("failed to send '%s' to server\n", packet_string);
        return;
    } 
        
    //receive from server
    char recv_message[MAX_CHAR];
    
    if(recv(socketfd, (char*)recv_message, sizeof(recv_message), 0) == ERROR) {
       printf("failed to receive ACK from server\n");
       return; 
    } 
        
    struct message *r = string_to_message(recv_message);
        
    //JN_ACK <session ID> or JN_NAK <session ID, reason>
    if(r->type == JN_ACK) {
        *joined = true;
        printf("you are now joined in session %s..\n", r->data);
    }
    else if (r->type == JN_NAK) {
        printf("failed to join in session %s\n", r->data);
    }
}

//leavesession
//leave current session
//send LEAVE_SESS
void leavesession(int socketfd, bool *joined, char username[]) {
    
    //create packet_string
    char packet_string[MAX_CHAR];
    struct message *m = create_message(LEAVE_SESS, username, "");
    strcpy(packet_string, message_to_string(m));
    
    //send packet_string to server
    if(send(socketfd, packet_string, sizeof(packet_string), 0) == ERROR) {
        printf("failed to send '%s' to server\n", packet_string);
        return;
    } 
    
    //close socket and reset variables
    *joined = false;
    printf("you left the session..\n");
}

//createsession <session ID>
//create a new session and join it
//send NEW_SESS
//receive NS_ACK <session ID> or NS_NAK
void createsession(char input[], int socketfd, bool *joined, char username[]) {
    
    //send NEW_SESS
    char command[MAX_CHAR], sessionID[MAX_CHAR];
    sscanf(input, "%s %s", &command, &sessionID);
    
    //create packet_string
    char packet_string[MAX_CHAR];
    struct message *m = create_message(NEW_SESS, username, sessionID);
    strcpy(packet_string, message_to_string(m));
    
    //send packet_string to server
    if(send(socketfd, packet_string, sizeof(packet_string), 0) == ERROR) {
        printf("failed to send '%s' to server\n", packet_string);
        return;
    } 
        
    //receive from server
    char recv_message[MAX_CHAR];
    
    if(recv(socketfd, (char*)recv_message, sizeof(recv_message), 0) == ERROR) {
       printf("failed to receive ACK from server\n");
       return; 
    } 
        
    struct message *r = string_to_message(recv_message);
        
    //NS_ACK <session ID> or NS_NAK
    if(r->type == NS_ACK) {
        *joined = true;
        printf("new session created: %s..\n", r->data);
    }
    else if (r->type == NS_NAK) {
        printf("failed to create session %s\n", sessionID);
    }
    
}

//list
//get the list of connected clients and available sessions
//send QUERY
//receive QU_ACK <users and sessions>
void list(int socketfd, char username[]) {
    
    //create packet_string
    char packet_string[MAX_CHAR];
    struct message *m = create_message(QUERY, username, "");
    strcpy(packet_string, message_to_string(m));
    
    //send packet_string to server
    if(send(socketfd, packet_string, sizeof(packet_string), 0) == ERROR) {
        printf("failed to send '%s' to server\n", packet_string);
        return;
    } 
        
    //receive from server
    char recv_message[MAX_CHAR];
    
    if(recv(socketfd, (char*)recv_message, sizeof(recv_message), 0) == ERROR) {
       printf("failed to receive ACK from server\n");
       return; 
    } 
        
    struct message *r = string_to_message(recv_message);
        
    //QU_AC
    if(r->type == QU_ACK) {
        printf("geting the list of connected clients and available sessions..\n");
        printf("%s", r->data);
    }
    
}


//send a message to current session
//send MESSAGE <message data>
void sendtext(char input[], int socketfd, char username[]) {
    
    //create packet_string
    char packet_string[MAX_CHAR];
    struct message *m = create_message(MESSAGE, username, input);
    strcpy(packet_string, message_to_string(m));
    
    //send packet_string to server
    if(send(socketfd, packet_string, sizeof(packet_string), 0) == ERROR) {
        printf("failed to send '%s' to server\n", packet_string);
        return;
    } 
    
    printf("(sent)\n"); 
}


void help() {
    printf("All valid commands:\n");
    printf("/login <client ID> <password> <server-IP> <server-port>\n");
    printf("/logout\n");
    printf("/joinsession <session ID>\n");
    printf("/leavesession\n");
    printf("/createsession <session ID>\n");
    printf("/list\n");
    printf("/quit\n");
    printf("/help\n");
}


//receive messages while in session
void *receivemessage(void* socketfd) {
    
    char buf[MAX_CHAR];
    int* socketfd_p = (int*)socketfd;
    
    while(true) {
        
        //receive from server
        if(recv(*socketfd_p, (char*)buf, sizeof buf, 0) != ERROR) {
            struct message *r = string_to_message(buf);
            
            if(r->type == MESSAGE) {
                printf("%s: %s\n", r->source, r->data);
            }
            free(r);
        }
        memset(buf, 0, sizeof buf);
    }
}