#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 256
void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    int sockfd = 0;
    int bytesReceived = 0;
    char recvBuff[CHUNK_SIZE];
    unsigned char buff_offset[10];   // buffer to send the File offset value
    unsigned char buff_command[2];   // buffer to send the Complete File (0) or Partial File Command (1). 
    int offset;                      // required to get the user input for offset in case of partial file command
    int command;                     // required to get the user input for command
    memset(recvBuff, '0', sizeof(recvBuff));
    struct sockaddr_in serverAddr;

    /* Create a socket first */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        error_exit("socket");
    }

    /* Initialize sockaddr_in data structure */
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5001); // port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
    /* Create file where data will be stored */
        FILE *fp;     
        fp = fopen("destination_file.txt", "ab"); 

        if(NULL == fp){
            error_exit("fopen");
        }

        fseek(fp, 0, SEEK_END);
        offset = ftell(fp);
        fclose(fp);
        
        fp = fopen("destination_file.txt", "ab"); 
        
        if(NULL == fp){
          error_exit("fopen");
        }
    
    printf("Enter (0) to get complete file, (1) to specify offset, (2) calculate the offset value from local file\n");
    scanf("%d", &command);
    snprintf(buff_command, 2 * sizeof(char), "%d", command);
    // write(sockfd, buff_command, 2);
    if(sendto(sockfd, buff_command, 2 * sizeof(char), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        error_exit("sendto");
    }
   
    if(command == 1 || command == 2)   // We need to specify the offset
    {
    
        if(command == 1)  // get the offset from the user
        {
            printf("Enter the value of File offset\n");
            scanf("%d", &offset);
        }
        // otherwise offset = size of local partial file, that we have already calculated
        snprintf(buff_offset, 10, "%d", offset);
        /* sending the value of file offset */
        // write(sockfd, buff_offset, 10);
        if( sendto(sockfd, buff_offset, 10, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
            error_exit("sendto");
        }
    }
    
    // Else { command = 0 then no need to send the value of offset }
    
    
    /* Receive data in chunks of 256 bytes */    
    int serverLen = sizeof(serverAddr);
    while((bytesReceived = recvfrom(sockfd, recvBuff, CHUNK_SIZE, 0, (struct sockaddr *)&serverAddr, &serverLen)) > 0)
    {
        printf("Bytes received %d\n",bytesReceived);    
        // recvBuff[n] = 0;
        if( fwrite(recvBuff, 1,bytesReceived,fp) < 0)
            error_exit("fwrite");
        // printf("%s \n", recvBuff);
    }

    fclose(fp);

    if(bytesReceived < 0){
        error_exit("recvfrom");
    }

    if(close(sockfd) < 0){
        error_exit("close");
    }

    return 0;
}
