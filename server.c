#include "megaphone.h"

#define SIZE_MESS 100
#define DATA_LEN 255
#define DATABASE "users.data"

void affiche_connexion(struct sockaddr_in6 adrclient){
  char adr_buf[INET6_ADDRSTRLEN];
  memset(adr_buf, 0, sizeof(adr_buf));
  
  inet_ntop(AF_INET6, &(adrclient.sin6_addr), adr_buf, sizeof(adr_buf));
  printf("adresse client : IP: %s port: %d\n", adr_buf, ntohs(adrclient.sin6_port));
}

int send_error(int sockclient) {
  if(sockclient < 0)
    return 1;
  // On cree une entete avec CODEREQ=31
  // send()...
  uint16_t a = htons(31);

  if(send(sockclient, &a, sizeof(uint16_t), 0) < 0)
    exit(-1);

  char buffer[4];
  memset(buffer, 0, 4);

  if(send(sockclient, buffer, sizeof(buffer), 0) < 0)
    exit(-1);

  return 0;
}

void goto_last_line(int fd) {
  if(fd < 0)
    return;
  lseek(fd, 0, SEEK_END);
  lseek(fd, -1, SEEK_CUR);
  char c;
  // on lit le fichier jusqu'au \n ou sinon jusqu'au debut du fichier
  while (lseek(fd, 0, SEEK_CUR) > 0) {
      read(fd, &c, sizeof(char));
      if (c == '\n')
          break;
      lseek(fd, -2, SEEK_CUR);
  }
}

void clear_pseudo(char * pseudo){
    char* pos_hstg = strchr(pseudo, '#');
    if(pos_hstg == NULL)
      return;
    size_t length_cleared = pos_hstg - pseudo;
    pseudo[length_cleared] = '\0';
}

int recv_client_subscription(int sockclient, client_msg* cmsg) {
  char pseudo[11];
  memset(pseudo, 0, sizeof(pseudo));

  int n = recv(sockclient, pseudo, 10, 0);
  if (n < 0) {send_error(sockclient);}
  pseudo[n] = '\0';

  char buffer[128];
  memset(buffer, 0, sizeof(buffer));

  int id = 0;
  int fd = open(DATABASE, O_RDWR | O_CREAT, 0666);

  if(fd < 0)
    return send_error(sockclient);

  goto_last_line(fd);

  read(fd, buffer, sizeof(buffer));

  if(strlen(buffer) != 0)
    id = atoi(buffer);
  id++;

  goto_last_line(fd);

  clear_pseudo(pseudo);
  sprintf(buffer, "%d %s\n", id, pseudo);
  write(fd, buffer, strlen(buffer));

  sprintf(buffer, "%d", id);
  write(fd, buffer, strlen(buffer));

  close(fd);

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
  // Ici on va convertir en ordre du host et prendre les valeurs de codereq et id. Pour ID on a besoin de
  // les caler à nouveau 5 bits vers la droite, voir dessin pour mieux comprendre. (board.excalidraw)
  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  cmsg->ID = (res & 0xFFE0) >> 5;
  // + vérifier que ID dépasse pas 11bits ?
  printf("CODEREQ: %d ID: %d\n", cmsg->CODEREQ, cmsg->ID);
  if(cmsg->CODEREQ > 6) {
    return send_error(sockclient);
  }
  else if(cmsg->CODEREQ == 1) { // INSCRIPTION
    return recv_client_subscription(sockclient, cmsg);
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
    return send_error(sockclient);
  }
  cmsg->NB = ntohs(res);

  // Vérifier valeur NUMFIL?

  recu = recv(sockclient, &res, sizeof(uint8_t), 0);
  if (recu <= 0){
    return send_error(sockclient);
  }
  cmsg->DATALEN = ntohs(res);

  // if... DATALEN...

  cmsg->DATA = malloc(sizeof(char) * cmsg->DATALEN);
  if(cmsg->DATA == NULL) {
    perror("malloc");
    return 1;
  }
  memset(cmsg->DATA, 0, cmsg->DATALEN);

  recu = recv(sockclient, cmsg->DATA, sizeof(char) * DATA_LEN, 0);
  if (recu != cmsg->DATALEN) {
    return send_error(sockclient);
  }

  return 0;
}

int main(int argc, char** args) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <port>\n", args[0]);
    exit(1);
  }
  
  //*** creation de la socket serveur ***
  int sock = socket(PF_INET6, SOCK_STREAM, 0);
  if(sock < 0){
    perror("creation socket");
    exit(1);
  }

  //*** creation de l'adresse du destinataire (serveur) ***
  struct sockaddr_in6 address_sock;
  memset(&address_sock, 0, sizeof(address_sock));
  address_sock.sin6_family = AF_INET6;
  address_sock.sin6_port = htons(atoi(args[1]));
  address_sock.sin6_addr = in6addr_any;
  
  int yes = 1;
  int r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  //*** on lie la socket au port PORT ***
  r = bind(sock, (struct sockaddr *) &address_sock, sizeof(address_sock));
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
  struct sockaddr_in6 adrclient;
  memset(&adrclient, 0, sizeof(adrclient));
  socklen_t size = sizeof(adrclient);
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
