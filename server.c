#include "megaphone.h"

#define SIZE_MESS 100
#define DATA_LEN 255
#define DATABASE "server_users.data"
#define INFOS "infos.data"

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

  if (msg->CODEREQ == 2) {
    if (send(sock, &msg->DATALEN, sizeof(u_int8_t), 0) < 0) send_error(sock, "send failed");
    if (send(sock, msg->DATA, msg->DATALEN, 0) < 0) send_error(sock, "send failed");
  }
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

  if(read(fd, buffer, sizeof(buffer))<0){
    perror("read in recv_client_subscription");
    close(fd);
    return -1;
  }

  if(strlen(buffer) != 0)
    id = atoi(buffer);
  id++;

  goto_last_line(fd);

  clear_pseudo(pseudo);
  sprintf(buffer, "%d %s\n", id, pseudo);
  if(write(fd, buffer, strlen(buffer))<0){
    perror("write in recv_client_subscription");
    close(fd);
    return -1;
  }

  sprintf(buffer, "%d", id);
  if(write(fd, buffer, strlen(buffer))<0){
    perror("write in recv_client_subscription");
    close(fd);
    return -1;
  }

  close(fd);

  // ON ENVOIE L ID A L UTILISATEUR
  client_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.ID = id;
  msg.CODEREQ = 1;
  query(sockclient, &msg);

  return 0;
}   

int nb_msg_fil(int fil) {
  char buf[100] = {0};
  sprintf(buf, "fil%d/fil%d.txt", fil, fil);
  int fd = open(buf, O_RDONLY, 0666);
  if(fd < 0) {
    perror("erreur ouverture fil");
    return 0;
  }
  goto_last_line(fd);
  memset(buf, 0, 100);
  int n = read(fd, buf, 100);
  if(n<0){
    perror("read in nb_msg_fil");
    close(fd);
    return 0;
  }
  close(fd);

  if(n <= 0)
    return 0;

  return atoi(buf);
}


int notify_ticket_reception(int sock, u_int8_t CODEREQ, uint8_t ID, int NUMFIL) {
  // Once ticket created successfully
  // notify client
  client_msg notification;
  memset(&notification, 0, sizeof(notification));
  notification.CODEREQ = CODEREQ;
  notification.ID = ID;
  notification.NUMFIL = NUMFIL;
  notification.NB = 0;
  notification.DATA = "Ticket created"; //tmp
  notification.DATALEN = strlen(notification.DATA);
  return query(sock, &notification);
}

int handle_ticket(int socket, client_msg* msg) {
  // This function supposes that the msg has been validated
  char buf[100];
  int fd, n;
  
  if (msg->NUMFIL == 0) {
    memset(buf, 0, sizeof(buf));
    msg->NUMFIL = nb_fils();
    sprintf(buf, "fil%d", msg->NUMFIL);
    if(mkdir(buf, 0777)!=0) {
      perror("failed to create folder"); 
      return -1;    
    }
  }
  sprintf(buf, "fil%d/fil%d.txt", msg->NUMFIL, msg->NUMFIL);
  fd = open(buf, O_RDWR | O_CREAT, 0666);
  if(fd == 0) {
    return send_error(socket, "This fil does not exists"); //tmp
  }
  // On récupère le nombre de message dans le fil
  int nb_msg = nb_msg_fil(msg->NUMFIL);

  // On récupère  le pseudo du client
  char pseudo[11];
  if(get_pseudo(msg->ID, pseudo, 11) != 0) {
    fprintf(stderr, "Erreur: impossible de recuperer le pseudo\n");
    return 1;
  }

  // va sur la derniere ligne du fichier
  goto_last_line(fd);

    // On écrit le pseudo
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "ID:%d PSEUDO:%s\nDATA: ", msg->ID, pseudo);
    if(write(fd, buf, strlen(buf))<0){
      perror("write pseudo in handle_ticket");
      close(fd);
      return -1;
    }

    // On écrit le message
    n = write(fd, msg->DATA, msg->DATALEN);
    if (n < 1 || write(fd, "\n", 1) < 1) {
      perror("failed to write message in handle_ticket");
      close(fd);
      return -1;
    }

    // On écrit le nouveau nb de msg total en bas du fichier
    sprintf(buf, "%d", ++nb_msg);
    n = write(fd, buf, strlen(buf));
    if(n <= 0) {
      perror("write nb msg in handle_ticket");
      close(fd);
      return -1;
    }
  
  return notify_ticket_reception(socket, msg->CODEREQ, msg->ID, msg->NUMFIL);
}

