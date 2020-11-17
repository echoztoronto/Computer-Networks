#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
//packet types
#define LOGIN 1
#define LO_ACK 2
#define LO_NAK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define NEW_SESS 9
#define NS_ACK 10
#define MESSAGE 11
#define QUERY 12
#define QU_ACK 13

#define MAX_NAME 100
#define MAX_DATA 1000


struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};


//print the message
void print_message(struct message *m) {
    printf("type: %d, size: %d, source: %s, data: %s\n", m->type, m->size, m->source, m->data);
}


//constructor for struct message
struct message* create_message(unsigned int type, unsigned int size, unsigned char source[], unsigned char data[]) {
    
    struct message *m = malloc(sizeof(struct message));
    
    m->type = type;
    m->size = size;
    strcpy(m->source, source);
    strcpy(m->data, data);
    
    return m;
}


//convert message struct to string (type:size:source:data)
char* message_to_string(struct message* m) {
    
    char *buf = malloc(sizeof(char)*MAX_DATA);
    sprintf(buf, "%d:%d:%s:%s", m->type, m->size, &m->source, &m->data);
    
    return buf;
}


//convert string (type:size:source:data) to message struct 
struct message* string_to_message(char buf[]) {
    
    struct message *m = malloc(sizeof(struct message));
    
    //TO DO
    
    return m; 
}