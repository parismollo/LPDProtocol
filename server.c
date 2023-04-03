#include "megaphone.h"

#define SIZE_MESS 100
#define DATA_LEN 255
#define DATABASE "server_users.data"

void affiche_connexion(struct sockaddr_in6 adrclient){
  char adr_buf[INET6_ADDRSTRLEN];
  memset(adr_buf, 0, sizeof(adr_buf));
  
  inet_ntop(AF_INET6, &(adrclient.sin6_addr), adr_buf, sizeof(adr_buf));
  printf("adresse client : IP: %s port: %d\n", adr_buf, ntohs(adrclient.sin6_port));
}

int send_error(int sockclient, char* msg) {
  if(sockclient < 0)
    return 1;
  fprintf(stderr, "%s\n", msg);
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

int query(int sock, client_msg* msg) {

    msg->ID &= 0x07FF; // On garde que les 11 premiers bits 
    msg->CODEREQ &= 0x001F; // On garde que les 5 premiers bits

    // Combine le codereq (5 bits de poids faible) avec l'ID (11 bits restants)
    uint16_t res = ((uint16_t)msg->CODEREQ) | (msg->ID << 5);
    res = htons(res); 
    if (send(sock, &res, sizeof(res), 0) < 0) send_error(sock, "send failed"); 

    u_int16_t tmp = htons(msg->NUMFIL);
    if (send(sock, &tmp, sizeof(u_int16_t), 0) < 0) send_error(sock, "send failed"); 

    tmp = htons(msg->NB);
    if (send(sock, &tmp, sizeof(u_int16_t), 0) < 0) send_error(sock, "send failed");
  
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
  if (n < 0) {send_error(sockclient, "error recv");}
  pseudo[n] = '\0';

  char buffer[128];
  memset(buffer, 0, sizeof(buffer));

  int id = 0;
  int fd = open(DATABASE, O_RDWR | O_CREAT, 0666);

  if(fd < 0)
    return send_error(sockclient, "error open database file");

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

  // ON ENVOIE L ID A L UTILISATEUR
  client_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.ID = id;
  msg.CODEREQ = 1;
  query(sockclient, &msg);

  return 0;
}   

int handle_ticket(client_msg* msg) {
  // This function supposes that the msg has been validated
  char buf[100];
  sprintf(buf, "fil%d/fil%d.txt", msg->NUMFIL, msg->NUMFIL);
  printf("%s\n",buf);
  int fd, n;
  fd = open(buf, O_WRONLY | O_APPEND | O_CREAT, 0666);
  if (fd == -1) {
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "fil%d", msg->NUMFIL);
    
    if(mkdir(buf, 0777)==0) {
      return handle_ticket(msg);
    }
    else {
      perror("failed to create folder"); 
      return -1;
    }
  }
  else {
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "ID: %d\n", msg->ID);
    write(fd, buf, strlen(buf));

    printf("DATA: %s\n", msg->DATA);
    n = write(fd, msg->DATA, msg->DATALEN);
    // printf("n: %d\n", n);
    if (n < 1 || write(fd, "\n", 1) < 1) {
      perror("failed to write");
      return -1;
    } 
  }
  return 0;
}

// Enregistre le pseudo dans la variable pseudo
void get_pseudo(int id, char* pseudo) {
  FILE* f = fopen(DATABASE, "r");
  if(f == NULL)
    return;
    
  char buf[1024];
  char* ptr;
  while(fgets(buf, 1024, f) != NULL) {
    ptr = strtok(buf, " \n");
    if(ptr == NULL)
      break;
    if(atoi(ptr) == id) {
      ptr = strtok(NULL, " \n");
      if(ptr == NULL)
        break;
      memset(pseudo, 0, 11);
      int min = strlen(ptr);
      if(min > 10)
        min = 10;
      memmove(pseudo, ptr, min);
    }
  }
  fclose(f);
}

int nb_fils() {
  // 2 OPTIONS:
  // 1. On vérifie tous les dossiers qui commence par "fil"
  // 2. on maintient un fichier infos.txt qui contient plusieurs infos sur le
  // serveur dont par exemple le nombre de fils en cours
  // il suffit alors dans cette fonction de lire le chiffre dans le fichier
  return 0;
}

char* get_fil_initiator(int fil) {
  // Open fil
  // get first pseudo ?
  return NULL;
}

