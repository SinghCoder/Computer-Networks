#include "server.h"

bool toDiscard;

void discardRandom(){
    int n = rand() % 100;
    // printf("random val is : %d", n);
    if( n < PDR){
        toDiscard = true;
    }
    else{
        toDiscard = false;
    }
    // if(toDiscard)
    // printf("And I'm discarding\n");
}

void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void print_trace(action act, int seq_num, int size, channel_id ch_id, char *data){
    char *actionStr;
    if(act == SENT){
        actionStr = "SENT";
    }
    else{
        actionStr = "RECEIVED";
    }
    printf("Server | %s | SeqNum : %d | Size : %d | Channel : %d\n", actionStr, seq_num, size, ch_id);
    // printf("Content : %s\n", data);
}

int main ()
{
    /*CREATE A TCP SOCKET*/
    int listenfd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0){ 
        error_exit("listening socket server error");
    }

    printf ("Server Socket Created\n");
    
    /*CONSTRUCT LOCAL ADDRESS STRUCTURE*/
    struct sockaddr_in serverAddress, clientAddress;
    
    memset (&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    
    printf ("Server address assigned\n");
    
    int temp = bind(listenfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    
    if (temp < 0){
        error_exit("bind error");
    }
    printf ("Binding successful\n");

    int temp1 = listen(listenfd, MAXPENDING);
    
    if (temp1 < 0){
        error_exit("listen error");
    }
    printf ("Now Listening\n");
    
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

            printf ("Handling Client %s inside process %d\n", inet_ntoa(clientAddress.sin_addr), getpid());
            
            FILE *destFilePtr = fopen("destination_file.txt", "wb");

            for(; ; ){        

                int n =  recv(connfd, &dataPkt, sizeof(dataPkt) , 0);

                if ( n < 0){ 
                    error_exit("recv dataPkt error");
                }

                if(!n){
                    printf("client closed the connection\n");
                    exit(EXIT_SUCCESS);
                }
                
                printf("received from %d\n", getpid());
                print_trace(RECEIVED, dataPkt.seq_num, dataPkt.data_size, dataPkt.channel_id, dataPkt.data);
                
                // if(dataPkt.channel_id == CHANNEL_ONE){
                //     printf("Igbnoring it\n");
                //     continue;
                // }
                // discardRandom();

                // if(toDiscard != true){
                    
                
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

                    if(dataPkt.is_last)
                        break;
                // }                
            }
            
            fclose(destFilePtr);

            if( close(connfd) < 0){
                error_exit("close connfd error");
            }

            printf("Received completely from %d\n", getpid());
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