// Enregistre le pseudo dans la variable pseudo
int get_pseudo(int id, char* pseudo, size_t pseudo_size) {
  FILE* f = fopen(DATABASE, "r");
  if(f == NULL)
    return 1;
  
  char buf[1024];
  memset(buf, 0, 1024);
  char* ptr, *sep = " \n";
  while(fgets(buf, 1024, f) != NULL) {
    // printf("buf: %s\n", buf);
    ptr = strtok(buf, sep);
    if(ptr == NULL)
      return 1;
    
    if(atoi(ptr) == id) {
      
      ptr = strtok(NULL, sep);
      if(ptr == NULL)
        return 1;
      memset(pseudo, 0, 11);
      int min = strlen(ptr);
      if(min > 10)
        min = 10;
      memmove(pseudo, ptr, min);
      break;
    }
    memset(buf, 0, 1024);
  }
  fclose(f);

  return 0;
}

int increase_nb_fils() {
  int nb = nb_fils();
  char buffer[100];
  sprintf(buffer, "%d", nb+1);
  return change_infos("nb_fils", buffer);
}

int nb_fils() {
  char number[100];
  if(get_infos("nb_fils", number, 100) < 0) {
    // Le fichier n'existe pas ou alors la ligne
    // avec nb_fils n'existe pas donc on considère
    // qu'il n'y aucun fil
    return 0;
  }

  return atoi(number);
}

// On lui passe une clée
// Et la valeur sera enregistrée dans le pointeur value
int get_infos(char* key, char* value, size_t val_size) {
  FILE* file = fopen(INFOS, "r");
  if(file == NULL) {
    return -1;
  }
  char line[1024];
  char* delim = ";\n";
  while(fgets(line, 1024, file) != NULL) {
    char* ptr;
    if((ptr = strtok(line, delim)) != NULL) {
      if(strcmp(ptr, key) == 0) {
        // La clée correspond. On récupère la valeur
        ptr = strtok(NULL, delim);
        if(ptr == NULL) {
          // Le fichier est mal formé
          return -1;
        }
        strncpy(value, ptr, val_size);
        return 0;
      }
    }
  }
  
  return -1;
}

int change_infos(char* key, char* new_value) {
  char buffer[100];

  FILE * source = fopen(INFOS, "r");
  if (source == NULL) {
    printf("change_infos failed");
    return 1;
  }

  FILE * dest = fopen("temp", "w");
  if (dest == NULL) {
    printf("change_infos failed");
    return 1;
  }

  while(fgets(buffer, sizeof(buffer), source)) {
    if(strstr(buffer, key)) {
      fprintf(dest, "%s;%s", key, new_value);
    }else {
      fprintf(dest, "%s", buffer);
    }
  }

  fclose(source);
  fclose(dest);

  remove("infos.data");
  rename("temp", "infos.data");

  return 0;
}

int get_fil_initiator(int fil, char* initiator, size_t buf_size) {
  char buffer[1024], *ptr;
  memset(buffer, 0, 1024);
  sprintf(buffer, "fil%d/fil%d.txt", fil, fil);
  FILE* file = fopen(buffer, "r");
  if(file == NULL) {
    perror("impossible d'ouvrir le fil");
    return 1;
  }
  if(fgets(buffer, 1024, file) != NULL) {
    if((ptr = strstr(buffer, "PSEUDO:")) != NULL) {
      ptr += strlen("PSEUDO:");
      strncpy(initiator, ptr, buf_size);
      fclose(file);
      return 0;
    }
  }
  fclose(file);
  return 1;
}

int total_msg_fils() {
  // nb total de msg ds ts les fils
  // Appel nb_fils()
  // Pour chaque fil, sum += nb_msg(i)
  int n = nb_fils();
  int sum = 0;
  for(int i=1;i<=n;i++) {
    sum += nb_msg_fil(i);
  }
  return sum;
}

