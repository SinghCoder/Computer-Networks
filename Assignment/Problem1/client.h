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
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "packet.h"

#define INPUT_FILE "input.txt"
#define CHUNK_SIZE 100
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"
#define TIMEOUT_S 2
#define MAX_TRIES 10
#define CLIENT_LOG_FILE "client.log"

union semun {   // for semaphore
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
} semArgs;

FILE *logFilePtr;