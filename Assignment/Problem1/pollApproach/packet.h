#ifndef PACKET_H
#define PACKET_H

#define CHUNK_SIZE 100  // file size

typedef enum { false, true } bool;
typedef enum { DATA, ACK } packet_category;
typedef enum { CHANNEL_ONE, CHANNEL_TWO } channel_id;


typedef enum { SENT, RCVD, DROP, TIMEOUT, RETRANSMIT, REJECT } action;
typedef enum { CLIENT, SERVER } nodeName;

char *actionStr[6] = {"SENT", "RCVD", "DROP", "TIMEOUT", "RETRANSMIT", "REJECTED"};
char *nodeNameStr[2] = {"CLIENT", "SERVER"};
char *packetTypeStr[2] = {"DATA", "ACK"};


typedef struct packet{
    char data[CHUNK_SIZE + 1];  // 1 for '\0'
    int data_size;
    int seq_num;
    bool is_last;
    packet_category category;
    int channel_id;
}packet_t;

typedef struct packet_list packet_list;
typedef struct packet_list_node list_node;  // list of packets

struct packet_list_node{
    packet_t pkt;
    list_node *next;
};

struct packet_list{
    list_node *head;
    int numPkts;
};


#endif
