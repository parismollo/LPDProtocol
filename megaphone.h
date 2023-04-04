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
#include <sys/stat.h>
#include <errno.h>


// STRUCTURES
typedef struct {
  u_int8_t CODEREQ, ID, DATALEN;
  uint16_t NUMFIL, NB;
  char* DATA;
} client_msg;

typedef struct {
  int ID;
  char pseudo[11];
  char text[256];
}message;

// FUNCTIONS
int get_pseudo(int id, char* pseudo);

#endif