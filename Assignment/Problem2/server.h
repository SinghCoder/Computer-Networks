#include "pktInfo.h"
#include "common.h"
#ifndef SERVER_H
#define SERVER_H

#define BUFFERSIZE 15   // number of packets to buffer
#define SERVER_LOG_FILE "server.log"
#define DESTINATION_FILE "destination_file.txt"

typedef struct packet_list packet_list; // sorted linked list of buffered packets
typedef struct packet_list_node list_node;  // node of list

struct packet_list_node{
    packet_t pkt;
    list_node *next;
};

struct packet_list{
    list_node *head;
    int numPkts;
};
FILE *logFilePtr;
#endif