#include "server.h"


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

/**
 * @brief Create a list to buffer packets
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
 * @brief insert packets in sequence order
 * 
 * @param list 
 * @param pkt 
 * @return packet_list* 
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

    if(list->numPkts == 0){
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

int main(int argc, char *argv[])
{
    logFilePtr = fopen(SERVER_LOG_FILE, "w");

    struct sockaddr_in serverAddr, clientAddr;

    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&serverAddr, sizeof(struct sockaddr_in));
    bzero(&clientAddr, sizeof(struct sockaddr_in));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if( bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        error_exit("binding failed");
    }
    
    packet_t dataPkt, ackPkt;
    socklen_t clientLen = sizeof(clientAddr);
    packet_list *pktList = createPktList(); // bufferinf list
    
    FILE *outputFile = fopen(DESTINATION_FILE, "wb");
    int expectedOffset = 0; // what offset I am expecting
    int numPending = 0;     // number of pending packets in queue
    int lastChunkOffset = -1;

    for( ; ; ){

        // recv from relay
        if(recvfrom(sockfd, &dataPkt, sizeof(dataPkt), 0, (struct sockaddr *) &clientAddr, &clientLen) < 0){
            error_exit("recvfrom failed");
        }

        nodeName relayName = ( (dataPkt.seq_num / CHUNK_SIZE) %2 == 0) ? RELAY1 : RELAY2;
        printLog(SERVER, RCVD, DATA, dataPkt.seq_num, relayName, SERVER);

        if(outputFile == NULL){
            outputFile = fopen(DESTINATION_FILE, "wb");
        }
        if(dataPkt.is_last){
            lastChunkOffset = dataPkt.seq_num;
        }

        bool toSendACK = true;  // send ack if not rejecting it due to full buffer
        bool allDone = false;   // finished all packets

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
        else{
            // out of order packet received, buffer if space available, else reject it
            if(numPending < BUFFERSIZE){
                pktList = insertPktInSortedOrder(pktList, dataPkt);
                numPending++;
            }
            else{
                toSendACK = false;
                printLog(SERVER, REJECTED, DATA, dataPkt.seq_num, relayName, SERVER);
            }
        }
        
        if(toSendACK){
            ackPkt.category = ACK;
            strcpy(ackPkt.data, "");
            ackPkt.data_size = 0;
            ackPkt.is_last = dataPkt.is_last;
            ackPkt.seq_num = dataPkt.seq_num;

            clientLen = sizeof(clientAddr);
            if( sendto(sockfd, &ackPkt, sizeof(ackPkt), 0, (struct sockaddr *) &clientAddr, clientLen) < 0 ){
                error_exit("sendto");
            }
            printLog(SERVER, SENT, ACK, ackPkt.seq_num, SERVER, relayName);
        }
        if(allDone)
            exit(EXIT_SUCCESS);
    }  

    fclose(logFilePtr);
    return 0;
}