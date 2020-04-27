#include "client.h"

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
void printLog(nodeName actionNode, action event, packetType type, int seqNum, nodeName sourceNode, nodeName destNode){
    char* timestamp = getFormattedTime();
    fprintf(logFilePtr, "%s | %s | %s | %s | %d | %s | %s |\n", timestamp, nodeNameStr[actionNode], actionStr[event], packetTypeStr[type], seqNum, nodeNameStr[sourceNode], nodeNameStr[destNode] );
    fflush(logFilePtr);
}

int main(){
    
    // open file to write logs
    logFilePtr = fopen(CLIENT_LOG_FILE, "w");
    
    // even and odd relay addresses
    int sockfdEven, sockfdOdd;
    struct sockaddr_in relayEvenAddr, relayOddAddr;

    // store data packets for each window element to assisst retransmission
    packet_t dataPkts[WINDOW_SIZE], ackPkt;

    sockfdEven = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfdEven < 0){
        error_exit("even pkts socket creation error");
    }
    
    sockfdOdd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfdOdd < 0){
        error_exit("odd pkts socket creation error");
    }    

    /* Initialize sockaddr_in data structure for relay 1*/
    relayEvenAddr.sin_family = AF_INET;
    relayEvenAddr.sin_port = htons(RELAY1_PORT); // port
    relayEvenAddr.sin_addr.s_addr = inet_addr(RELAY1_IP);

    /* Initialize sockaddr_in data structure for relay 2*/
    relayOddAddr.sin_family = AF_INET;
    relayOddAddr.sin_port = htons(RELAY2_PORT); // port
    relayOddAddr.sin_addr.s_addr = inet_addr(RELAY2_IP);

    FILE *inpFilePtr = fopen(INPUT_FILE, "rb");
    
    if(inpFilePtr == NULL){
        error_exit("opening input file error");
    }

    // get file size and numbner of chunks to write
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

    // array corresponding to each packet sent telling whether ack is received or not. NA=> data pkt was not sent for this
    ackstatus_t ackRcvd[WINDOW_SIZE] = { NA };
    // any ack not received yet?
    bool someoneLeft = false;
    int numRead;    // bytes read from file
    int windowStartSeqNum = -1; // starting sequence number for window

    struct pollfd poll_fds[2];  // fds for both relay to check for ACK

    poll_fds[0].fd = sockfdEven;
    poll_fds[1].fd = sockfdOdd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].events = POLLIN;

    // get number of windows to be sent
    int numWindows = (int) (ceil( ( (numChunks + 0.0) / (WINDOW_SIZE + 0.0) ) ));

    for(int i = 0; i < numWindows ; i++){
        
        // mark each ack recvd or not to be Not applicable, since data pkt might not exist corresp to it
        memset(ackRcvd, NA, sizeof(ackstatus_t) * WINDOW_SIZE);

        someoneLeft = false;

        for(int j = 0; j < WINDOW_SIZE; j++){

            dataPkts[j].seq_num = ftell(inpFilePtr);
            
            numRead = fread(dataPkts[j].data, 1, CHUNK_SIZE, inpFilePtr);
            
            if(numRead == 0 && ferror(inpFilePtr)){
                error_exit("reading chunk failed");
            }

            if(numRead == 0){
                break;
            }

            dataPkts[j].data[numRead] = '\0';
            dataPkts[j].category = DATA;
            dataPkts[j].data_size = numRead;
            
            if(dataPkts[j].seq_num + dataPkts[j].data_size == fileSize) // have read last pkt
                dataPkts[j].is_last = true;
            else
                dataPkts[j].is_last = false;            

            ackRcvd[j] = NOTRECEIVED;   // mark ack rcvd false =>data sent
            someoneLeft = true;

            if( (dataPkts[j].seq_num / CHUNK_SIZE) % 2 == 0 ){  // even chunk, send to even relay
                if(sendto(sockfdEven, &(dataPkts[j]), sizeof(dataPkts[j]), 0, (struct sockaddr *)&relayEvenAddr, sizeof(relayEvenAddr)) < 0){
                    error_exit("sendto even relay failed");
                }
                else{
                    printLog(CLIENT, SENT, DATA, dataPkts[j].seq_num, CLIENT, RELAY1);
                }
            }
            else{   // odd chunk send to odd relay
                if(sendto(sockfdOdd, &(dataPkts[j]), sizeof(dataPkts[j]), 0, (struct sockaddr *)&relayOddAddr, sizeof(relayOddAddr)) < 0){
                    error_exit("sendto odd relay failed");
                }
                else{
                    printLog(CLIENT, SENT, DATA, dataPkts[j].seq_num, CLIENT, RELAY2);
                }
            }
        }

        windowStartSeqNum = dataPkts[0].seq_num;

        // check for ACKS for all elements of window
        while(someoneLeft){    //poll until all ACKs for window are received
            int poll_status = poll(poll_fds, 2, TIMEOUT_MS);

            if(poll_status == -1){
                error_exit("poll failed");
            }

            if(poll_status == 0){   // timeout occured
                // retransmit those whose ACK is not received yet
                for(int j = 0; j < WINDOW_SIZE; j++){
                    if(ackRcvd[j] == NOTRECEIVED){
                        if( (dataPkts[j].seq_num / CHUNK_SIZE) % 2  == 0 ){
                            if(sendto(sockfdEven, &(dataPkts[j]), sizeof(dataPkts[j]), 0, (struct sockaddr *)&relayEvenAddr, sizeof(relayEvenAddr)) < 0){
                                error_exit("retransmission to even relay failed");
                            }
                            printLog(CLIENT, RETRANSMIT, DATA, dataPkts[j].seq_num, CLIENT, RELAY1);
                        }
                        else{
                            if(sendto(sockfdOdd, &(dataPkts[j]), sizeof(dataPkts[j]), 0, (struct sockaddr *)&relayOddAddr, sizeof(relayOddAddr)) < 0){
                                error_exit("retransmission to odd relay failed");
                            }
                            printLog(CLIENT, RETRANSMIT, DATA, dataPkts[j].seq_num, CLIENT, RELAY2);
                        }
                    }
                }
                continue;
                // return 0;
            }

            // someone is readable => ACK is rcvd
            for(int j = 0; j < 2; j++){
                if( (poll_fds[j].fd != -1) && (poll_fds[j].revents & POLLIN)){
                    int n = read(poll_fds[j].fd, &ackPkt, sizeof(ackPkt));
                    
                    if(n < 0){
                        error_exit("reading ACK failed");
                    }                   
                    
                    if(n == 0){ // server has closed the connection
                        if(close(poll_fds[j].fd) < 0){
                            error_exit("closing socket failed");
                        }
                        poll_fds[j].fd = -1;
                    }
                    else{   // ACK received, mark it
                        nodeName src = (j == 0) ? RELAY1 : RELAY2;
                        printLog(CLIENT, RECEIVED, ACK, ackPkt.seq_num, src, CLIENT);
                        int pktNum = (ackPkt.seq_num - windowStartSeqNum) / CHUNK_SIZE;
                        ackRcvd[pktNum] = RECEIVED;
                    }
                }
            }
            
            someoneLeft = false;
            
            // check if is there someone whose ack not rcvd yet?
            for(int j = 0; j < WINDOW_SIZE; j++){
                if(ackRcvd[j] == NOTRECEIVED){
                    someoneLeft = true;
                    break;
                }
            }
        }
    }

    fclose(inpFilePtr);
    fclose(logFilePtr);
}

