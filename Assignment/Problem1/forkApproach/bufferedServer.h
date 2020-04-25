#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXPENDING 5
#define BUFFERSIZE 10
#define SERVER_PORT 8888
#define CHUNK_SIZE 9
#define PDR 30  

typedef enum { false, true } bool;
typedef enum { DATA, ACK } packet_category;
typedef enum { SENT, RECEIVED } action;
typedef enum { CHANNEL_ONE, CHANNEL_TWO } channel_id;

typedef struct packet{
    char data[CHUNK_SIZE + 1];
    int data_size;
    int seq_num;
    bool is_last;
    packet_category category;
    int channel_id;
}packet_t;

typedef struct packet_list packet_list;
struct packet_list{
    packet_t pkt;
    int numPkts;
    packet_list *next;
};