// Lit une ligne dans prendre le '\n'
int readline(int fd, char* line, size_t buf_size) {
  char c;
  memset(line, 0, buf_size);
  for(int i=0;i<buf_size;i++) {
    if(read(fd, &c, 1) < 0)
      return 1;
    if(c == '\n')
      return 0;
    line[i] = c;
  }
  return 2; // Ligne trop grande
}

int get_last_messages(int nb, int fil, message* messages) {
  // Open fil
  // get nb last msg
  // store them (char** array)

  char buf[100];
  sprintf(buf, "fil%d/fil%d.txt", fil, fil);
  printf("%s\n",buf);

  char line[1024];

  int fd = open(buf, O_RDONLY, 0666);
  
  long file_size = lseek(fd, 0, SEEK_END);
    
  long num_lines = 0, current_pos;
  for (current_pos = file_size - 1; current_pos >= 0; current_pos--) {
    lseek(fd, current_pos, SEEK_SET);
    read(fd, buf, 1);
    if(*buf == '\n')
      num_lines++;
    if(num_lines >= nb * 2)
      break;
  }

  if(num_lines < nb * 2) {
    fprintf(stderr, "il y a moins de %d msg\n", nb);
    return -1;
  }

  for(int i=0;i<nb;i++) {
    memset(&messages[i], 0, sizeof(message));
    // On lit l'ID
    readline(fd, line, 1024); // GESTION ERREURS
    // Check if begins with "ID: "
    messages[i].ID = atoi(line + 4); // strlen("ID: ") = 4
    // On lit le message
    readline(fd, line, 1024); // GESTION ERREURS
    // Check if begins with "DATA: "
    memmove(messages[i].text, line + 6, 255); // strlen("DATA: ") = 6
  }

  return 0;
}

int send_msg_ticket(int sockclient, int numfil, char* origine, message* msg) {
  // ...
  return 0;
}

int list_tickets(int sockclient, client_msg* msg) {
  // This function supposes that the msg has been validated
  if(msg->NB <= 0) {
    fprintf(stderr, "Bad Number");
    return -1;
  }

  /*
    1. Envoyer un premier billet selon les conditions du pdf
  */

  /*
    2. Envoyer les N derniers messages du fil
  */

  char buf[100];
  sprintf(buf, "fil%d/fil%d.txt", msg->NUMFIL, msg->NUMFIL);
  printf("%s\n",buf);

  char* initator = get_fil_initiator(msg->NUMFIL);
  if(initator == NULL) {
    // LE FIL N EXISTE PAS
    // send error...
    return -1;
  }

  message* messages = malloc(sizeof(message) * msg->NB);
  if(messages == NULL) {
    //send error
    return -1;
  }

  if(get_last_messages(msg->NB, msg->NUMFIL, messages) != 0) {
    // send error
    free(messages);
    return -1;
  }



  free(messages);
  return 0;
}

int validate_and_exec_msg(client_msg* msg) {
  // This function validate and call the appropriate function according to the msg
  // check if message has a valid structure, check if id exists or not, etc
  // if codereq == ? then ...
  // etc..
  return handle_ticket(msg); //tmp
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
    return send_error(sockclient, "CODEREQ too large");
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
    return send_error(sockclient, "error recv");
  }
  cmsg->NB = ntohs(res);

  // Vérifier valeur NUMFIL?

  recu = recv(sockclient, &res, sizeof(uint8_t), 0);
  if (recu <= 0){
    return send_error(sockclient, "error recv");
  }
  cmsg->DATALEN = res;

  // if... DATALEN...

  cmsg->DATA = malloc(sizeof(char) * cmsg->DATALEN);
  if(cmsg->DATA == NULL) {
    perror("malloc");
    return 1;
  }
  memset(cmsg->DATA, 0, cmsg->DATALEN);

  recu = recv(sockclient, cmsg->DATA, sizeof(char) * DATA_LEN, 0);
  // printf("recu: %d", recu);
  // printf("datalen: %d", cmsg->DATALEN);
  if (recu != cmsg->DATALEN) {
    return send_error(sockclient, "error DATALEN");
  }

  validate_and_exec_msg(cmsg);
  return 0;
}

int main(int argc, char** args) {
  // int ids[] = {133322, 12, 999};
  // for(int i=0;i<3;i++) {
  //   char pseudo[11];
  //   get_pseudo(ids[i], pseudo);
  //   printf("%s\n", pseudo);
  // }
  // return 0;

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

  if(msg.DATA)
    free(msg.DATA);

  //*** fermeture socket client ***
  close(sockclient);
  //*** fermeture socket serveur ***
  close(sock);
  
  return 0;
}
