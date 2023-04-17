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
void clear_pseudo(char * pseudo);
int get_pseudo(int id, char* pseudo, size_t pseudo_size);
int get_fil_initiator(int fil, char* initiator, size_t buf_size);
int get_infos(char* key, char* value, size_t val_size);
int increase_nb_fils();
int nb_fils();
int change_infos(char* key, char* new_value);
int query(int sock, client_msg* msg);

#endif