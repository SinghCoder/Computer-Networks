#include "client.h"

/**
 * @brief Prints error along with a msg of your choice and exits the program
 * 
 */
void error_exit(char *s){
    perror(s);
    exit(1);
}

// Returns the local date/time formatted as 2014-03-19 11:11:52
char* getFormattedTime(void) {

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    // Must be static, otherwise won't work
    char *_retval = (char*) malloc(sizeof(char) * 20);
    
    strftime(_retval, sizeof(_retval), "%H:%M:%S", timeinfo);

    char ms[8];
    snprintf(ms, sizeof(ms), "%.06ld", tv.tv_usec);
    strncat(_retval, ms, 28);
    return _retval;
}

/**
 * @brief print logs to the file
 * 
 * @param actionNode where the action happened?
 * @param event what was it? sent/rcvd/rejected/timeout/drop/etc
 * @param type data packet or ack pkt
 * @param seqNum seq num of pkt
 * @param sourceNode where pkt came from
 * @param destNode where it was destined for
 */

void printLog(nodeName actionNode, action event, packet_category type, int seqNum, nodeName sourceNode, nodeName destNode){
    char* timestamp = getFormattedTime();
    fprintf(logFilePtr, "%s | %s | %s | %s | %d | %s | %s |\n", timestamp, nodeNameStr[actionNode], actionStr[event], packetTypeStr[type], seqNum, nodeNameStr[sourceNode], nodeNameStr[destNode] );
    fflush(logFilePtr);
}

/**
 * @brief To print log on screen
 * 
 * @param act - SENT/RCVD
 * @param seq_num Sequence number
 * @param size how many bytes?
 * @param ch_id which channel?
 */
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

/**
 * @brief sends file chunks to server
 * 
 * @param chnlNum which channel number is this
 * @param fileFd shared file descriptor
 * @param numChunks total number of chunks in the file
 * @param semId semaphore Id, semaphore is required for critical section of seeking and reading file
 */
void sendToServer(channel_id chnlNum, int fileFd, int numChunks, int semId){
    struct sockaddr_in serverAddr;  // server address structure
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

    /**
     * @brief a buffering data packet
     * and the ack packet
     */
    packet_t dataPkt, ackPkt;
    struct sembuf semops[1];    // single semaphore
    int bytesRead = 0;  // number of bytes read from file
    int numTries = 0;   // number of times a chunk has been sent, drop after max tries
    
    do{ // run until all chunks have been sent
        /**
         * @brief Enter critical section
         */
        semops[0].sem_num = 0;
        semops[0].sem_op = -1;
        semops[0].sem_flg = 0;

        if(semop(semId, semops, 1) < 0)
            error_exit("semop error");

                bytesRead = read(fileFd, dataPkt.data, CHUNK_SIZE);
                
                if(bytesRead <= 0){
                    break;  // no more chunk remaining to send
                }

                // create data packet
                dataPkt.data[bytesRead] = '\0';        
                dataPkt.category = DATA;
                dataPkt.channel_id = chnlNum;
                dataPkt.data_size = bytesRead;
                dataPkt.seq_num = lseek(fileFd, 0, SEEK_CUR) - bytesRead;

        semops[0].sem_num = 0;
        semops[0].sem_op = 1;
        semops[0].sem_flg = 0;
        
        if(semop(semId, semops, 1) < 0) // release critical section
            error_exit("semop error");

        if(dataPkt.seq_num < 0){
            error_exit("Error seeking");
        }

        if( (numChunks - 1) * CHUNK_SIZE <= dataPkt.seq_num){   // if last pkt
            dataPkt.is_last = true;
        }
        else{
            dataPkt.is_last = false;        
        }

        // send till max tries
        if( (numTries < MAX_TRIES) && send(sockfd, &dataPkt, sizeof(dataPkt), 0) < 0){
            error_exit("socket send error");
        }
        else{
            numTries++;
            print_trace(SENT, dataPkt.seq_num, dataPkt.data_size, dataPkt.channel_id);
            printLog(CLIENT, SENT, DATA, dataPkt.seq_num, CLIENT, SERVER);
        }

        fd_set rcvSet;
        int n;
        
        struct timeval tv;

        for( ; ; ){           // check readability of socket for recieving ack 

            FD_ZERO(&rcvSet);
            FD_SET(sockfd, &rcvSet);    // add socket to rcvSet

            tv.tv_sec = TIMEOUT_S;      // apply timer
            tv.tv_usec = 0;

            if((n = select(sockfd + 1, &rcvSet, NULL, NULL, &tv) ) < 0){
                error_exit("select error");
            }

            else if(n == 0) {     // timeout expired, send packet again, if within max_tries upper_limit
                
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
                        printLog(CLIENT, RETRANSMIT, DATA, dataPkt.seq_num, CLIENT, SERVER);
                    }
                }
            }

            else{   // socket is readable, receive an ACK
                if(recv(sockfd, &ackPkt, sizeof(ackPkt), 0) < 0){
                    error_exit("recv ack error");
                }
                else{
                    numTries = 0;
                    if(ackPkt.seq_num != dataPkt.seq_num){
                        continue;
                    }
                    print_trace(RCVD, ackPkt.seq_num, ackPkt.data_size, ackPkt.channel_id);
                    printLog(CLIENT, RCVD, ACK, ackPkt.seq_num, SERVER, CLIENT);
                }
                if(ackPkt.is_last){ 
                    return;
                }
                break;
            }
        }
    }while( bytesRead > 0); // end of sending packets

    if(close(sockfd) < 0){
        error_exit("close sockfd error");
    }
}

int main(){

    logFilePtr = fopen(CLIENT_LOG_FILE, "w");

    pid_t pid_ch1, pid_ch2;    

    FILE *inpFilePtr = fopen(INPUT_FILE, "rb");
    
    if(inpFilePtr == NULL){
        error_exit("opening input file error");
    }

    // get file size and number of chunks to be sent
    fseek(inpFilePtr, 0, SEEK_END);
    
        int fileSize = ftell(inpFilePtr);
        int numChunks = fileSize / CHUNK_SIZE;
        
        if(fileSize % CHUNK_SIZE != 0)
            numChunks++;

    fclose(inpFilePtr);

    // open file as file descriptor, so that it is shared between both processes
    int fileFd = open(INPUT_FILE, O_RDONLY);

    if(fileFd < 0){
        error_exit("Could not open input file");
    }

    // cvreate a semaphore for accessing file to seek and read
    int semId = semget(IPC_PRIVATE, 1, 0666);

    if(semId < 0){
        error_exit(" semget failed");
    }
    
    semArgs.val = 1;
    if(semctl(semId, 0, SETVAL, semArgs) < 0){
        error_exit("semctl failed");
    }

    pid_ch1 = fork();    

    switch(pid_ch1){
        case 0: // child_one, handles channel one
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
        default:    //parent, handels channel two
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