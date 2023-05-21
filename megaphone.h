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
#define NOTIFICATION_UDP_PORT 4321
#define DATABASE "server_users.data"
#define INFOS "infos.data"

/* STRUCTURES */

// Permet de stocker le message d'un client ou parfois du serveur
typedef struct {
  u_int8_t CODEREQ, DATALEN;
  uint16_t NUMFIL, NB, ID;
  // DATALEN codé sur 1 octet. Donc DATA -> taille 255 max + 1 (pour '\0')
  // Pär contre, quand on fait un send on envoie seulement -> 255 max
  char DATA[256];
  uint8_t multicast_addr[16];
} client_msg;

typedef struct {
  u_int8_t CODEREQ, ID;
  uint16_t NUMFIL;
  char PSEUDO[11];
  char DATA[21];
} notification;

// Utilisé pour stocker les infos pour un message à mettre dans un fil
typedef struct {
  int ID;
  char pseudo[11];
  char text[256];
} message;

// Utilisé pour échanger des fichiers client/serveur
typedef struct {
  uint16_t codreq_id;
  uint16_t num_bloc;
  char data[512];
} FilePacket;

// Utilisé pour trier les packets qu'on a reçu en UDP dans le bonne ordre
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
void replace_after(char* str, char target, char replace_char, size_t maxsize);
int prefix(char* str, char* pre);
void print_error(char * message);
int query_client(int sock, client_msg* msg);
int query_server(int sock, client_msg* msg);
void help_client();
char* get_file_name(char* path);
void goto_last_line(int fd);

// fil.c
int total_msg_fils(uint8_t nb_msg_by_fil);
int nb_msg_fil(int fil);
int get_fil_initiator(int fil, char* initiator, size_t buf_size);
int is_user_registered(int id);
int change_infos(char* key, char* new_value);
int get_last_messages(int nb, int fil, message* messages);
int get_pseudo(int id, char* pseudo, size_t pseudo_size);
int increase_nb_fils();
int nb_fils();
int get_infos(char* key, char* value, size_t val_size);
int readline(int fd, char* line, size_t buf_size);

// Send/Download file
void insert_packet_sorted(Node** head, FilePacket packet);
int write_packets_to_file(Node* head, char* file_path);
void free_list(Node* head);

int send_file(struct sockaddr_in6 servadr, client_msg msg, char* file_path);
Node* download_file(int UDP_port, int ID, int CODREQ);

#endif