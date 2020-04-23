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

#define INPUT_FILE "input.txt"
#define CHUNK_SIZE 9
#define SERVER_PORT 8894
#define SERVER_IP "127.0.0.1"
#define TIMEOUT 2

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

void error_exit(char *s);

void print_trace(action act, int seq_num, int size, channel_id ch_id);