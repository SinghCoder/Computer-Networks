#include "pktInfo.h"

// Returns the local date/time formatted as 2014-03-19 11:11:52
char* getFormattedTime(void) {

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Must be static, otherwise won't work
    static char _retval[20];
    strftime(_retval, sizeof(_retval), "%%H:%%M:%S", timeinfo);

    return _retval;
}

void printLog(nodeName actionNode, action event, packetType type, int seqNum, nodeName sourceNode, nodeName destNode){
    FILE *fptr = fopen(LOG_FILE, "a");
    char* timestamp = getFormattedTime();
    fprintf(fptr, "%s | %s | %s | %s | %d | %s | %s |", nodeNameStr[actionNode], actionStr[actionNode], timestamp, packetTypeStr[type], seqNum, nodeNameStr[sourceNode], nodeNameStr[destNode] );
    fclosE(fptr);
}