#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>



int main() {
	char input[100], command[100];
	int socketfd;
	//pthread_t rec_thread;

	while(true) {
		fgets(input, 100, stdin);
        sscanf(input, "%s", &command);

        if (strcmp(command, '/login') == 0) {
            //login();
            printf("login..\n");
        } else if (strcmp(command, '/logout') == 0) {
            //logout();
            printf("logout..\n");
        } else if (strcmp(command, '/joinsession') == 0) {
            //joinsession();
            printf("join..\n");
        } else if (strcmp(command, '/leavesession') == 0) {
            //leavesession();
            printf("leave..\n");
        } else if (strcmp(command, '/createsession') == 0) {
            //createsession();
            printf("create..\n");
        } else if (strcmp(command, '/list') == 0) {
            //list();
            printf("list..\n");
        } else if (strcmp(command, '/quit') == 0) {
            //logout();
            printf("quit..\n");
            break;
        } else {
            //send_text();
            printf("send..\n");
        }
    }

	return 0;
}