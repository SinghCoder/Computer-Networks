#include "relay.h"
#include "common.h"
#include <math.h>

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
    FILE *fptr = (actionNode == RELAY1) ? logFilePtr1 : logFilePtr2;
    fprintf(fptr, "%s | %s | %s | %s | %d | %s | %s |\n", timestamp, nodeNameStr[actionNode], actionStr[event], packetTypeStr[type], seqNum, nodeNameStr[sourceNode], nodeNameStr[destNode] );
    fflush(fptr);
}

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

int main( int argc, char *argv[]){    
    if(argc < 2){
        printf("Usage: ./relay <relay_num>(1/2)");
        exit(EXIT_FAILURE);
    }

    int relayNum = atoi(argv[1]);   // take as input to determine which port to bind to
    int relayPort = (relayNum == 1) ? RELAY1_PORT : RELAY2_PORT;
    int srvrSockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(srvrSockfd < 0){
        error_exit("sending socket creation failed");
    }

    int cliSockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(cliSockfd < 0){
        error_exit("socket");
    }

    if(relayNum == 1){
        logFilePtr1 = fopen(RELAY1_LOG_FILE, "w");
    }
    else{
        logFilePtr2 = fopen(RELAY2_LOG_FILE, "w");
    }

    struct sockaddr_in serverAddr, relayAddr, clientAddr;
    memset(&relayAddr, '0', sizeof(relayAddr));
    memset(&clientAddr, '0', sizeof(clientAddr));
    memset(&serverAddr, '0', sizeof(serverAddr));

    // my address, where client will contact
    relayAddr.sin_family = AF_INET;
    relayAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    relayAddr.sin_port = htons(relayPort);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // port
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP); 

    if( bind(cliSockfd, (struct sockaddr*)&relayAddr,sizeof(relayAddr)) < 0) {
        error_exit("bind");
    }

    struct timeval timeout;      
    timeout.tv_sec = TIMEOUT_S;
    timeout.tv_usec = 0;
    nodeName relayName = (relayNum == 1) ? RELAY1 : RELAY2;

    // make receive non blocking, bcz server might not send ACK when it's buffer is full
    if (setsockopt (srvrSockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        error_exit("setsockopt failed for srvrSockfd\n");

    packet_t dataPkt, ackPkt;
    socklen_t clientLen, serverLen;
    clientLen = sizeof(clientAddr);
    serverLen = sizeof(serverAddr);
    float delay_s;
    struct timespec delayTime;

    while(true){
        // rcv pkt from client
        if(recvfrom(cliSockfd, &dataPkt, sizeof(dataPkt), 0, (struct sockaddr *) &clientAddr, &clientLen) < 0){
            error_exit("recv data from client failed");
        }
        printLog(relayName, RCVD, DATA, dataPkt.seq_num, CLIENT, relayName);
        // drop randomly

        discardRandom();

        if(toDiscard == true){
            printLog(relayName, DROP, DATA, dataPkt.seq_num, CLIENT, relayName);
        }
        else{
            
            // delay for a random time between [0-upper_limit] ms
            
            delay_s = ((float)rand()/(float)(RAND_MAX)) * DELAY_UPPER_LIMIT_MS;
            delayTime.tv_sec = (int)delay_s;
            delayTime.tv_nsec = (int)((delay_s - floor(delay_s)) * pow(10, 9));

            nanosleep(&delayTime, &delayTime);
            
            // snd to server
            
            if( sendto(srvrSockfd, &dataPkt, sizeof(dataPkt), 0, (struct sockaddr *) &serverAddr, serverLen) < 0 ){
                error_exit("send data to server failed");
            }
            
            printLog(relayName, SENT, DATA, dataPkt.seq_num, relayName, SERVER);
            
            // rcv ack
            
            if(recvfrom(srvrSockfd, &ackPkt, sizeof(ackPkt), 0, (struct sockaddr *) &serverAddr, &serverLen) < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    continue;   // don't send an ACK, since server didn't rcv the data pkt yet
                }
                error_exit("recv ack from server failed");
            }
            
            printLog(relayName, RCVD, ACK, ackPkt.seq_num, SERVER, relayName);
            
            // snd back to client
            
            clientLen = sizeof(clientAddr);
            
            if( sendto(cliSockfd, &ackPkt, sizeof(ackPkt), 0, (struct sockaddr *) &clientAddr, clientLen) < 0 ){
                error_exit("send ack to client failed");
            }
            printLog(relayName, SENT, ACK, ackPkt.seq_num, relayName, CLIENT);
        }
    }
    if(relayNum == 1)
        fclose(logFilePtr1);
    else
        fclose(logFilePtr2);
}