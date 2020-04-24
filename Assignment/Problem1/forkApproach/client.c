#include "client.h"

void error_exit(char *s){
    perror(s);
    exit(1);
}

void sendToServer(channel_id chnlNum, int fileFd, int numChunks){
    
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
    // dataPkt.data[bytesRead] = '\0';
    // if(bytesRead < 0){
    //     error_exit("read error");
    // }

    // for(int i = chnlNum; i < numChunks; i += 2){
    while( (bytesRead = read(fileFd, dataPkt.data, CHUNK_SIZE)) > 0){
    //     fseek( filePtr, i * CHUNK_SIZE, SEEK_SET);
        // bytesRead = fread(dataPkt.data, 1, CHUNK_SIZE, filePtr);

        dataPkt.data[bytesRead] = '\0';
    //     // printf("%s\n", dataPkt.data);

        // if(bytesRead == 0){
        //     if(feof( filePtr )){
        //         printf("I'm process %d, and I have done my work\n", getpid());
        //         printf("At the end I read %s", dataPkt.data);
        //         // return;
        //     }
        //     else if(ferror( filePtr )){
        //         error_exit(" file read error");
        //     }
        // }
        
    //     // dataPkt.data[bytesRead] = '\0';
        dataPkt.category = DATA;
        dataPkt.channel_id = chnlNum;
        dataPkt.data_size = bytesRead;
        dataPkt.seq_num = lseek(fileFd, 0, SEEK_CUR);
        if(dataPkt.seq_num < 0){
            error_exit("Error seeking");
        }
    //     if(i == numChunks - 1 || i == numChunks - 2)
    //         dataPkt.is_last = true;
    //     else
    //         dataPkt.is_last = false;        

    //     if( send(sockfd, &dataPkt, sizeof(dataPkt), 0) < 0){
    //         error_exit("socket send error");
    //     }

    //     fd_set rcvSet;
    //     int n;
        
    //     struct timeval tv;

    //     for( ; ; ){            

    //         FD_ZERO(&rcvSet);
    //         FD_SET(sockfd, &rcvSet);

    //         tv.tv_sec = TIMEOUT;
    //         tv.tv_usec = 0;

    //         if((n = select(sockfd + 1, &rcvSet, NULL, NULL, &tv) ) < 0){
    //             error_exit("select error");
    //         }

    //         else if(n == 0) {     // timeout expired, send packet again
    //             printf("Timeout occured\n");
    //             if (send(sockfd , &dataPkt, sizeof(dataPkt), 0) < 0 ){
    //                 error_exit("send after timeout error");
    //             }
    //         }

    //         else{
    //             if(recv(sockfd, &ackPkt, sizeof(ackPkt), 0) < 0){
    //                 error_exit("recv ack error");
    //             }
    //             break;
    //         }
    //     }
    }

    if(close(sockfd) < 0){
        error_exit("close sockfd error");
    }
}

int main(){

    pid_t pid_ch1, pid_ch2;    

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

    int fileFd = open(INPUT_FILE, O_RDONLY);

    if(fileFd < 0){
        error_exit("Could not open input file");
    }

    pid_ch1 = fork();

    switch(pid_ch1){
        case 0: // child_one
        {
            printf("Hello from child %d, I will handle odd chunks and fileSize is %d bytes from filePtr\n", getpid(), fileSize);
            
            sendToServer(CHANNEL_ONE, fileFd, numChunks);

            if(close(fileFd) < 0){
                error_exit("close fileFd error");
            }
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
            printf("Hello from parent %d, I will handle even chunks and fileSize is %d bytes\n", getpid(), fileSize);
            
            sendToServer(CHANNEL_TWO, fileFd, numChunks);

            if(close(fileFd) < 0){
                error_exit("close fileFd error");
            }

            wait(NULL);            
        }
        break;
    }    
}