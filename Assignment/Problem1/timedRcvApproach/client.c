#include "client.h"

void error_exit(char *s){
    perror(s);
    exit(1);
}

void print_trace(action act, int seq_num, int size, channel_id ch_id, char *data){
    char *actionStr;
    if(act == SENT){
        actionStr = "SENT";
    }
    else{
        actionStr = "RECEIVED";
    }
    printf("Client | %s | SeqNum : %d | Size : %d | Channel : %d\n", actionStr, seq_num, size, ch_id);
    printf("Content : %s\n", data);
}

int nextChunkNum(bool *chunkSent, bool *inProgress, int numChunks){
    for(int i = 0; i < numChunks; i++){
        if(chunkSent[i] == false && inProgress[i] == false){
            chunkSent[i] = true;
            inProgress[i] = true;
            return i;
        }
    }
    return -1;
}

int main(){
    
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

    bool chunkSent[numChunks], inProgress[numChunks];

    for(int i = 0; i < numChunks; i++){
        chunkSent[i] = false;
        inProgress[i] = false;
    }

    struct sockaddr_in serverAddr;
    int sockfd[2], slen=sizeof(serverAddr);

    /* Initialize sockaddr_in data structure */
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    struct timeval timeout;      
    timeout.tv_sec = TIMEOUT_S;
    timeout.tv_usec = 0;

    int maxFd = -1;

    for(int i = 0; i < NUM_CONNECTIONS; i++){
        if ( (sockfd[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
            error_exit("socket creation error");
        }

        /* Attempt a connection */
        if(connect(sockfd[i], (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
            error_exit("connect child1 error");
        }

        if (setsockopt (sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
            error_exit("setsockopt failed for sockfd\n");
        
        if(maxFd < sockfd[i])
            maxFd = sockfd[i];
    }


    int pktsOut = 0;
    int nextChunk = 0;
    int bytesRead = 0;
    packet_t dataPkt[NUM_CONNECTIONS], ackPkt[NUM_CONNECTIONS];
    int numTries[NUM_CONNECTIONS];
    bool lastSentSuccessfully = false;
    
    for(int i = 0; i < NUM_CONNECTIONS; i++) {
        nextChunk = nextChunkNum(chunkSent, inProgress, numChunks);
        
        if(nextChunk != -1){
            fseek( inpFilePtr, nextChunk * CHUNK_SIZE, SEEK_SET);
            bytesRead = fread(dataPkt[i].data, 1, CHUNK_SIZE, inpFilePtr);

            dataPkt[i].data[bytesRead] = '\0';
            // printf("%s\n", dataPkt[i].data);

            if(bytesRead == 0){
                if(feof( inpFilePtr )){
                    // return;
                }
                else if(ferror( inpFilePtr )){
                    error_exit(" file read error");
                }
            }
            
            dataPkt[i].category = DATA;
            dataPkt[i].channel_id = i;
            dataPkt[i].data_size = bytesRead;
            dataPkt[i].seq_num = nextChunk * CHUNK_SIZE;
            if(nextChunk == numChunks - 1 || nextChunk == numChunks - 2)
                dataPkt[i].is_last = true;
            else
                dataPkt[i].is_last = false;        

            if( send(sockfd[i], &dataPkt[i], sizeof(dataPkt[i]), 0) < 0){
                error_exit("socket send error");
            }
            else{
                numTries[i] = 1;
                pktsOut++;
            }
        }
    }
    for( ; ; ){ 
        for(int i = 0; i < NUM_CONNECTIONS; i++) {
            int n = recv(sockfd[i], &ackPkt[i], sizeof(ackPkt[i]), 0);
            printf("recv return %d for i = %d\n", n, i);
            if(n < 0){
                if(errno != EAGAIN)
                    error_exit("recv ack error");
                else{
                    printf("numTries[%d] = %d\n", i, numTries[i]);
                    if(numTries[i] < MAX_TRIES){
                        printf("sending data again\n");
                        if( send(sockfd[i], &dataPkt[i], sizeof(dataPkt[i]), 0) < 0){
                            error_exit("socket send error");
                        }
                        else{
                            numTries[i]++;
                            pktsOut++;
                        }           
                    }
                    else{
                        if(numTries[i] == MAX_TRIES){
                            printf("crossed my retries, stepping backl\n");
                            chunkSent[dataPkt[i].seq_num / CHUNK_SIZE] = false;
                            inProgress[dataPkt[i].seq_num / CHUNK_SIZE] = false;
                            numTries[i]++;
                        }
                    }
                }
            }
            else if(n == 0){
                if(close(sockfd[i]) < 0){
                    error_exit("closing sockfd");
                }
            }
            else{
                pktsOut--;
                // rcv success
                inProgress[dataPkt[i].seq_num / CHUNK_SIZE] = false;

                nextChunk = nextChunkNum(chunkSent, inProgress, numChunks);
        
                if(nextChunk != -1){
                    fseek( inpFilePtr, nextChunk * CHUNK_SIZE, SEEK_SET);
                    bytesRead = fread(dataPkt[i].data, 1, CHUNK_SIZE, inpFilePtr);

                    dataPkt[i].data[bytesRead] = '\0';
                    // printf("%s\n", dataPkt[i].data);

                    if(bytesRead == 0){
                        if(feof( inpFilePtr )){
                            // return;
                        }
                        else if(ferror( inpFilePtr )){
                            error_exit(" file read error");
                        }
                    }
                    
                    dataPkt[i].category = DATA;
                    dataPkt[i].channel_id = i;
                    dataPkt[i].data_size = bytesRead;
                    dataPkt[i].seq_num = nextChunk * CHUNK_SIZE;
                    if(nextChunk == numChunks - 1)
                        dataPkt[i].is_last = true;
                    else
                        dataPkt[i].is_last = false;        

                    if( send(sockfd[i], &dataPkt[i], sizeof(dataPkt[i]), 0) < 0){
                        error_exit("socket send error");
                    }
                    else{
                        numTries[i] = 1;
                        pktsOut++;
                    }
                }

                if(ackPkt[i].seq_num / CHUNK_SIZE == numChunks - 1){
                    lastSentSuccessfully = true;
                }
            }
        }
        if(lastSentSuccessfully && pktsOut == 0)
            break;
    }
    
    fclose(inpFilePtr);
    exit(EXIT_SUCCESS);
}