#include "client.h"

void error_exit(char *s){
    perror(s);
    exit(1);
}

void sendToServer(channel_id chnlNum, FILE *filePtr, int numChunks){
    
    struct sockaddr_in serverAddr;
    int sockfd, slen=sizeof(serverAddr);

    /* Initialize sockaddr_in data structure */
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        error_exit("socket creation error");
    }

    /* Attempt a connection */
    if(connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        error_exit("connect child1 error");
    }

    packet_t dataPkt, ackPkt;
    int bytesRead = 0;

    for(int i = chnlNum; i < numChunks; i += 2){
        fseek( filePtr, i * CHUNK_SIZE, SEEK_SET);
        bytesRead = fread(dataPkt.data, 1, CHUNK_SIZE, filePtr);

        dataPkt.data[bytesRead] = '\0';
        // printf("%s\n", dataPkt.data);

        if(bytesRead == 0){
            if(feof( filePtr )){
                printf("I'm process %d, and I have done my work\n", getpid());
                printf("At the end I read %s", dataPkt.data);
                // return;
            }
            else if(ferror( filePtr )){
                error_exit(" file read error");
            }
        }
        
        // dataPkt.data[bytesRead] = '\0';
        dataPkt.category = DATA;
        dataPkt.channel_id = chnlNum;
        dataPkt.data_size = bytesRead;
        dataPkt.seq_num = i * CHUNK_SIZE;
        if(i == numChunks - 1 || i == numChunks - 2)
            dataPkt.is_last = true;
        else
            dataPkt.is_last = false;        

        if( send(sockfd, &dataPkt, sizeof(dataPkt), 0) < 0){
            error_exit("socket send error");
        }

        fd_set rcvSet;
        int n;
        
        struct timeval tv;

        for( ; ; ){            

            FD_ZERO(&rcvSet);
            FD_SET(sockfd, &rcvSet);

            tv.tv_sec = TIMEOUT;
            tv.tv_usec = 0;

            if((n = select(sockfd + 1, &rcvSet, NULL, NULL, &tv) ) < 0){
                error_exit("select error");
            }

            else if(n == 0) {     // timeout expired, send packet again
                printf("Timeout occured\n");
                if (send(sockfd , &dataPkt, sizeof(dataPkt), 0) < 0 ){
                    error_exit("send after timeout error");
                }
            }

            else{
                if(recv(sockfd, &ackPkt, sizeof(ackPkt), 0) < 0){
                    error_exit("recv ack error");
                }
                break;
            }
        }
    }

    if(close(sockfd) < 0){
        error_exit("close sockfd error");
    }
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
    int numChunks = fileSize / CHUNK_SIZE;
    
    if(fileSize % CHUNK_SIZE != 0)
        numChunks++;

    fclose(inpFilePtr);

    inpFilePtr = fopen(INPUT_FILE, "rb");
    
    if(inpFilePtr == NULL){
        error_exit("opening input file error");
    }   

    switch(pid_ch1){
        case 0: // child_one
        {
            printf("Hello from child %d, I will handle odd chunks and fileSize is %d bytes from filePtr\n", getpid(), fileSize);
            
            sendToServer(CHANNEL_ONE, inpFilePtr, numChunks);

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

                sendToServer(CHANNEL_TWO, inpFilePtr, numChunks);

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