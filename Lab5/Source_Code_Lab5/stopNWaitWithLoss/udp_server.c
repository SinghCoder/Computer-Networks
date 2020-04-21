/* Simple udp server with stop and wait functionality */
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512  //Max length of buffer
#define PORT 8882   //The port on which to listen for incoming data
 
void error_exit(char *s)
{
    perror(s);
    exit(1);
}
 
typedef struct packet1{
    int sq_no;
}ACK_PKT;

typedef struct packet2{
    int sq_no;
    char data[BUFLEN];
}DATA_PKT;

int main(void)
{
    struct sockaddr_in serverAddr, clientAddr;
    int sockfd, i, slen = sizeof(clientAddr) , recv_len;
    //char buf[BUFLEN];
    DATA_PKT rcv_pkt;
    ACK_PKT  ack_pkt;
    //create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
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
    int state =0;
    while(1)
    {	     
        switch(state)
        {  
            case 0:
            {
                printf("Waiting for packet 0 from sender...\n");
                fflush(stdout);

                //try to receive some data, this is a blocking call
                if ((recv_len = recvfrom(sockfd, &rcv_pkt, BUFLEN, 0, (struct sockaddr *) &clientAddr, &slen)) == -1){
                    error_exit("recvfrom()");
                }
                if (rcv_pkt.sq_no == 0){  
                    printf("Packet received with seq. no. %d and Packet content is = %s\n",rcv_pkt.sq_no, rcv_pkt.data);
                    ack_pkt.sq_no = 0;
                    
                    if (sendto(sockfd, &ack_pkt, recv_len, 0, (struct sockaddr*) &clientAddr, slen) == -1){
                        error_exit("sendto()");
                    }
                    state = 1;
                    break;
                }
                else{   // send ACK1 and remain in this state
                    ack_pkt.sq_no = 1;
                    
                    if (sendto(sockfd, &ack_pkt, recv_len, 0, (struct sockaddr*) &clientAddr, slen) == -1){
                        error_exit("sendto()");
                    }
                }
            }
            case 1:
            {   
                printf("Waiting for packet 1 from sender...\n");
                fflush(stdout);

                //try to receive some data, this is a blocking call
                if ((recv_len = recvfrom(sockfd, &rcv_pkt, BUFLEN, 0, (struct sockaddr *) &clientAddr, &slen)) == -1){
                    error_exit("recvfrom()");
                }
                if (rcv_pkt.sq_no==1){ 
                    printf("Packet received with seq. no.=1 %d and Packet content is= %s\n",rcv_pkt.sq_no, rcv_pkt.data);
                    ack_pkt.sq_no = 1;
                    
                    if (sendto(sockfd, &ack_pkt, recv_len, 0, (struct sockaddr*) &clientAddr, slen) == -1){
                        error_exit("sendto()"); 
                    }
                    state = 0;
                    break;
                }
                else{   //send ACK0  and remain in this state
                    ack_pkt.sq_no = 0;
                    
                    if (sendto(sockfd, &ack_pkt, recv_len, 0, (struct sockaddr*) &clientAddr, slen) == -1){
                        error_exit("sendto()"); 
                    }
                }
            }   
        }
    }
    close(sockfd);
    return 0;
}
