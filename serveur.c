#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE_MESS 100
#define DATA_LEN 255

typedef struct {
  int CODEREQ, ID, NUMFIL, NB, DATALEN;
  char* DATA;
} client_msg;

void affiche_connexion(struct sockaddr_in adrclient){
  char adr_buf[INET_ADDRSTRLEN];
  memset(adr_buf, 0, sizeof(adr_buf));
  
  inet_ntop(AF_INET, &(adrclient.sin_addr), adr_buf, sizeof(adr_buf));
  printf("adresse client : IP: %s port: %d\n", adr_buf, ntohs(adrclient.sin_port));
}

int send_error(int sockclient) {
  if(sockclient < 0)
    return 1;
  // On cree une entete avec CODEREQ=31
  // send()...
  return 0;
}

int recv_client_msg(int sockclient, client_msg* cmsg) {
  if (sockclient < 0)
      return 1;
    //*** reception d'un message ***
    uint16_t res;
    int recu = recv(sockclient, &res, sizeof(uint16_t), 0);
    if (recu <= 0){
      perror("erreur lecture");
      return(1);
    }
    res = ntohs(res);
    cmsg->CODEREQ = (res & 0xF800) >> 11; // On mask avec 111110000..
    cmsg->ID = (res & 0x07FF);
    
    // + vérifier que ID dépasse pas 11bits ?
    if(cmsg->CODEREQ > 6) {
      send_error(sockclient); 
      return 1;
    }

    recu = recv(sockclient, &res, sizeof(uint16_t), 0);
    if (recu <= 0){
      perror("erreur lecture");
      return 1;
    }
    cmsg->NUMFIL = ntohs(res);

    // Vérifier valeur NUMFIL?

    recu = recv(sockclient, &res, sizeof(uint16_t), 0);
    if (recu <= 0){
      perror("erreur lecture");
      return 1;
    }
    cmsg->NB = ntohs(res);

    // Vérifier valeur NUMFIL?

    recu = recv(sockclient, &res, sizeof(uint8_t), 0);
    if (recu <= 0){
      perror("erreur lecture");
      return 1;
    }
    cmsg->DATALEN = htons(res);

    // if... DATALEN...

    cmsg->DATA = malloc(sizeof(char) * cmsg->DATALEN);
    if(cmsg->DATA == NULL) {
      perror("malloc");
      return 1;
    }
    memset(cmsg->DATA, 0, cmsg->DATALEN);

    recu = recv(sockclient, cmsg->DATA, sizeof(char) * DATA_LEN, 0);
    if (recu != cmsg->DATALEN) {
      send_error(sockclient);
      perror("erreur lecture DATA");
      return 1;
    }

  return 0;
}

int main(int argc, char** args) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", args[0]);
        exit(1);
    }
    
  //*** creation de la socket serveur ***
  int sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock < 0){
    perror("creation socket");
    exit(1);
  }

  //*** creation de l'adresse du destinataire (serveur) ***
  struct sockaddr_in address_sock;
  memset(&address_sock, 0, sizeof(address_sock));
  address_sock.sin_family = AF_INET;
  address_sock.sin_port = htons(atoi(args[1]));
  address_sock.sin_addr.s_addr=htonl(INADDR_ANY);
  
  //*** on lie la socket au port PORT ***
  int r = bind(sock, (struct sockaddr *) &address_sock, sizeof(address_sock));
  if (r < 0) {
    perror("erreur bind");
    exit(2);
  }

  //*** Le serveur est pret a ecouter les connexions sur le port PORT ***
  r = listen(sock, 0);
  if (r < 0) {
    perror("erreur listen");
    exit(2);
  }

  //*** le serveur accepte une connexion et cree la socket de communication avec le client ***
  struct sockaddr_in adrclient;
  memset(&adrclient, 0, sizeof(adrclient));
  socklen_t size=sizeof(adrclient);
  int sockclient = accept(sock, (struct sockaddr *) &adrclient, &size);
  if(sockclient == -1){
    perror("probleme socket client");
    exit(1);
  }	   

  affiche_connexion(adrclient);
  
  client_msg msg;
  recv_client_msg(sockclient, &msg);

  //*** fermeture socket client ***
  close(sockclient);

  //*** fermeture socket serveur ***
  close(sock);
  
  return 0;
}