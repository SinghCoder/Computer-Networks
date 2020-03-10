/* Simple udp server with stop and wait functionality */
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
 
#define BUFLEN 512  //Max length of buffer
#define PORT 8882   //The port on which to listen for incoming data
 
void die(char *s)
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
    struct sockaddr_in si_me, si_other;
    int s, i, slen = sizeof(si_other) , recv_len;
    //char buf[BUFLEN];
    DATA_PKT rcv_pkt;
    ACK_PKT  ack_pkt;
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
    int state =0;
    while(1)
    {	     
	switch(state)
	  {  case 0:
		{   printf("Waiting for packet 0 from sender...\n");
                    fflush(stdout);
         
         //try to receive some data, this is a blocking call
         if ((recv_len = recvfrom(s, &rcv_pkt, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
                     {
                        die("recvfrom()");
                     }
                    if (rcv_pkt.sq_no==0)
                     {  printf("Packet received with seq. no. %d and Packet 
                        content is = %s\n",rcv_pkt.sq_no, rcv_pkt.data);
			      ack_pkt.sq_no = 0;
	  if (sendto(s, &ack_pkt, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
        		{
           		    die("sendto()");
                  }
                        state = 1;
                        break;
                  }
		 }
	   case 1:
                   {   printf("Waiting for packet 1 from sender...\n");
                       fflush(stdout);
         
                    //try to receive some data, this is a blocking call
         if ((recv_len = recvfrom(s, &rcv_pkt, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
                     {
                        die("recvfrom()");
                     }
		  if (rcv_pkt.sq_no==1)
                 { printf("Packet received with seq. no.=1 %d and Packet content
                   is= %s\n",rcv_pkt.sq_no, rcv_pkt.data);
			 ack_pkt.sq_no = 1;
	   if (sendto(s, &ack_pkt, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
        		{    die("sendto()"); }
                       state = 0;
                       break;
            }

	}   
    }
    close(s);
    return 0;
}
