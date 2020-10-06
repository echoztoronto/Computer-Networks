#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
    if(argc != 3) {
        perror("Usage: deliver <server address> <server port number>\n");
        exit(0);
    }
    
    return 0;
}