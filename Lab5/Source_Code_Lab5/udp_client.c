/*
    Simple udp client with stop and wait functionality
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
 
#define BUFLEN 512  //Max length of buffer
#define PORT 8882   //The port on which to send data

typedef struct packet1{
    int sq_no;
}ACK_PKT;

typedef struct packet2{
    int sq_no;
    char data[BUFLEN];
}DATA_PKT;

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(void)
{
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];
    DATA_PKT send_pkt,rcv_ack;
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
	si_other.sin_addr.s_addr = inet_addr("127.0.0.1");
     
 int state = 0;
    while(1)
    {      

	switch(state)
	{  case 0: printf("Enter message 0: ");//wait for sending packet with seq. no. 0
                  fgets(send_pkt.data,sizeof(send_pkt),stdin);
 			send_pkt.sq_no = 0;		
			if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_other, slen)==-1)
        		{
            	  die("sendto()");
        		}
			state = 1; 
			break;
			
				 
		       
	   case 1:   //waiting for ACK 0
                  if (recvfrom(s, &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &si_other, &slen) == -1)
        		{
            		  die("recvfrom()");
        		}
		  if (rcv_ack.sq_no==0)
                     {  printf("Received ack seq. no. %d\n",rcv_ack.sq_no);
                             state = 2; 
			     break;
                     }
		 
	    case 2:   
			 printf("Enter message 1: ");  
                  //wait for sending packet with seq. no. 1
                   fgets(send_pkt.data,sizeof(send_pkt),stdin);
 			 send_pkt.sq_no = 1;		
			if (sendto(s, &send_pkt, sizeof(send_pkt) , 0 , (struct sockaddr *) &si_other, slen)==-1)
        		{
            	  die("sendto()");
        		}
			state = 3; 
			break;

                 
         case 3:	//waiting for ACK 1
		      if(recvfrom(s, &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &si_other, &slen) == -1)
        		 {
            	   die("recvfrom()");
        		 }
			if (rcv_ack.sq_no==1)
                     { printf("Received ack seq. no. %d\n",rcv_ack.sq_no);
                       state = 0; 
			     break;
                     }       
      }

 }
    close(s);
    return 0;
}

