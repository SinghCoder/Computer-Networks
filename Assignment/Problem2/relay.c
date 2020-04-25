#include "relay.h"
void error_exit(char *s){
    perror(s);
    exit(1);
}