// Vérifie si pre est bien préfix de str
int prefix(char* str, char* pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

// Lit une ligne sans prendre le '\n'
int readline(int fd, char* line, size_t buf_size) {
  char c;
  int n;
  memset(line, 0, buf_size);
  for(int i=0;i<buf_size;i++) {
    if((n = read(fd, &c, 1)) < 0){
      perror("read in readline");
      close(fd);
      return 1;
    }
    else if(n == 0) // EOF -> End Of File
      return 0;
      
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

  char line[1024];

  int fd = open(buf, O_RDONLY, 0666);
  
  if(fd < 0) {
    perror("open file");
    return -1;
  }

  long file_size = lseek(fd, 0, SEEK_END);
    
  long num_lines = 0, current_pos;
  int skip_last_line = 1; // Permet de skip la derniere ligne
  for (current_pos = file_size - 1; current_pos >= 0; current_pos--) {
    lseek(fd, current_pos, SEEK_SET);
    if(read(fd, buf, 1) < 0){
      perror("read in get_last_message");
      close(fd);
      return -1;
    }
    if(*buf == '\n') {
      // On skip la derniere ligne (soit la premiere ligne rencontrée en partant de la fin)
      if(skip_last_line) {
        skip_last_line = 0;
        continue;
      }
      num_lines++;
    }
    if(num_lines >= nb * 2)
      break;
  }

  if(current_pos == -1) {
    // On a lu le fichier entierement. 1ere ligne comprise
    // On doit donc ajouter cette 1ere ligne au compteur
    num_lines++;
  }

  if(num_lines < nb * 2) {
    fprintf(stderr, "il y a moins de %d msg\n", nb);
    close(fd);
    return -1;
  }

  char* ptr;
  char* sep = ": \n";
  int j;
  for(int i=0;i<nb;i++) {
    memset(&messages[i], 0, sizeof(message));
    // On lit l'ID
    readline(fd, line, 1024); // GESTION ERREURS
    if(!prefix(line, "ID:"))
      goto bad_file_format;
    
    // On découpe la ligne en: [ID, 123, PSEUDO, leo]
    if((ptr = strtok(line, sep)) == NULL)
      goto bad_file_format;
    for(j=1;j<4 && (ptr = strtok(NULL, sep)) != NULL;j++) {
      if(j == 1) // ID
        messages[i].ID = atoi(ptr);
      else if(j == 3) // PSEUDO
        memmove(messages[i].pseudo, ptr, strlen(ptr));
    }
    if(j != 4)
      goto bad_file_format;
    
    // On lit le message
    readline(fd, line, 1024); // GESTION ERREURS
    if(!prefix(line, "DATA:"))
      goto bad_file_format;
    memmove(messages[i].text, line + 6, 255); // strlen("DATA: ") = 6
  }

  close(fd);
  return 0;

  bad_file_format:
    if(fd)
      close(fd);
    fprintf(stderr, "Bad file fil format\n");
    return -1;
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
    1. Envoyer un premier billet selon les conditions suivantes:
  */

  /*
    1) Vérifier que le client est bien inscrit
    2) Renvoyer un message au client avec le meme ID et CODEREQ
    3) Si NUMFIL est positif, on garde le même
       Sinon, si il est égale à 0, on a: NUMFIL=nb_fils()
    4) Si f (NUMFIL client) > 0:
          NB = SI n > nb_msg_fil f -> nb_msg_fil()
          OU SI n = 0 -> nb_msg_fil()
          SINON n
       SINON (si f n'est pas un vrai fil):
          NB = somme du nombre de messages envoyés de chaque fil.

– à n sinon
  */

  /*
    2. Envoyer les N derniers messages du fil
  */

  char buf[100] = {0};
  sprintf(buf, "fil%d/fil%d.txt", msg->NUMFIL, msg->NUMFIL);

  char initator[11] = {0};
  if(get_fil_initiator(msg->NUMFIL, initator, 11) != 0) {
    // LE FIL N EXISTE PAS
    // send error...
    return -1;
  }

  message* messages = malloc(sizeof(message) * msg->NB);
  if(messages == NULL) {
    // send error
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

int validate_and_exec_msg(int socket, client_msg* msg) {
  // This function validate and call the appropriate function according to the msg
  // check if message has a valid structure, check if id exists or not, etc
  // if codereq == ? then ...
  // etc..
  return handle_ticket(socket, msg); //tmp
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
  // printf("CODEREQ: %d ID: %d\n", cmsg->CODEREQ, cmsg->ID);
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
  recu = recv(sockclient, cmsg->DATA, cmsg->DATALEN, 0);
  // printf("recu: %d", recu);
  // printf("datalen: %d", cmsg->DATALEN);
  if (recu != cmsg->DATALEN) {
    return send_error(sockclient, "error DATALEN");
  }

  validate_and_exec_msg(sockclient, cmsg);
  return 0;
}

int main(int argc, char** args) {

  // char infos[1024];
  // get_infos("test", infos, sizeof(infos));
  // printf("%s\n", infos);

  // return 0;

  // int nb = nb_fils();

  // printf("nombre fils: %d\n", nb);
  // return 0;

  // TEST: get_last_messages
  // message mess[10];
  // get_last_messages(6, 0, mess);
  // for(int i=0;i<6;i++) {
  //   printf("%d %s %s\n", mess[i].ID, mess[i].pseudo, mess[i].text);
  // }

  // TEST: nb_msg_fil(fil)
  // printf("%d\n", nb_msg_fil(0));

  change_infos("nb_fils", "52");
  return 0;

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

  // Pour avoir une socket polymorphe : 
  int no = 0;
  int ret = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
  if (ret < 0) {
    fprintf(stderr, "échec de setsockopt() : (%d)\n", errno);
  }
  
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

  // Ajouter if msg != NULL ... car comme ça valgrind n'est pas content.
  // if(msg.DATA)
  //   free(msg.DATA);

  //*** fermeture socket client ***
  close(sockclient);
  //*** fermeture socket serveur ***
  close(sock);
  
  return 0;
}
