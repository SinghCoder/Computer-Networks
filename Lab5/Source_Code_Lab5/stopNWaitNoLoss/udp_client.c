/*
    Simple udp client with stop and wait functionality
*/
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
 
#define BUFLEN 512  //Max length of buffer
#define PORT 8882   //The port on which to send data

typedef struct packet1{
    int sq_no;
}ACK_PKT;

typedef struct packet2{
    int sq_no;
    char data[BUFLEN];
}DATA_PKT;

void error_exit(char *s){
    perror(s);
    exit(1);
}
 
int main(void){
    struct sockaddr_in serverAddr;
    int sockfd, i, slen=sizeof(serverAddr);

    char buf[BUFLEN];
    char message[BUFLEN];

    DATA_PKT send_pkt,rcv_ack;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        error_exit("socket");
    }
 
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
     
    int state = 0;
    while(1)
    {
        switch(state)
        {
            case 0: 
            {
                printf("Enter message 0: ");//wait for sending packet with seq. no. 0
                fgets(send_pkt.data, BUFLEN, stdin);

                send_pkt.sq_no = 0;		
                if (sendto(sockfd , &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &serverAddr, slen) == -1){
                    error_exit("sendto()");
                }
                state = 1; 
            }
            break; 

            case 1:
            {//waiting for ACK 0
                if (recvfrom(sockfd , &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &serverAddr, &slen) == -1){
                    error_exit("recvfrom()");
                }
                if(rcv_ack.sq_no==0){  
                    printf("Received ack seq. no. %d\n",rcv_ack.sq_no);
                    state = 2;                     
                }                
            }
            break;

            case 2:
            {
                printf("Enter message 1: ");  
                //wait for sending packet with seq. no. 1
                fgets(send_pkt.data, BUFLEN, stdin);

                send_pkt.sq_no = 1;		
                if (sendto(sockfd, &send_pkt, sizeof(send_pkt) , 0 , (struct sockaddr *) &serverAddr, slen)==-1){
                    error_exit("sendto()");
                }
                state = 3; 
            }
            break;                    

            case 3:	//waiting for ACK 1
            {
                if(recvfrom(sockfd, &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &serverAddr, &slen) == -1){
                    error_exit("recvfrom()");
                }
                if (rcv_ack.sq_no==1){ 
                    printf("Received ack seq. no. %d\n",rcv_ack.sq_no);
                    state = 0;                     
                }       
            }
            break;
        }
    }
    if( close(sockfd) < 0){
        error_exit("close");
    }
    return 0;
}

