#ifndef CLIENT_H
#define CLIENT_H
#include "pktInfo.h"
#include "common.h"
#include <math.h>

#define INPUT_FILE "input.txt"
#define WINDOW_SIZE 4
#define TIMEOUT_MS 4000
#define CLIENT_LOG_FILE "client.log"

typedef enum {NOTRECEIVED, RECEIVED, NA} ackstatus_t;   // statuses of ACK for all packets in window

FILE *logFilePtr;
#endif