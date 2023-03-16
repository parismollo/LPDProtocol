#ifndef MEGAPHONE_H
#define MEGAPHONE_H

// LIBRARIES
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

// STRUCTURES
typedef struct {
  u_int8_t CODEREQ, ID, DATALEN;
  uint16_t NUMFIL, NB;
  char* DATA;
} client_msg;

#endif