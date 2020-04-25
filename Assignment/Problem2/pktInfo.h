#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <time.h> // time_t, tm, time, localtime, strftime

#define CHUNK_SIZE 100
#define LOG_FILE "logs.txt"
typedef enum { false, true } bool;
typedef enum { S, R, D, TO, RE } action;
typedef enum { CLIENT, SERVER, RELAY1, RELAY2 } nodeName;
typedef enum { DATA, ACK } packetType;

typedef struct packet{
    char data[CHUNK_SIZE + 1];
    int data_size;
    int seq_num;
    bool is_last;
    packetType category;
}packet_t;

char *actionStr[5] = {"SENT", "RCVD", "DROP", "TIMEOUT", "RETRANSMIT"};
char *nodeNameStr[4] = {"CLIENT", "SERVER", "RELAY1", "RELAY2"};
char *packetTypeStr[2] = {"DATA", "ACK"};

void printLog(nodeName, action, packetType, int, nodeName, nodeName);
