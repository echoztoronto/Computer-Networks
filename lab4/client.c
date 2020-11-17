#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAXCHAR 100
#define SOCKET_ERROR -1


//login <client ID> <password> <server-IP> <server-port>
//log into the server at the given address and port
void login(char input[], int *socketfd, bool *logged);

//logout
//exit the server
void logout(int *socketfd, bool *logged);

//joinsession <session ID> 
//join the session with given session id
void joinsession(char input[], int socketfd, bool *joined);

//leavesession
//leave current session
void leavesession(int socketfd, bool *joined);

//createsession <session ID>
//create a new session and join it
void createsession(char input[], int socketfd, bool *joined);

//list
//get the list of connected clients and available sessions
void list();

//quit
//terminate the program
void quit();

//send a message to current session
void sendtext(char input[], int socketfd);

//receive messages from server (using a thread)
//void receivemessage();

//display all valid commands
void help();

//connect to server (helper for login)
int connect_server(char ip[], char port[]);



int main() {
	char input[MAXCHAR], command[MAXCHAR];
	int socketfd;
    bool logged = false;
    bool joined = false;
    
    //use thread for receiving messages
	//pthread_t rec_thread;

	while(true) {
		fgets(input, MAXCHAR, stdin);
        sscanf(input, "%s", &command);

        if (strcmp(command, "/login") == 0) {
            if(logged) {
                printf("you are already logged in!\n");
            } else {
                login(input, &socketfd, &logged);
                
            }
        } else if (strcmp(command, "/logout") == 0) {
            if(!logged) {
                printf("you are not logged in!\n");
            } else {
                logout(&socketfd, &logged);
            }
        } else if (strcmp(command, "/joinsession") == 0) {
            if(!logged) {
                printf("you are not logged in!\n");
            }
            else if(joined) {
                printf("you can only join one session!\n");
            } else {
                joinsession(input, socketfd, &joined);
            }
            
        } else if (strcmp(command, "/leavesession") == 0) {
            if(joined) {
                leavesession(socketfd, &joined);
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
                createsession(input, socketfd, &joined);
            }
        } else if (strcmp(command, "/list") == 0) {
            list();
        } else if (strcmp(command,"/quit") == 0) {
            quit();
            break;
        } else if (strcmp(command,"/help") == 0) {
            help();
        } else {
            if(!logged || !joined) {
                printf("Invalid command. Use /help to see all valid commands.\n");
            } else {
                sendtext(input, socketfd);
            }                
        }
    }

	return 0;
}



//login <client ID> <password> <server-IP> <server-port>
//log into the server at the given address and port
//send LOGIN <client ID, password>
//receive LO_ACK or LO_NAK <reason> 
void login(char input[], int *socketfd, bool *logged) {
    char command[MAXCHAR], clientID[MAXCHAR], password[MAXCHAR], serverIP[MAXCHAR], serverPort[MAXCHAR];
    sscanf(input, "%s %s %s %s %s", &command, &clientID, &password, &serverIP, &serverPort);

    //*socketfd = connect_server(serverIP, serverPort);
    
    if(*socketfd != SOCKET_ERROR) {
        printf("cannot log you in, please try again\n");
    } else {
        *logged = true;
        printf("logging in..\n");
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
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return SOCKET_ERROR;
    }
    
    //create a socket 
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(socketfd < 0) {
        printf("Failed to create socket\n");
        return SOCKET_ERROR;
    }
    
    //connect to server
    connectfd = connect(socketfd, res->ai_addr, res->ai_addrlen);
    if(connectfd < 0 ) {
        printf("Failed to connect to server: %s ,port %s\n", ip, port);
        return SOCKET_ERROR;
    }
    
    return socketfd;
}

//logout
//exit the server
//send EXIT
void logout(int *socketfd, bool *logged) {
    
    
    printf("logging out..\n");
    
    *logged = false;
}


//joinsession <session ID> 
//join the session with given session id
//send JOIN <session ID>
//receive JN_ACK <session ID> or JN_NAK <session ID, reason>
void joinsession(char input[], int socketfd, bool *joined) {
    
    
    printf("joining the session..\n");
    
    *joined = true;
}

//leavesession
//leave current session
//send LEAVE_SESS
void leavesession(int socketfd, bool *joined) {
    
    
    printf("leaving the session..\n");
    
    *joined = false;
}

//createsession <session ID>
//create a new session and join it
//send NEW_SESS
//receive NS_ACK <session ID>
void createsession(char input[], int socketfd, bool *joined) {
    
    
    printf("creating a session..\n");
    
    *joined = true;
}

//list
//get the list of connected clients and available sessions
//send QUERY
//receive QU_ACK <users and sessions>
void list() {
    printf("list..\n");
}

//quit
//terminate the program
void quit() {
    printf("quitting..\n");
}


//send a message to current session
//send MESSAGE <message data>
void sendtext(char input[], int socketfd) {
    printf("sending..\n");
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
