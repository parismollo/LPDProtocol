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
#include <sys/wait.h>
#include <net/if.h>
#include <pthread.h>



// STRUCTURES
typedef struct {
  u_int8_t CODEREQ, ID, DATALEN;
  uint16_t NUMFIL, NB;
  char* DATA; // Il faut remplacer par char DATA[256] !
  uint8_t multicast_addr[16];
} client_msg;

typedef struct {
  int ID;
  char pseudo[11];
  char text[256];
}message;

typedef struct {
  uint16_t codreq_id;
  uint16_t num_bloc;
  char data[513]; // 512 + 1 (pour '\0')
}FilePacket;

typedef struct Node {
  FilePacket packet;
  struct Node* next;
} Node;

// FUNCTIONS
void clear_pseudo(char * pseudo);
int get_pseudo(int id, char* pseudo, size_t pseudo_size);
int get_fil_initiator(int fil, char* initiator, size_t buf_size);
int get_infos(char* key, char* value, size_t val_size);
int increase_nb_fils();
int nb_fils();
int change_infos(char* key, char* new_value);
int query(int sock, client_msg* msg);
void replace_after(char* str, char target, char replace_char, size_t maxsize);
int prefix(char* str, char* pre);

#endif