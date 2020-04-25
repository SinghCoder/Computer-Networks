#include "server.h"

bool toDiscard;

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

packet_list *createPktList(){
    packet_list *list = (packet_list *) malloc(sizeof(packet_list));
    list->next = NULL;
    list->numPkts = 0;
    return list;
}

packet_list *insertPktInSortedOrder(packet_list *list, packet_t pkt){
    if(list->numPkts >= BUFFERSIZE){
        return list;
    }
    packet_list *node = (packet_list *) malloc(sizeof(packet_list));
    node->pkt = pkt;
    node->next = NULL;
    packet_list *temp = list, *prev = list;
    list->numPkts++;
    if(node->pkt.seq_num < temp->pkt.seq_num){
        node->next = temp;
        list = node;
        return list;
    }
    while(temp != NULL){
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

char *getNextData(packet_list **list, int seq_num){
    char *data = NULL;
    packet_list *temp = *list, *prev = *list;
    
    if((*list)->pkt.seq_num == seq_num){
        data = (*list)->pkt.data;
        *list = (*list)->next;
        (*list)->numPkts--;
        return data;
    }

    while(temp != NULL){
        if(temp->pkt.seq_num == seq_num){
            prev->next = temp->next;
            data = temp->pkt.data;
            free(temp);
            (*list)->numPkts--;
            return data;
        }
        prev = temp;
        temp = temp->next;
    }
    return data;
}

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

int main ()
{
    /*CREATE A TCP SOCKET*/
    int listenfd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0){ 
        error_exit("listening socket server error");
    }
    
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
    
    packet_list *bufferedPktList = createPktList();
    int numBufferedPkts = 0;
    int expectedOffset = 0;
    int clientLength = sizeof(clientAddress);
    pid_t pid;

    for( ; ; ){
        int connfd = accept (listenfd, (struct sockaddr*)&clientAddress, &clientLength);
        
        if (connfd < 0){
            error_exit("accept error");
        }

        if((pid = fork()) == 0){
            close(listenfd);

            packet_t dataPkt, ackPkt;
            
            FILE *destFilePtr = fopen("destination_file.txt", "wb");

            for(; ; ){
                                    
                if ( recv(connfd, &dataPkt, sizeof(dataPkt) , 0) < 0){ 
                    error_exit("recv dataPkt error");
                }

                discardRandom();

                if(toDiscard != true){
                    print_trace(RECEIVED, dataPkt.seq_num, dataPkt.data_size, dataPkt.channel_id);
                    printf("was expecting offset %d and rcvd %d\n", expectedOffset, dataPkt.seq_num);
                    if(dataPkt.seq_num == expectedOffset){
                        fwrite(dataPkt.data, 1, dataPkt.data_size, destFilePtr);
                        expectedOffset += dataPkt.data_size;
                        if(numBufferedPkts > 0){
                            int tempOffset = expectedOffset;
                            char *data;
                            while( ( data = getNextData(&bufferedPktList, tempOffset) ) != NULL){
                                fwrite(data, 1, strlen(data), destFilePtr);
                            }
                        }
                    }
                    else{
                        if(numBufferedPkts < BUFFERSIZE){ 
                            printf("Buffering the packet due to out of order packets\n");
                            bufferedPktList = insertPktInSortedOrder(bufferedPktList, dataPkt);
                            numBufferedPkts++;
                        }
                        else{   // drop the packets
                            printf("Buffer full, so droppoing\n");
                            continue;
                        }
                    }

                    ackPkt.category = ACK;
                    ackPkt.channel_id = dataPkt.channel_id;
                    ackPkt.data_size = 0;
                    ackPkt.is_last = dataPkt.is_last;
                    ackPkt.seq_num = dataPkt.seq_num;
                    strcpy(ackPkt.data, "");

                    int bytesSent = send (connfd, &ackPkt, sizeof(ackPkt) , 0);
                    
                    if (bytesSent != sizeof(ackPkt)){
                        error_exit("send ack error");
                    }
                    else{
                        print_trace(SENT, ackPkt.seq_num, ackPkt.data_size, ackPkt.channel_id);
                    }

                    if(dataPkt.is_last)
                        break;
                }     
                else{
                    printf("discarding %d seqNum pkt\n", dataPkt.seq_num);
                }           
            }
            
            fclose(destFilePtr);

            if( close(connfd) < 0){
                error_exit("close connfd error");
            }
            
            exit(EXIT_SUCCESS);
        }

        if( close(connfd) < 0){
            error_exit("close connfd error");
        }
    }
    if( close(listenfd)  < 0){
        error_exit("close listenfd error");
    }
}