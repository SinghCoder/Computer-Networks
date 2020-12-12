#include "server.h"

bool toDiscard;

/**
 * @brief sets discard variable to true or false based on a random number value
 * 
 */
void discardRandom(){
    int n = rand() % 100;
    if( n < PDR){
        toDiscard = true;
    }
    else{
        toDiscard = false;
    }
}

void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
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
        actionStr = "SENT";
    }
    else{
        actionStr = "RCVD";
    }
    printf("%s | Seq. No : %d | Size : %d bytes | Channel : %d\n", actionStr, seq_num, size, ch_id);    
}

/**
 * @brief Create a linked list of packets to maintain a queue
 * 
 * @return created list
 */
packet_list *createPktList(){
    packet_list *list = (packet_list *) malloc(sizeof(packet_list));
    list->head = NULL;
    list->numPkts = 0;
    return list;
}

/**
 * @brief Insert packet in sorted order of sequence numbers in the list
 * 
 * @param list 
 * @param pkt 
 * @return packet_list* modified list
 */
packet_list *insertPktInSortedOrder(packet_list *list, packet_t pkt){
    if(list && list->numPkts >= BUFFERSIZE){
        return list;
    }

    list_node *node = (list_node *) malloc(sizeof(list_node));
    
    node->pkt.category = pkt.category;
    strcpy(node->pkt.data, pkt.data);
    node->pkt.data_size = pkt.data_size;
    node->pkt.is_last = pkt.is_last;
    node->pkt.seq_num = pkt.seq_num;
    node->next = NULL;
    
    // traverse and find the place where to insert
    list_node *temp = list->head, *prev = list->head;

    if(list->numPkts == 0){ // first packet, insert at head
        list->head = node;
        list->numPkts = 1;
        return list;
    }

    list->numPkts++;
    // least sequence number packet, insert at head again
    if(node->pkt.seq_num < temp->pkt.seq_num){
        node->next = temp;
        list->head = node;
        return list;
    }

    // find the position and insert there
    while(temp != NULL){
        if(node->pkt.seq_num == temp->pkt.seq_num){
            return list;
        }
        if(node->pkt.seq_num > prev->pkt.seq_num && node->pkt.seq_num < temp->pkt.seq_num){
            node->next = temp;
            prev->next = node;
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    if(temp == NULL){
        prev->next = node;
    }
    return list;
}

/**
 * @brief Get the Next data matching the sequence number or return NULL
 * 
 * @param list buffered packets list
 * @param seq_num what u are looking for
 * @return char* matching payload or NULL if no match
 */
char *getNextData(packet_list *list, int seq_num){   

    if(list == NULL || list->numPkts == 0 ){
        return NULL;
    }
    char *data = NULL;
    list_node *temp = list->head, *prev = list->head;
    
    if(temp->pkt.seq_num == seq_num){
        data = (char*)malloc(sizeof(char) * (CHUNK_SIZE + 1));
        strcpy(data ,temp->pkt.data );
        list->head = temp->next;
        list->numPkts--;
        return data;
    }

    while(temp != NULL){
        if(temp->pkt.seq_num == seq_num){
            prev->next = temp->next;
            data = (char*)malloc(sizeof(char) * (CHUNK_SIZE + 1));
            strcpy(data ,temp->pkt.data );
            free(temp);
            list->numPkts--;
            return data;
        }
        prev = temp;
        temp = temp->next;
    }
    return data;
}

int main ()
{
    logFilePtr = fopen(SERVER_LOG_FILE, "w");
    // send sometimes does not return when channel is closed, so ignoring sig_pipe, so that it returns -1 instead
    signal(SIGPIPE, SIG_IGN);

    /*CREATE A TCP SOCKET*/
    int listenfd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0){ 
        error_exit("listening socket server error");
    }
    
    // for allowing binding again
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error_exit("setsockopt(SO_REUSEADDR) failed");

    /*CONSTRUCT LOCAL ADDRESS STRUCTURE*/
    struct sockaddr_in serverAddress, clientAddress;
    
    memset (&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int temp = bind(listenfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    
    if (temp < 0){
        error_exit("bind error");
    }

    int temp1 = listen(listenfd, MAXPENDING);
    
    if (temp1 < 0){
        error_exit("listen error");
    }
    
    int numClients = 2; // 2 channels to monitor
    struct pollfd poll_fds[numClients + 1]; // 1 for listen_sockfd
    int clientLength = sizeof(clientAddress);
    pid_t pid;

    // Initialize all to pollin event
    for(int i = 0; i < numClients; i++){
        poll_fds[i].fd = -1;
        poll_fds[i].events = POLLIN;
        poll_fds[i].revents = 0;
    }

    // mark listening fd for pollin event to enable accepting connections
    poll_fds[numClients].fd = listenfd;
    poll_fds[numClients].events = POLLIN;
    poll_fds[numClients].revents = 0;

    int pollStatus;
    int numReady = 0;
    int connfd;

    packet_t dataPkt, ackPkt;
    packet_list *pktList = createPktList(); // buffered packets list

    int expectedOffset = 0; // what offset is expected
    int numPending = 0;     // how many out of order packets are pending
    int lastChunkOffset = -1;   // offset of last chunk
                
    FILE *outputFile = fopen(DESTINATION_FILE, "wb");

    for( ; ; ){
        
        // poll all 
        pollStatus = poll(poll_fds, numClients + 1, -1);

        if(pollStatus == -1){
            error_exit("polling failed");
        }

        numReady = pollStatus;
        int cliLen = sizeof(clientAddress);
        if(poll_fds[numClients].revents & POLLIN){  // listening socket is readable, accept a conenction
            connfd = accept(listenfd, (struct sockaddr *)&clientAddress, &cliLen);
            if(connfd < 0){
                error_exit("accepting clinet connection failed");
            }

            for(int i = 0; i < numClients; i++){
                if(poll_fds[i].fd == connfd)
                    break;
                if(poll_fds[i].fd == -1){
                    poll_fds[i].fd = connfd;
                    break;
                }
            }
            poll_fds[numClients].revents = 0;
        }

        // check for ACK on both channels
        for(int i = 0; i < numClients; i++){
            if(dataPkt.is_last){
                lastChunkOffset = dataPkt.seq_num;
            }
            bool toSendACK = true;
            bool allDone = false;

            if( (poll_fds[i].fd != -1) && (poll_fds[i].revents & POLLIN)){
                /**
                 * @brief The client exists at this fd, and the fd is readable => ACK has arrived
                 */
                poll_fds[i].revents = 0;
                int n;
                if ( (n = recv(poll_fds[i].fd, &dataPkt, sizeof(dataPkt) , 0) ) < 0){ 
                    error_exit("recv dataPkt error");
                }
                if(n > 0){
            
                    discardRandom();    // discard packet randomly
                    
                    if(outputFile == NULL){
                        outputFile = fopen(DESTINATION_FILE, "wb");
                    }

                    if(toDiscard != true){
                        print_trace(RCVD, dataPkt.seq_num, dataPkt.data_size, dataPkt.channel_id);
                        printLog(SERVER, RCVD, DATA, dataPkt.seq_num, CLIENT, SERVER);

                        // packet came is what I expected
                        if(dataPkt.seq_num == expectedOffset){
                            fwrite(dataPkt.data, 1, dataPkt.data_size, outputFile);
                            fflush(outputFile);
                            expectedOffset += dataPkt.data_size;
                            
                            
                            if(dataPkt.seq_num == lastChunkOffset){
                                allDone = true;
                            }

                            //write pending packets if any which are contiguous
                            char *data = NULL;
                            
                            while( (data = getNextData(pktList, expectedOffset)) != NULL ){
                                if(expectedOffset == lastChunkOffset){
                                    allDone = true;
                                }
                                fwrite(data, 1, strlen(data), outputFile);
                                fflush(outputFile);
                                expectedOffset += strlen(data);                
                            }
                            if(dataPkt.is_last){
                                fclose(outputFile);
                                outputFile = NULL;                
                            }            
                        }
                        else{   // out of order packet has arrived
                            if(numPending < BUFFERSIZE){
                                // insert if buffer free
                                pktList = insertPktInSortedOrder(pktList, dataPkt);
                                numPending++;
                            }
                            else{
                                // Reject the packet
                                toSendACK = false;
                            }
                        }
                        
                        if(toSendACK){

                            ackPkt.category = ACK;
                            ackPkt.channel_id = dataPkt.channel_id;
                            ackPkt.data_size = 0;
                            ackPkt.is_last = dataPkt.is_last;
                            ackPkt.seq_num = dataPkt.seq_num;
                            strcpy(ackPkt.data, "");

                            int bytesSent = send(poll_fds[i].fd, &ackPkt, sizeof(ackPkt) , 0);
                            if (bytesSent != sizeof(ackPkt)){
                                error_exit("send ack error");
                            }
                            else{
                                print_trace(SENT, ackPkt.seq_num, ackPkt.data_size, ackPkt.channel_id);
                                printLog(SERVER, SENT, ACK, ackPkt.seq_num, SERVER, CLIENT);
                            }
                        }
                    }       
                    else{
                        // discarding packet
                        printLog(SERVER, REJECT, DATA, dataPkt.seq_num, CLIENT, SERVER);
                    } 
                }   // end of if
            }
        }   // End of loop checking readability 
    }   // Main for loop for polling



    
    if( close(listenfd)  < 0){
        error_exit("close listenfd error");
    }

    fclose(logFilePtr);
}