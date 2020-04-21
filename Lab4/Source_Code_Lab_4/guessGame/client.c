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
     
    if(sendto(sockfd, "start", 5 , 0 , (struct sockaddr *) &serverAddr, slen)==-1){
        error_exit("sendto()");
    }
    int recv_len = 0;
    while(1){
        
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        recv_len = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &serverAddr, &slen);
        if(recv_len == -1){
            error_exit("recvfrom()");
        }

        buf[recv_len] = '\0';
        printf ("%s\n", buf);

        scanf("%s", message);
        //send the message
        if(sendto(sockfd, message, strlen(message) , 0 , (struct sockaddr *) &serverAddr, slen)==-1){
            error_exit("sendto()");
        }
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        if(strcmp(message, "exit") == 0){   
            break;
        }
        
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        recv_len = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &serverAddr, &slen);
        if(recv_len == -1){
            error_exit("recvfrom()");
        }
        
        buf[recv_len] = '\0';
        printf("Received result : %s\n", buf);        
    }
 
    close(sockfd);
    return 0;
}

