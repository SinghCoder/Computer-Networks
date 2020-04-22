#include "client.h"

void error_exit(char *s){
    perror(s);
    exit(1);
}

int main(){

    pid_t pid_ch1, pid_ch2;

    pid_ch1 = fork();

    FILE *inpFilePtr = fopen(INPUT_FILE, "rb");
    
    if(inpFilePtr == NULL){
        error_exit("opening input file error");
    }

    fseek(inpFilePtr, 0, SEEK_END);
    int fileSize = ftell(inpFilePtr);
    fclose(inpFilePtr);

    inpFilePtr = fopen(INPUT_FILE, "rb");
    
    if(inpFilePtr == NULL){
        error_exit("opening input file error");
    }

    struct sockaddr_in serverAddr;
    int sockfd, slen=sizeof(serverAddr);

    switch(pid_ch1){
        case 0: // child_one
        {
            printf("Hello from child %d, I will handle odd chunks and fileSize is %d bytes from filePtr\n", getpid(), fileSize);
            
            /* Initialize sockaddr_in data structure */
            memset((char *) &serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(SERVER_PORT);
            serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

            /* Attempt a connection */
            if(connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
                error_exit("connect child1 error");
            }

            fclose(inpFilePtr);
            exit(EXIT_SUCCESS);
        }
        break;
        case -1: // -1 => error
        {
            error_exit("forking child 1 error");
        }
        break;
        default:    //parent
        {
            pid_ch2 = fork();
            if(pid_ch2 < 0){
                error_exit("forking child 2 error");
            }
            else if(pid_ch2 == 0){  // child 2
                printf("Hello from child %d, I will handle even chunks and fileSize is %d bytes\n", getpid(), fileSize);

                /* Initialize sockaddr_in data structure */
                memset((char *) &serverAddr, 0, sizeof(serverAddr));
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(SERVER_PORT);
                serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

                /* Attempt a connection */
                if(connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
                    error_exit("connect child2 error");
                }

                fclose(inpFilePtr);
                exit(EXIT_SUCCESS);
            }
            else{   //parent
                printf("Hello from parent %d, fileSize is %d bytes\n", getpid(), fileSize);
                fclose(inpFilePtr);
                while(wait(NULL) > 0);
            }
        }
        break;
    }    
}