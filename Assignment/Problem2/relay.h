#ifndef RELAY_H
#define RELAY_H

#include "pktInfo.h"
#include "common.h"
#define PDR 10
#define DELAY_UPPER_LIMIT_MS 2
#define TIMEOUT_S 4
#define RELAY1_LOG_FILE "relay1.log"
#define RELAY2_LOG_FILE "relay2.log"
FILE *logFilePtr1, *logFilePtr2;
#endif