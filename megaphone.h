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
#include <sys/select.h>
#include <sys/types.h>

#define DEFAULT_UDP_PORT 33333

// STRUCTURES
typedef struct {
  u_int8_t CODEREQ, DATALEN;
  uint16_t NUMFIL, NB, ID;
  char DATA[256]; // DATALEN codé sur 1 octet. Donc DATA -> taille 255 max + 1 (pour '\0')
  uint8_t multicast_addr[16];
} client_msg;

typedef struct {
  u_int8_t CODEREQ, ID;
  uint16_t NUMFIL;
  char PSEUDO[11];
  char DATA[21];
} notification;

typedef struct {
  int ID;
  char pseudo[11];
  char text[256];
} message;

typedef struct {
  uint16_t codreq_id;
  uint16_t num_bloc;
  char data[512];
} FilePacket;

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

// Send/Download file
void insert_packet_sorted(Node** head, FilePacket packet);
int write_packets_to_file(Node* head, char* file_path);
void free_list(Node* head);

int send_file(struct sockaddr_in6 servadr, client_msg msg, char* file_path);
Node* download_file(int UDP_port, int ID, int CODREQ);

#endif