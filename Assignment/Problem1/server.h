#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>  
#include <time.h>   // strftime() in log printing
#include <sys/time.h>
#include "packet.h"

#define MAXPENDING 5    // listen queue limit
#define BUFFERSIZE 32   // numebr of packets to buffer
#define SERVER_PORT 8888
#define PDR 10     // drop rate
#define TIMEOUT_MS 3000     // timeout for poll in ms
#define SERVER_LOG_FILE "server.log"    // log file for server
#define DESTINATION_FILE "destination_file.txt"
FILE *logFilePtr;