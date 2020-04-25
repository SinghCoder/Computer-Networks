#include "client.h"

void error_exit(char *s){
    perror(s);
    exit(1);
}

int main(){
    int sockfdEven, sockfdOdd;
    struct sockaddr_in relayEvenAddr, relayOddAddr;
    packet_t dataPkts[WINDOW_SIZE], ackPkts[WINDOW_SIZE];

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

    bool ackRcvd[WINDOW_SIZE] = { false };
    bool someoneLeft = false;
    int numRead;

    struct pollfd poll_fds[2];

    poll_fds[0].fd = sockfdEven;
    poll_fds[1].fd = sockfdOdd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].events = POLLIN;

    for(int i = 0; i < (int) (ceil((numChunks + 0.0) / WINDOW_SIZE)) ; i++){
        
        for(int j = 0; j < WINDOW_SIZE; j++){
            numRead = fread(dataPkts[j].data, 1, sizeof(dataPkts[j]), inpFilePtr);
            if(numRead == 0 && ferror(inpFilePtr)){
                error_exit("reading chunk failed");
            }
            dataPkts[j].category = DATA;
            dataPkts[j].data_size = numRead;
            dataPkts[j].is_last = false;
            dataPkts[j].seq_num = fseek(inpFilePtr, 0, SEEK_CUR);

            if( (dataPkts[j].seq_num % CHUNK_SIZE == 0) ){
                if(sendto(sockfdEven, &(dataPkts[j]), sizeof(dataPkts[j]), 0, (struct sockaddr *)&relayEvenAddr, sizeof(relayEvenAddr)) < 0){
                    error_exit("sendto even relay failed");
                }
            }
            else{
                if(sendto(sockfdOdd, &(dataPkts[j]), sizeof(dataPkts[j]), 0, (struct sockaddr *)&relayOddAddr, sizeof(relayOddAddr)) < 0){
                    error_exit("sendto odd relay failed");
                }
            }
        }

        while(true){    //poll until all ACKs for window are received
            int poll_status = poll(poll_fds, 2, TIMEOUT_MS);

            if(poll_status == -1){
                error_exit("poll failed");
            }

            if(poll_status == 0){
                printf("Timeout occured\n");
                continue;
                // return 0;
            }

            for(int j = 0; j < 2; j++){
                if( (poll_fds[j].fd != -1) && (poll_fds[j].revents & POLLIN)){
                    printf("poll_fd %d is readable, recieving data from it\n", poll_fds[j].fd);
                    int n = read(poll_fds[j].fd, &ackPkts[j], sizeof(ackPkts[j]));
                    if(n < 0){
                        error_exit("reading ACK failed");
                    }
                    printf(" Recieved ACK for seqnum %d\n", ackPkts[j].seq_num);
                    if(n == 0){ // server has closed the connection
                        close(poll_fds[j].fd);
                        poll_fds[j].fd = -1;
                    }
                    else{   // ACK received, mark it
                        // ackRcvd[j]
                    }
                }
            }
        }
    }

    fclose(inpFilePtr);
}

