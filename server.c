#include "megaphone.h"

#define SIZE_MESS 100
#define DATA_LEN 255

// Permet de stocker l'adresse du dernier client connecté
struct sockaddr_in6 CLIENT_ADDR;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;


void print_connexion(struct sockaddr_in6 adrclient){
  char adr_buf[INET6_ADDRSTRLEN];
  memset(adr_buf, 0, sizeof(adr_buf));
  
  inet_ntop(AF_INET6, &(adrclient.sin6_addr), adr_buf, sizeof(adr_buf));
}

int send_error(int sockclient, char* msg) {
  print_error(msg);

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

int query_client(int sock, client_msg* msg) {
  msg->ID &= 0x07FF; // On garde que les 11 premiers bits 
  msg->CODEREQ &= 0x001F; // On garde que les 5 premiers bits

  // Combine le codereq (5 bits de poids faible) avec l'ID (11 bits restants)
  uint16_t res = ((uint16_t)msg->CODEREQ) | (msg->ID << 5);
  res = htons(res); 
  if(send(sock, &res, sizeof(res), 0) < 0)
    goto error;

  u_int16_t tmp = htons(msg->NUMFIL);
  if(send(sock, &tmp, sizeof(u_int16_t), 0) < 0)
    goto error;

  tmp = htons(msg->NB);
  if(send(sock, &tmp, sizeof(u_int16_t), 0) < 0)
    goto error;

  if(msg->CODEREQ == 4) {
    if(send(sock, msg->multicast_addr, 16,0) < 0)
      goto error;
  }
  return 0;

  error:
    return send_error(sock, "send failed");
}

int notification_query(int sock, notification* msg, struct sockaddr_in6 grsock) {
  msg->ID &= 0x07FF; // Keep only the first 11 bits 
  msg->CODEREQ &= 0x001F; // Keep only the first 5 bits

  // Combine the codereq (5 low order bits) with the ID (remaining 11 bits)
  uint16_t res = ((uint16_t)msg->CODEREQ) | (msg->ID << 5);
  res = htons(res); 
  sendto(sock, &res, sizeof(res), 0, (struct sockaddr*)&grsock, sizeof(grsock));

  u_int16_t tmp = htons(msg->NUMFIL);
  sendto(sock, &tmp, sizeof(tmp), 0, (struct sockaddr*)&grsock, sizeof(grsock));

  // Ensure PSEUDO is exactly 10 bytes
  char pseudo[11] = {0}; // Initialize to zeros
  strncpy(pseudo, msg->PSEUDO, 10);
  sendto(sock, pseudo, 10, 0, (struct sockaddr*)&grsock, sizeof(grsock));

  // Ensure DATA is exactly 20 bytes
  char data[21] = {0}; // Initialize to zeros
  strncpy(data, msg->DATA, 20);
  sendto(sock, data, 20, 0, (struct sockaddr*)&grsock, sizeof(grsock));
  
  return 0;
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
  query_client(sockclient, &msg);

  return 0;
}   

void updateFileInfo(char *filepath, int reset) {
    FILE *file = fopen(filepath, "r");
    int number = 0;  // initialize number to 0

    if (file != NULL) {
        if (fscanf(file, "%d", &number) == 1) {
            number++;
        }
        fclose(file);
    }

    if (reset) {
        number = 0;
    }

    file = fopen(filepath, "w");
    if (file == NULL) {
      print_error("Error opening/creating the file for writing");
      return;
    }

    fprintf(file, "%d\n", number);
    fclose(file);
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
  notification.DATALEN = 0;
  memset(notification.DATA, 0, 255);

  return query_client(sock, &notification);
}

// Permet de recupérer un message d'un client et de le publier sur un fil
// Si notify vaut true, on renvoie un message au client pour confirmer
// Sinon, on n'envoie pas de message de confirmation
int handle_ticket(int socket, client_msg* msg, int notify) {
  // This function supposes that the msg has been validated
  char buf[100];
  int fd, n;
  
  if (msg->NUMFIL == 0) {
    memset(buf, 0, sizeof(buf));
    msg->NUMFIL = nb_fils() + 1;
    sprintf(buf, "fil%d", msg->NUMFIL);
    if(mkdir(buf, 0777)!=0) {
      perror("failed to create folder"); 
      return -1;    
    }
    if(increase_nb_fils() != 0)
      return send_error(socket, "error cannot increase nb fils number");
  }
  sprintf(buf, "fil%d/fil%d.txt", msg->NUMFIL, msg->NUMFIL);
  fd = open(buf, O_RDWR | O_CREAT, 0666);
  if(fd == 0) {
    return send_error(socket, "This fil does not exists"); //tmp
  }
  // On récupère le nombre de message dans le fil
  
  int nb_msg = nb_msg_fil(msg->NUMFIL);
  
  char otherbuf[50];
  memset(otherbuf, 0, sizeof(otherbuf));
  sprintf(otherbuf, "fil%d/fil%d.info", msg->NUMFIL, msg->NUMFIL);
  updateFileInfo(otherbuf, 0);

  // On récupère  le pseudo du client
  char pseudo[11];
  if(get_pseudo(msg->ID, pseudo, 11) != 0) {
    print_error("Erreur: impossible de recuperer le pseudo");
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
  // On confirme la reception au client
  if(notify)
    return notify_ticket_reception(socket, msg->CODEREQ, msg->ID, msg->NUMFIL);

  // Dans ce cas, on veut juste ecrire un message dans un fil
  // Mais sans discuter avec un client. C'est le cas lorsqu'on poste un billet
  // apres la reception d'un fichier
  return 0;
}

int send_msg_ticket(int sockclient, uint16_t numfil, char* origine, message msg) {
  
  numfil = htons(numfil);
  if(send(sockclient, &numfil, sizeof(numfil), 0) < 0)
    goto error;
  // origine est sur 10 octets. 11 pour le '\0'
  replace_after(origine, '\n', '#', 11); // On place des '#' pour combler à la fin
  replace_after(msg.pseudo, '\0', '#', 11); // De meme pour le pseudo
  
  // On envoie l'origine (pseudo createur du fil)
  if(send(sockclient, origine, 10, 0) < 0)
     goto error;

  // On envoie le pseudo de celui qui a envoye le msg
  if(send(sockclient, msg.pseudo, 10, 0) < 0)
    goto error;

  // Ne peut pas depasser 255
  uint8_t data_len = (uint8_t) strlen(msg.text);

  // On envoie DATA_LEN
  if(send(sockclient, &data_len, sizeof(uint8_t), 0) < 0)
    goto error;

  if(data_len > 0) {
    // On envoie le text du message (DATA)
    if(send(sockclient, msg.text, data_len, 0) < 0)
      goto error;
  }
  return 0;

  error:
    return send_error(sockclient, "send failed");
}

int list_tickets(int sockclient, client_msg* msg) {
  // This function supposes that the msg has been validated

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
  */

  // if(check_subscription)......

  client_msg response = {0};
  response.ID = msg->ID;
  response.CODEREQ = msg->CODEREQ;
  response.NUMFIL = msg->NUMFIL == 0 ? nb_fils() : msg->NUMFIL;


  if(msg->NUMFIL > 0) { // Correspond à un fil existant
    int nb_msg = nb_msg_fil(msg->NUMFIL);
    if(msg->NB > nb_msg || msg->NB == 0)
      response.NB = nb_msg;
    else
      response.NB = msg->NB;
  }
  else { // 0 -> NB msg de tous les fils
    response.NB = total_msg_fils(msg->NB);
  }

  if(query_client(sockclient, &response) < 0)
    return send_error(sockclient, "error: send entete list_tickets");

  /*
    2. Envoyer les N derniers messages du fil
  */
  char initator[11] = {0};

  int i = 1; // Par défaut, i=1, on va parcourir tous les fils
  if(msg->NUMFIL > 0) // Sinon, si on veut un fil en particulier
    i = response.NUMFIL; // On fait 1 seul tour de boucle.

  for(;i<=response.NUMFIL;i++) {
    memset(initator, 0, 11);
    if(get_fil_initiator(i, initator, 11) != 0) {
      // LE FIL N EXISTE PAS
      continue;
    }

    // On récupère le nombre de messages qu'on veut
    int nb_msg = nb_msg_fil(i);
    if(msg->NB <= nb_msg && msg->NB != 0)
      nb_msg = msg->NB;

    message* messages = malloc(sizeof(message) * nb_msg);
    if(messages == NULL) {
      // send error
      return -1;
    }

    if(get_last_messages(nb_msg, i, messages) != 0) {
      // send error
      free(messages);
      return -1;
    }

    for(int j=0;j<nb_msg;j++) {
      if(send_msg_ticket(sockclient, i, initator, messages[j]) != 0) {
        // send error
        free(messages);
        return -1;
      }
    }

    free(messages);
  }

  return 0;
}

char * generate_multicast_address_ipv6(int group_id) {
    char* multicast_address = malloc(40); 
    if(multicast_address == NULL) {
      perror("error malloc");
      return NULL;
    }
    memset(multicast_address, 0, 40);
    uint8_t prefix[] = {0xff, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    prefix[11] = group_id;
    inet_ntop(AF_INET6, prefix, multicast_address, 40);
    return multicast_address;
}

char * get_multicast_address(int numfil){
  char buffer[256];
  sprintf(buffer, "fil%d/multicast_address_fil_%d.txt", numfil, numfil);
  struct stat st;
  if (stat(buffer, &st) == 0) { // file exists
    char* address = malloc(40);
    if(address == NULL) {
      return NULL;
    }
    memset(address, 0, 40);
    FILE* file = fopen(buffer, "r");
    if (file == NULL) {
      free(address);
      return NULL;
    }
    if(fgets(address, 40, file) != NULL) {
      fclose(file);
      return address;
    }
    fclose(file);
    free(address);
  } // file does not exist
  char* address = generate_multicast_address_ipv6(numfil);
  FILE* file = fopen(buffer, "w");
  if(file == NULL) return NULL;
  fprintf(file, "%s", address);
  fclose(file);
  return address;
}

int subscription_fil(int sockclient, client_msg* msg){
  char * multicast_addr;
  if((multicast_addr = get_multicast_address(msg->NUMFIL)) == NULL) 
      return send_error(sockclient, "error: could not add subscription");
  
  // Confirmation de l'abonnement
  client_msg response = {0};
  response.ID = msg->ID;
  response.CODEREQ = 4;
  response.NUMFIL = msg->NUMFIL;
  response.NB = 0;
  inet_pton(AF_INET6, multicast_addr, response.multicast_addr);
  if(query_client(sockclient, &response) < 0) {
    free(multicast_addr);
    return send_error(sockclient, "error: could not confirm subscription");
  }
  free(multicast_addr);
  return 0;
}

////////////////////////////////////
// TELECHARGEMENT/ENVOIE FICHIERS //
////////////////////////////////////

int send_file_to_client(int clientsock, client_msg* msg) {
  // On stocke le nom du fichier pour plus tard
  char file_name[256];
  memset(file_name, 0, 256);
  strncpy(file_name, msg->DATA, msg->DATALEN);

  // On garde CODREQ, ID, NUMFIL et NB pareil
  msg->DATALEN = 0;
  memset(msg->DATA, 0, 256);

  if(query_client(clientsock, msg) < 0) {
    return send_error(clientsock, "error: cannot send msg in download_client_file");
  }

  // On termine la connexion TCP avec le client
  close(clientsock);

  // On récupère le port UDP pour envoyer le fichier
  int UDP_port = msg->NB;

  // Adresse de destination
  // struct sockaddr_in6 clientadr;
  // memset(&clientadr, 0, sizeof(clientadr));
  // clientadr.sin6_family = AF_INET6;
  // inet_pton(AF_INET6, "::1", &clientadr.sin6_addr); // TODO: POUR L INSTANT ON MET ::1. ATTENTION A MODIFIER PAR LA VRAI ADRESSE DU CLIENT QU ON RECUPERE DANS ACCEPT !!
  // clientadr.sin6_port = UDP_port;

  // On récupère le sockaddr_in6 du dernier client
  // On modifie le port
  CLIENT_ADDR.sin6_port = UDP_port;

  char file_path[1024];
  memset(file_path, 0, 1024);
  sprintf(file_path, "fil%d/%s", msg->NUMFIL, file_name);

  if(send_file(CLIENT_ADDR, *msg, file_path) < 0) {
    print_error("error send file with UDP");
    return -1;
  }

  return 0;
}

int download_client_file(int clientsock, client_msg* msg, int UDP_port) {
  // On stocke le nom du fichier pour plus tard
  char file_name[256];
  memset(file_name, 0, 256);
  strncpy(file_name, msg->DATA, msg->DATALEN);

  // On garde CODREQ, ID et NUMFIL pareil
  msg->NB = UDP_port;
  msg->DATALEN = 0;

  if(query_client(clientsock, msg) < 0) {
    return send_error(clientsock, "error: cannot send msg in download_client_file");
  }

  // On termine la connexion TCP avec le client
  close(clientsock);

  Node* packets_list = NULL;
  if((packets_list = download_file(msg->NB, msg->ID, msg->CODEREQ)) == NULL) {
    print_error("Error in download_file");
    return -1;
  }

  // On crée un "fake" message client pour pouvoir appeler la fonction handle_ticket
  // Cela a pour but d'écrire le nom du fichier recu dans le fichier du fil
  msg->NB = 0;
  msg->DATALEN = strlen(file_name);
  strncpy(msg->DATA, file_name, msg->DATALEN+1); // +1 pour copier aussi le '\0' et etre sur que tout va bien

  if(handle_ticket(-1, msg, 0) != 0) {
    fprintf(stderr, "error writing ticket in fil %d\n", msg->NUMFIL);
    free_list(packets_list);
    return -1;
  }

  // On récupère le numéro du fil (si c'est un nouveau fil)
  if(msg->NUMFIL == 0)
    msg->NUMFIL = nb_fils(); // C'est le dernier fil dans ce cas (il vient d etre créer par handle ticket)
  
  char file_path[1024];
  memset(file_path, 0, 1024);
  sprintf(file_path, "fil%d/%s", msg->NUMFIL, file_name);

  // On écrit les paquets dans le fichier
  if(write_packets_to_file(packets_list, file_path) < 0) {
    print_error("Erreur lors de l'écriture des packets dans le fichier");
    free_list(packets_list);
    return -1;
  }

  // On free la liste
  free_list(packets_list);

  return 0;
}

int validate_and_exec_msg(int socket, client_msg* msg) {

  u_int8_t req = msg->CODEREQ;
  if(req > 6 || req < 1) {
    send_error(socket, "the query code must be between 1 and 6");
    return -1;
  }
  if(req != 1) { // Verify that the user is regitered
    if(!is_user_registered(msg->ID)) {
      send_error(socket, "Please register");
      return -1;
    }

  }
  if(req > 2) { // Verifie que le fil existe
    if(msg->NUMFIL < 0 || msg->NUMFIL > nb_fils()) {
        send_error(socket, "This fil doesn't exist");
        return -1;
      }
    if(req == 4 || req == 6){//Pour s'abonner ou télécharger, on ne peut pas donner le numéro de fil 0
      if(msg->NUMFIL == 0){
        send_error(socket, "Please provide a valid fil number");
        return -1;
      }
    }
  }
  char pseudo[11];
 
  switch(req) {
    case 1 :
      memset(pseudo, 0, sizeof(pseudo));
      int n = recv(socket, pseudo, 10, 0);
      if (n < 0) {send_error(socket, "error recv");}
        pseudo[n] = '\0';
      clear_pseudo(pseudo);

      // Vérification du pseudo
      if (strlen(pseudo) == 0) {
        send_error(socket, "Username is empty");
        return -1;
      }
      // If pseudo is valid, call the appropriate function
      return recv_client_subscription(socket, msg);

    case 2 :
      if(msg->NB != 0) {
        send_error(socket, "NB value must be 0");
        return -1;
      }
      return handle_ticket(socket, msg, 1);
    
    case 3 :
      return list_tickets(socket, msg);

    case 4 :
      if(msg->NB != 0) {
        return send_error(socket, "NB value must be 0");
      }
      if(msg->DATALEN != 0) {
        return send_error(socket, "DATALEN must be 0");
      } 
      return subscription_fil(socket, msg);
    case 5:
      printf("Reception of a client file...\n");
      return download_client_file(socket, msg, DEFAULT_UDP_PORT);
    case 6:
      printf("Sending a file...\n");
      return send_file_to_client(socket, msg);
  }
  send_error(socket, "issue in function validate_and_exec_msg");
  return -1;
}


int recv_client_msg(int sockclient) {
  if (sockclient < 0)
    return 1;
  //*** reception d'un message ***
  uint16_t res;
  int recu = recv(sockclient, &res, sizeof(uint16_t), 0);
  if (recu < 0) {
    perror("erreur lecture");
    return 1;
  }
  
  // Ici on va convertir en ordre du host et prendre les valeurs de codereq et id. Pour ID on a besoin de
  // les caler à nouveau 5 bits vers la droite, voir dessin pour mieux comprendre. (board.excalidraw)
  res = ntohs(res);

  client_msg cmsg;
  cmsg.CODEREQ = (res & 0x001F); // On mask avec 111110000..
  cmsg.ID = (res & 0xFFE0) >> 5;
  // + vérifier que ID dépasse pas 11bits ?
  // if(cmsg.CODEREQ > 6) {
  //   return send_error(sockclient, "CODEREQ too large");
  // }
  // else if(cmsg.CODEREQ == 1) { // INSCRIPTION
  //   return recv_client_subscription(sockclient, &cmsg);
  // }
  recu = recv(sockclient, &res, sizeof(uint16_t), 0);
  if (recu < 0) {
    perror("erreur lecture");
    return 1;
  }
  cmsg.NUMFIL = ntohs(res);

  recu = recv(sockclient, &res, sizeof(uint16_t), 0);
  if (recu <= 0) {
    return send_error(sockclient, "error recv");
  }
  cmsg.NB = ntohs(res);
  
  recu = recv(sockclient, &res, sizeof(uint8_t), 0);
  if (recu <= 0) {
    return send_error(sockclient, "error recv");
  }
  cmsg.DATALEN = res;

  if(cmsg.DATALEN > 0) {
    memset(cmsg.DATA, 0, 256);
    recu = recv(sockclient, cmsg.DATA, cmsg.DATALEN, 0);
    if (recu != cmsg.DATALEN) {
      return send_error(sockclient, "error DATALEN");
    }
  }
    

  return validate_and_exec_msg(sockclient, &cmsg);
}

int broadcast(int port, int numfil) {
  int sock;

  if((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    perror("erreur socket");
    return -1;
  }

  struct sockaddr_in6 grsock;
  memset(&grsock, 0, sizeof(grsock));
  grsock.sin6_family = AF_INET6;
  
  char * multicast_address = get_multicast_address(numfil);
  inet_pton(AF_INET6, multicast_address, &grsock.sin6_addr);
  grsock.sin6_port = htons(port);

  int ifindex = 0;
  
  if(setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex))) {
    perror("Error initializing the local interface.");
    return 1;
  }

  char buf[50];
  int nb_msg;
  sprintf(buf, "fil%d/fil%d.info", numfil, numfil);
  pthread_mutex_lock(&file_mutex);  // lock mutex
  FILE *file = fopen(buf, "r");
  if (file == NULL) {
      nb_msg = 1;
  }else {
    if (fscanf(file, "%d", &nb_msg) != 1) nb_msg = 1;
  }
  fclose(file);
  pthread_mutex_unlock(&file_mutex);
  message* messages = malloc(sizeof(message) * nb_msg);
  if(messages == NULL) {
    // send error
    return -1;
  }
  if(get_last_messages(nb_msg, numfil, messages) != 0) {
    goto cleanup;
  }
  
  notification msg;
  msg.CODEREQ = 4;
  msg.ID = 0;
  msg.NUMFIL = numfil;
  for(int j=0;j<nb_msg;j++) {
    memset(msg.DATA, '\0', sizeof(msg.DATA));
    strncpy(msg.DATA, messages[j].text, 20);
    strncpy(msg.PSEUDO, messages[j].pseudo, 10);
    msg.PSEUDO[10] = '\0';
    notification_query(sock, &msg, grsock);
  }

  goto cleanup;

  cleanup:
    close(sock);
    free(multicast_address);
    free(messages);
    return 0;
}

void * update_thread(void * arg) {
    int sleep_value = 60;
    while(1) {
        sleep(sleep_value);
        int nb = nb_fils();
        char info[50];
        for(int i=1; i<=nb; i++) {
            memset(info, 0, 50);
            sprintf(info, "fil%d/fil%d.info", i, i);
            pthread_mutex_lock(&file_mutex);
            updateFileInfo(info, 1);
            pthread_mutex_unlock(&file_mutex);
        }
    }
    return NULL;
}

void * multicast_thread(void * arg) {
    pthread_t tid;
    if (pthread_create(&tid, NULL, &update_thread, NULL) != 0) {
        perror("Failed to create thread");
        return NULL;
    }

    // Broadcast loop
    while(1) {
      int nb = nb_fils();
      for(int i=1; i<=nb; i++) {
          broadcast(NOTIFICATION_UDP_PORT, i);
      }
      sleep(10);
    }

    pthread_join(tid, NULL);

    return NULL;
}



int main(int argc, char** args) {
  // Par défaut, le serveur tournera sur le port 7777
  int port = 7777;

  if(argc >= 2) {
    if(strcmp(args[1], "--help") == 0) {
      fprintf(stderr, "Usage: %s <port>\n", args[0]);
      return 0;
    }
    port = atoi(args[1]);
  }

  printf("Lancement du serveur megaphone sur le port %d\n", port);

  // Multicast feature running in a different thread
  pthread_t multicast_tid;
  int * mcport = malloc(sizeof(int));
  // *mcport = NOTIFICATION_UDP_PORT;
  if(pthread_create(&multicast_tid, NULL, multicast_thread, NULL) < 0) {
    perror("pthread_create failed");
    return 1;
  }

  //*** creation de la socket serveur ***
  int sock = socket(PF_INET6, SOCK_STREAM, 0);
  if(sock < 0) {
    perror("creation socket");
    return 1;
  }
  
  //*** creation de l'adresse du destinataire (serveur) ***
  struct sockaddr_in6 address_sock;
  memset(&address_sock, 0, sizeof(address_sock));
  address_sock.sin6_family = AF_INET6;
  address_sock.sin6_port = htons(port);
  address_sock.sin6_addr = in6addr_any;

  // Pour avoir une socket polymorphe : 
  int no = 0;
  int r = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
  if(r < 0) {
    fprintf(stderr, "Failure of setsockopt() : (%d)\n", errno);
    return 1;
  }
  
  int yes = 1;
  r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if(r < 0) {
    fprintf(stderr, "Failure of setsockopt() : (%d)\n", errno);
    close(sock);
    return 1;
  }

  //*** on lie la socket au port PORT ***
  r = bind(sock, (struct sockaddr *) &address_sock, sizeof(address_sock));
  if(r < 0) {
    perror("erreur bind");
    close(sock);
    return 1;
  }

  //*** Le serveur est pret a ecouter les connexions sur le port PORT ***
  r = listen(sock, 0);
  if(r < 0) {
    perror("erreur listen");
    close(sock);
    return 1;
  }

  socklen_t cli_len = sizeof(CLIENT_ADDR);
  while(1) {
    //*** le serveur accepte une connexion et cree la socket de communication avec le client ***
    memset(&CLIENT_ADDR, 0, sizeof(struct sockaddr_in6));
    int sockclient = accept(sock, (struct sockaddr *) &CLIENT_ADDR, &cli_len);
    if(sockclient == -1) {
      perror("issue with socket client");
      exit(1);
    }
    switch(fork()) {
    case -1:
      break;
    case 0:
      close(sock);
      print_connexion(CLIENT_ADDR);
      return recv_client_msg(sockclient);
    default:
      close(sockclient);
      while(waitpid(-1, NULL, WNOHANG) > 0);
    }
  }
  close(sock);
  free(mcport);
  return 0;
}
