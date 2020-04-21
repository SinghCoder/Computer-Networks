#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    int sockfd = 0;
    struct sockaddr_in serverAddr, clientAddr;
    char sendBuff[1025];
    int numrv;
    int slen;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0){
        error_exit("socket");
    }

    printf("Socket retrieve success\n");

    memset(&serverAddr, '0', sizeof(serverAddr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(5001);

    if( bind(sockfd, (struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0) {
        error_exit("bind");
    }

    while(1)
    {
        unsigned char offset_buffer[10] = {'\0'}; 
        unsigned char command_buffer[2] = {'\0'}; 
        int offset;
        int command;
        
        printf("Waiting for client to send the command (Full File (0) Partial File (1)\n"); 
        
        int recv_len = recvfrom(sockfd, command_buffer, 2, 0, (struct sockaddr *) &clientAddr, &slen);

        if(recv_len < 0){
            error_exit("recvfrom");
        }
        command_buffer[recv_len] = '\0';
        
        sscanf(command_buffer, "%d", &command); 
        printf("Received packet from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        printf("Command received %s\n", command_buffer);
                
        if(command == 0)
            offset = 0;        
        else
        {
            printf("Waiting for client to send the offset\n");  
            
            if(recvfrom(sockfd, offset_buffer, 10, 0, (struct sockaddr *) &clientAddr, &slen) < 0){
                error_exit("recvfrom");
            }
            sscanf(offset_buffer, "%d", &offset);         
        }
        
            
        /* Open the file that we wish to transfer */
        FILE *fp = fopen("source_file.txt","rb");
        if(fp==NULL){
            error_exit("fopen ")  ;
        }   

        /* Read data from file and send it */
        fseek(fp, offset, SEEK_SET);
        while(1)
        {
            /* First read file in chunks of 256 bytes */
            unsigned char buff[256]={0};
            int nread = fread(buff ,1 ,256 ,fp);
            printf("Bytes read %d \n", nread);        

            /* If read was success, send data. */
            if(nread > 0)
            {
                printf("Sending \n");
                if( sendto(sockfd, buff, nread, 0, (struct sockaddr*) &clientAddr, slen) < 0 ){
                    error_exit("sendto");
                }
                // write(sockfd, buff, nread);
            }

            /*
             * There is something tricky going on with read .. 
             * Either there was error, or we reached end of file.
             */
            if (nread < 256)
            {
                if (feof(fp))
                    printf("End of file\n");

                if (ferror(fp)){
                    error_exit("read");
                }
                break;
            }


        }

        // if( close(sockfd) < 0){
        //     error_exit("close");
        // }
        sleep(1);
    }


    return 0;
}


