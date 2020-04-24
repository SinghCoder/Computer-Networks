#include "client.h"

void error_exit(char *s){
    perror(s);
    exit(1);
}

void print_trace(action act, int seq_num, int size, channel_id ch_id){
    char *actionStr;
    if(act == SENT){
        actionStr = "SENT PKT";
    }
    else{
        actionStr = "RCVD PKT";
    }
    printf("%s | Seq. No : %d | Size : %d bytes | Channel : %d\n", actionStr, seq_num, size, ch_id);    
}

void sendToServer(channel_id chnlNum, int fileFd, int numChunks, int semId){
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

    struct timeval timeout;      
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        error_exit("setsockopt failed for sockfd\n");

    packet_t dataPkt, ackPkt;
    struct sembuf semops[1];
    int bytesRead = 0;
    int numTries = 0;
    do{
        semops[0].sem_num = 0;
        semops[0].sem_op = -1;
        semops[0].sem_flg = 0;
        if(semop(semId, semops, 1) < 0)
            error_exit("semop error");

        bytesRead = read(fileFd, dataPkt.data, CHUNK_SIZE);
        if(bytesRead <= 0){
            break;
        }
        dataPkt.data[bytesRead] = '\0';        
        dataPkt.category = DATA;
        dataPkt.channel_id = chnlNum;
        dataPkt.data_size = bytesRead;
        dataPkt.seq_num = lseek(fileFd, 0, SEEK_CUR) - bytesRead;

        semops[0].sem_num = 0;
        semops[0].sem_op = 1;
        semops[0].sem_flg = 0;
        if(semop(semId, semops, 1) < 0)
            error_exit("semop error");

        if(dataPkt.seq_num < 0){
            error_exit("Error seeking");
        }
        if( (numChunks - 1) * CHUNK_SIZE <= dataPkt.seq_num){
            dataPkt.is_last = true;
        }
        else{
            dataPkt.is_last = false;        
        }

        if( (numTries < MAX_TRIES) && send(sockfd, &dataPkt, sizeof(dataPkt), 0) < 0){
            error_exit("socket send error");
        }
        else{
            numTries++;
            print_trace(SENT, dataPkt.seq_num, dataPkt.data_size, dataPkt.channel_id);
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
                if(numTries >= MAX_TRIES){
                    printf("Too many attempts done for packet with seq num %d, now stepping back\n", dataPkt.seq_num);
                    wait(NULL);
                    exit(EXIT_SUCCESS);
                }
                else{
                    if (send(sockfd , &dataPkt, sizeof(dataPkt), 0) < 0 ){
                        error_exit("send after timeout error");
                    }
                    else{
                        numTries++;
                        print_trace(SENT, dataPkt.seq_num, dataPkt.data_size, dataPkt.channel_id);
                    }
                }
            }

            else{
                if(recv(sockfd, &ackPkt, sizeof(ackPkt), 0) < 0){
                    error_exit("recv ack error");
                }
                else{
                    numTries = 0;
                    print_trace(RECEIVED, dataPkt.seq_num, dataPkt.data_size, dataPkt.channel_id);
                }
                if(ackPkt.is_last){
                    wait(NULL);
                    exit(EXIT_SUCCESS);
                }
                break;
            }
        }
    }while( bytesRead > 0);

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

    int semId = semget(IPC_PRIVATE, 1, 0);

    if(semId < 0){
        error_exit(" semget failed");
    }
    
    semArgs.val = 1;
    if(semctl(semId, 0, SETVAL, semArgs) < 0){
        error_exit("semctl failed");
    }

    pid_ch1 = fork();    

    switch(pid_ch1){
        case 0: // child_one
        {
            
            sendToServer(CHANNEL_ONE, fileFd, numChunks, semId);

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
            sendToServer(CHANNEL_TWO, fileFd, numChunks, semId);

            if(close(fileFd) < 0){
                error_exit("close fileFd error");
            }

            wait(NULL);              
        }
        break;
    }    
}