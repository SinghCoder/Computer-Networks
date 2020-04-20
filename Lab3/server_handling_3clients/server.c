#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAXPENDING 5
#define BUFFERSIZE 32
#define MAX_CLIENTS 3

void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int main ()
{
    /*CREATE A TCP SOCKET*/
    int sockfd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0){ 
        error_exit("socket");
    }

    printf ("Server Socket Created\n");
    
    /*CONSTRUCT LOCAL ADDRESS STRUCTURE*/
    struct sockaddr_in serverAddress, clientAddress;
    memset (&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    
    printf ("Server address assigned\n");
    
    int temp = bind(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    
    if (temp < 0){
        error_exit("bind");
    }
    printf ("Binding successful\n");

    int temp1 = listen(sockfd, MAXPENDING);
    
    if (temp1 < 0){
        error_exit("listen");
    }
    printf ("Now Listening\n");
    
    char msg[BUFFERSIZE];
    int clientLength = sizeof(clientAddress);

    for(int i=0; i<MAX_CLIENTS; i++){
        int connfd = accept (sockfd, (struct sockaddr*)&clientAddress, &clientLength);
        
        if (connfd < 0){
            error_exit("accept");
        }
        printf ("Handling Client %s\n", inet_ntoa(clientAddress.sin_addr));

        int temp2 = recv(connfd, msg, BUFFERSIZE, 0);
        if (temp2 < 0){ 
            error_exit("recv");
        }
        printf("Received from client : %s\n", msg);
        
        printf ("ENTER MESSAGE FOR CLIENT\n");
        fgets(msg, BUFFERSIZE, stdin);
        
        int bytesSent = send (connfd, msg, strlen(msg), 0);
        if (bytesSent != strlen(msg)){
            error_exit("send");
        }

        if( close(connfd) < 0){
            error_exit("close connfd");
        }
    }
    if( close(sockfd)  < 0){
        error_exit("close sockfd");
    }
}