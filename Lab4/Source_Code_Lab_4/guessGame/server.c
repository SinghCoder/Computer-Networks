/*  Simple udp server */

#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data
 
void error_exit(char *s){
    perror(s);
    exit(1);
}
 
int main(void){
    struct sockaddr_in serverAddr, clientAddr;
    int sockfd, i, slen = sizeof(clientAddr) , recv_len;
    char buf[BUFLEN];
     
    //create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd == -1){
        error_exit("socket");
    }
     
    // zero out the structure
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
     
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(sockfd , (struct sockaddr*)&serverAddr, sizeof(serverAddr) ) == -1){
        error_exit("bind");
    }
     
    recv_len = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &clientAddr, &slen);
    if (recv_len == -1){
        error_exit("recvfrom()");
    }
    buf[recv_len] =  '\0';
    if(strcmp(buf, "exit") == 0){
        close(sockfd);
        return 0;    
    }
    //keep listening for data
    while(1){
        fflush(stdout);

        snprintf(buf, BUFLEN, "Enter a number between 1-10\n");
        if(sendto(sockfd, buf, strlen(buf) , 0, (struct sockaddr*) &clientAddr, slen) == -1){
            error_exit("sendto()");
        }

        int randomNo = (rand() % 10) + 1;
        //try to receive some data, this is a blocking call
        recv_len = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &clientAddr, &slen);
        if (recv_len == -1){
            error_exit("recvfrom()");
        }
        buf[recv_len] = '\0';        

        if(strcmp(buf, "exit") == 0) {
            break;
        }

        printf("Random number is %d\n", randomNo);
        if(randomNo == atoi(buf)){
            snprintf(buf, BUFLEN, "You win!!\n");
        }
        else{
            snprintf(buf, BUFLEN, "You lose!!\n");
        }
        if(sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*) &clientAddr, slen) == -1){
            error_exit("sendto()");
        }
        //print details of the client/peer and the data received
        // printf("Received packet from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        // printf("Data: %s\n" , buf);        
    }
    close(sockfd);
    return 0;
}

