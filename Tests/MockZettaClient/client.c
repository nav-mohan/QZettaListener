#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024 
#define SERVER_PORT 10000

int main(int argc, char *argv[])
{
    // Create a socket
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Connect to the server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERVER_PORT);
    int connect_status = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("CONNECTED %d \n",connect_status);
    if(connect_status != 0){
        printf("Failed to connect\n");
        return connect_status;
    }
    FILE* xml_file;

    char *filename = argv[1];
    xml_file = fopen(filename,"r");
    int bytes_read = 0;
    unsigned char xml_buffer[BUFFER_SIZE];
    do
    {
        sleep(1); // withtout this SLEEP you'll end up sending the whole audio file at the same time
        memset(xml_buffer,0,BUFFER_SIZE);
        bytes_read = fread(xml_buffer,1,BUFFER_SIZE,xml_file);
        int bytes_sent = send(sockfd,xml_buffer,bytes_read,0);
        printf("\nread %d | sent %d\n",bytes_read,bytes_sent);
    } while (bytes_read > 0);

    // Close the socket
    close(sockfd);

    return 0;
}
