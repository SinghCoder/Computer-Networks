/* Simple udp client */
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUFLEN 512  //Max length of buffer

#define PORT 8888   // The port on which to send data

void error_exit(char *msg){
    perror(msg);
    exit(1);
}

int main(void){

    struct sockaddr_in serverAddr;
    int sockfd, i, slen=sizeof(serverAddr);
    char buf[BUFLEN];
    char message[BUFLEN];
    
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd == -1){
        error_exit("socket");
    }
 
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
     
    
    while(1){
        printf("Enter message for server, or exit to leave: ");
        
        scanf("%s", message);        
         
        //send the message
        if(sendto(sockfd, message, strlen(message) , 0 , (struct sockaddr *) &serverAddr, slen)==-1){
            error_exit("sendto()");
        }
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if(recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &serverAddr, &slen) == -1){
            error_exit("recvfrom()");
        }
         
        printf("Received from server : %s\n", buf);
        if(strcmp(message, "exit") == 0){   
            break;
        }
    }
 
    close(sockfd);
    return 0;
}

