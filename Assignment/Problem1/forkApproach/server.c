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
    
    char msg[BUFFERSIZE];
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
                
                    fseek(destFilePtr, dataPkt.seq_num, SEEK_SET);
                    fwrite(dataPkt.data, 1, dataPkt.data_size, destFilePtr);

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