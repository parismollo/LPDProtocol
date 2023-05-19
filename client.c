#include "megaphone.h"

#define SIZE_MESS 1024
// #define IP_SERVER "fdc7:9dd5:2c66:be86:4849:43ff:fe49:79bf"
#define IP_SERVER "::1"
#define PORT 7777
#define CLIENT_ID_FILE "client_id.data"
#define CLIENT_MCADDRESS "address.data"

int ID = 0; // ID zero is not valid, valid id has to be > 0.

// permet de vérifier si le client est déjà abonné au service. 
// Elle lit l'ID du client à partir d'un fichier et renvoie 1 si l'ID est valide (supérieur à 0) ou 0 sinon.
int check_subscription() {
    char buffer[SIZE_MESS];
    memset(buffer, 0, sizeof(buffer));
    
    int fd = open(CLIENT_ID_FILE, O_RDONLY);
    if (fd == -1) {
        // Failed to find id
        return 0;
    }
    int n;
    if((n = read(fd, buffer, SIZE_MESS)) < 0) {
        // Failed to find id
        perror("read in check_subscription");
        close(fd);
        return 0;
    }

    buffer[n] = '\0';
    ID = atoi(buffer);
    if (ID == 0) {return 0;}
    // ID found
    return 1;
}

int send_error(int sock, char* msg) {
    if(errno)
      perror(msg);
    else
      fprintf(stderr, "%s\n", msg);

    if(sock)
      close(sock);

    return -1;
}

int server_notification_post(int sockclient, client_msg* cmsg) {
  if (sockclient < 0)
    return 1;
  uint16_t res;
  int recu = recv(sockclient, &res, sizeof(uint16_t), 0);
  if (recu <= 0){
    perror("erreur lecture notif post");
    return(1);
  }

  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if (cmsg->CODEREQ == 31) return 1;
  cmsg->ID = (res & 0xFFE0) >> 5;
  recu = recv(sockclient, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    perror("erreur lecture notif post");
    return 1;
  }
  cmsg->NUMFIL = ntohs(res);
  recu = recv(sockclient, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    return send_error(sockclient, "error recv");
  }
  
  cmsg->NB = ntohs(res);
  recu = recv(sockclient, &res, sizeof(uint8_t), 0);
  
//   if (recu <= 0){ No data here for server notification
//     return send_error(sockclient, "xerror recv");
//   }
  
  cmsg->DATALEN = res;
  memset(cmsg->DATA, 0, 256);
  
  recu = recv(sockclient, cmsg->DATA, cmsg->DATALEN, 0);
  if (recu != cmsg->DATALEN) {
    return send_error(sockclient, "error DATALEN");
  }

  printf("[Server response]: CODEREQ: %d ID: %d NUMFIL: %d DATA: %s\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->DATA);

  return 0;
}

// Permet d'envoyer un billet au serveur pour un numéro de fil donné. 
// Elle crée une structure client_msg avec le code de requête 2 (envoi de billet) 
// et les informations nécessaires, puis appelle la fonction query_server pour envoyer la requête au serveur.
int send_ticket(int sock, int numfil, char* text) {
    client_msg msg;
    msg.CODEREQ = 2;
    msg.ID = ID;
    msg.NUMFIL = numfil;
    msg.NB = 0;
    msg.DATALEN = strlen(text);
    memset(msg.DATA, 0, 256);
    strncpy(msg.DATA, text, strlen(text));
    if (query_server(sock, &msg) == 0) {
        client_msg msg; //TODO : pourquoi ?
        return server_notification_post(sock, &msg);
    }
    return 1;
}

int query_server(int sock, client_msg* msg) {
    // printf("id: %d, codreq: %d\n", msg->ID, msg->CODEREQ);
    msg->ID &= 0x07FF; // On garde que les 11 premiers bits 
    msg->CODEREQ &= 0x001F; // On garde que les 5 premiers bits
    
    // Combine le codereq (5 bits de poids faible) avec l'ID (11 bits restants)
    uint16_t res = ((uint16_t)msg->CODEREQ) | (msg->ID << 5);
    
    res = htons(res); 
    if (send(sock, &res, sizeof(res), 0) < 0)
      return send_error(sock, "send failed"); 
  
    u_int16_t tmp = htons(msg->NUMFIL);
    if (send(sock, &tmp, sizeof(u_int16_t), 0) < 0)
      return send_error(sock, "send failed"); 

    tmp = htons(msg->NB);
    if (send(sock, &tmp, sizeof(u_int16_t), 0) < 0)
      return send_error(sock, "send failed");

    if (send(sock, &(msg->DATALEN), sizeof(u_int8_t), 0) < 0)
      return send_error(sock, "send failed");
    printf("DATA: %s\n", msg->DATA);

    if(msg->DATALEN > 0) {
      if (send(sock, msg->DATA, msg->DATALEN, 0) < 0)
        return send_error(sock, "send failed");
    }
    else {
      // memset(msg->DATA, 0, 255);
      // if (send(sock, msg->DATA, 255, 0) < 0)
      //   return send_error(sock, "send failed");
    }
    
    return 0;
}

// Permet de demander au serveur de s'abonner au service en fournissant un pseudo. 
// Elle envoie une requête avec le code 1 (abonnement) et le pseudo du client. 
// Elle attend ensuite une réponse du serveur contenant l'ID du client, qu'elle stocke 
// dans un fichier pour une utilisation ultérieure.
int query_subscription(int sock, char* pseudo) {
    uint16_t a = htons(1);

    send(sock, &a, sizeof(uint16_t), 0);

    char buffer[10];
    // On remplit le buffer avec des '#'
    memset(buffer, '#', sizeof(buffer));
    memmove(buffer, pseudo, strlen(pseudo));

    if(send(sock, buffer, 10, 0) < 0) send_error(sock, "send failed");
    // printf("nombre d'octets envoyés: %d\n", ecrit);
    uint16_t res, id, codereq;
    if(recv(sock, &res, sizeof(res), 0) < 0) {
        fprintf(stderr, "error recv");
        return 1;
    }
    res = ntohs(res);
    codereq = (res & 0x001F); // On mask avec 111110000..
    if (codereq == 31) return 1;
    id = (res & 0xFFE0) >> 5;
    printf("[Server response]: CODEREQ: %d ID: %d\n", codereq, id);
    
    if(codereq != 1) {
        fprintf(stderr, "error codereq != 1");
        return 1;
    }

    int fd = open(CLIENT_ID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0) {
        perror("open file");
        return 1;
    }

    sprintf(buffer, "%hd", id);
    if(write(fd, buffer, strlen(buffer)) < 0) {
        // fprintf(stderr, "error write id in users_client file");
        perror("write id in user_client file");
        close(fd);
        return 1;
    }
    
    close(fd);

    return 0;
}

// Permet de récupérer un certain nombre de billets pour un numéro de fil donné. 
// Elle crée une structure client_msg avec le code de requête 3 (récupération de billets), 
// le numéro de fil et le nombre de billets demandés,    
// puis appelle la fonction query_server pour envoyer la requête au serveur.
int process_ticket(int sock) {
  
  // read numfil
  uint16_t numfil;
  if(recv(sock, &numfil, sizeof(uint16_t), 0) < 0) {
    return send_error(sock, "recv failed1");
  }
  numfil = ntohs(numfil);

  // read origine
  char origine[11];
  if(recv(sock, origine, 10, 0) < 0) return send_error(sock, "recv failed2");
  origine[10] = '\0';
  clear_pseudo(origine);
  // read pseudo
  char pseudo[11];
  if(recv(sock, pseudo, 10, 0) < 0) return send_error(sock, "recv failed3");
  pseudo[10] = '\0';
  clear_pseudo(pseudo);
  // read datalen
  
  uint8_t len;
  if(recv(sock, &len, sizeof(uint8_t), 0) < 0) {
    return send_error(sock, "recv failed4");
  }
  
  if(len > 0) {
    // read data
    char *data = malloc(len+1);
    if (data == NULL) {
      return send_error(sock, "malloc failed5");
    }
    memset(data, 0, len+1);
    if(recv(sock, data, len, 0) <= 0) {
      free(data);
      return send_error(sock, "recv failed6");
    }
    printf("[Server response]: NUMFIL: %hu ORIGIN: %s PSEUDO: %s DATALEN: %d DATA: %s\n", numfil, origine, pseudo, len, data);
    free(data);
  }
  else {
    printf("[Server response]: NUMFIL: %hu ORIGIN: %s PSEUDO: %s, DATALEN: %d\n", numfil, origine, pseudo, len);
  }

  return 0;
}

int server_notification_get(int sock, client_msg* cmsg) {
  // Handle first message
  if (sock < 0)
    return 1;
  uint16_t res;
  int recu = recv(sock, &res, sizeof(uint16_t), 0);
  if (recu <= 0){
    perror("erreur lecture notif getx");
    return(1);
  }
  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if(cmsg->CODEREQ == 31) return 1; 
  cmsg->ID = (res & 0xFFE0) >> 5;
  
  recu = recv(sock, &res, sizeof(uint16_t), 0); 
  if (recu <= 0) {
    perror("erreur lecture notif get");
    return 1;
  }
  cmsg->NUMFIL = ntohs(res);
  
  recu = recv(sock, &res, sizeof(uint16_t), 0);
  if (recu <= 0){
    return send_error(sock, "error recv");
  }
  cmsg->NB = ntohs(res);

  printf("[Server response]: CODEREQ: %d ID: %d NUMFIL: %d :  NB: %d\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->NB);

  // Handle next n messages
  for(int i=0; i<cmsg->NB; i++) {

    if(process_ticket(sock) < 0) {
      fprintf(stderr, "ERREUR: process ticket\n");
      return -1;
    }
  }
  return 0;
}

int save_mcaddress(char * address) {
  FILE* file;
  file = fopen(CLIENT_MCADDRESS, "a");
  if(file == NULL) return 1;
  fprintf(file, "%s\n", address);
  fclose(file);
  return 0;
}

int server_notification_subscription(int sock, client_msg * cmsg) {
  if (sock < 0)
    return 1;
  uint16_t res;
  int recu = recv(sock, &res, sizeof(uint16_t), 0);
  if (recu <= 0){
    perror("erreur lecture notif abonnement");
    return(1);
  }

  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if(cmsg->CODEREQ == 31) return 1; 
  cmsg->ID = (res & 0xFFE0) >> 5;
  recu = recv(sock, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    perror("erreur lecture notif abonnement");
    return 1;
  }
  cmsg->NUMFIL = ntohs(res);
  recu = recv(sock, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    return send_error(sock, "error recv");
  }
  cmsg->NB = ntohs(res);

  recu = recv(sock, &res, sizeof(uint8_t), 0);
  cmsg->DATALEN = res;

  uint8_t addrmult[16]; // Use an array of 16 bytes for the binary multicast address
  if (recv(sock, addrmult, sizeof(addrmult), 0) < 0) {
    // handle error
    return 1;
  }
  char addrmult_str[40];
  inet_ntop(AF_INET6, addrmult, addrmult_str, sizeof(addrmult_str));
  if(save_mcaddress(addrmult_str) != 0) {
    perror("failed to save multicast address");
    return 1;
  }
  printf("[Server response]: CODEREQ: %d ID: %d NUMFIL: %d  NB: %d ADDRM: %s\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->NB, addrmult_str);
  return 0;
}

int get_tickets(int sock, int num_fil, int nombre_billets) {
    client_msg msg;
    msg.CODEREQ = 3;
    msg.ID = ID;
    msg.NUMFIL = num_fil;
    msg.NB = nombre_billets;
    msg.DATALEN = 0;
    memset(msg.DATA, 0, 256);
    if (query_server(sock, &msg) == 0) {
        client_msg msg;
        return server_notification_get(sock, &msg);
    }
    return 1; 
}

// Permet de s'abonner à un fil de discussion en envoyant une requête avec le code 4 (abonnement à un fil) 
// et le numéro de fil souhaité. Elle utilise également la fonction query_server pour envoyer la requête au serveur.
int subscribe_to_fil(int sock, int num_fil) {
  client_msg msg;
  msg.CODEREQ = 4;
  msg.ID = ID;
  msg.NUMFIL = num_fil;
  msg.NB = 0;
  msg.DATALEN = 0;
  memset(msg.DATA, 0, 256);
  if (query_server(sock, &msg) == 0) {
    client_msg msg;
    return server_notification_subscription(sock, &msg);
  }
  return 1;
}

int recv_server_query(int sock, client_msg* cmsg, int data) {
  if (sock < 0)
    return 1;
  uint16_t res;
  int recu = recv(sock, &res, sizeof(uint16_t), 0);
  if (recu <= 0) {
    fprintf(stderr, "erreur lecture");
    return 1;
  }

  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if(cmsg->CODEREQ == 31) return 1; 
  cmsg->ID = (res & 0xFFE0) >> 5;

  recu = recv(sock, &res, sizeof(uint16_t), 0);
  if (recu <= 0) {
    fprintf(stderr, "erreur lecture");
    return 1;
  }
  cmsg->NUMFIL = ntohs(res);
  recu = recv(sock, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    return send_error(sock, "error recv");
  }
  cmsg->NB = ntohs(res);

  // Si on ne doit pas recevoir de data, on quitte la fonction
  if(!data)
    return 0;

  recu = recv(sock, &res, sizeof(uint8_t), 0);
  cmsg->DATALEN = res;

  if(cmsg->DATALEN > 0) {
    memset(cmsg->DATA, 0, 256);
    recu = recv(sock, cmsg->DATA, cmsg->DATALEN, 0);
    if (recu <= 0) {
      perror("erreur lecture notif get");
      return 1;
    }
  }
  printf("**Server notification**: CODEREQ: %d ID: %d NUMFIL: %d :  NB: %d\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->NB);

  return 0;
}

int recv_server_thread_notification(int sock, notification* cmsg, struct sockaddr* cliaddr, socklen_t * len) {
  // This function refers to the notification being received by all the fils the user is subscribed to.
  // This is not the reply from cli-server tcp interecations!
  if (sock < 0)
    return 1;
  uint16_t res;
  int recu = recvfrom(sock, &res, sizeof(uint16_t), 0, (struct sockaddr*)cliaddr, len);
  if (recu <= 0) { 
    perror("Failed to receive CODEREQ and ID"); 
    return 1;
  }

  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if(cmsg->CODEREQ == 31) return 1; 
  cmsg->ID = (res & 0xFFE0) >> 5;

  recu = recvfrom(sock, &res, sizeof(uint16_t), 0, (struct sockaddr*)cliaddr, len);
  if (recu <= 0) { 
    perror("Failed to receive NUMFIL"); 
    return 1;
  }
  cmsg->NUMFIL = ntohs(res);

  recvfrom(sock, cmsg->PSEUDO, 10, 0, (struct sockaddr*)cliaddr, len);
  recvfrom(sock, cmsg->DATA, 20, 0, (struct sockaddr*)cliaddr, len);

  cmsg->PSEUDO[10] = '\0';
  cmsg->DATA[20] = '\0';

  printf("**Server notification**: CODEREQ: %d ID: %d NUMFIL: %d :  PSEUDO: %s DATA: %s\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->PSEUDO, cmsg->DATA);

  return 0;
}


////////////////////////////////////
// TELECHARGEMENT/ENVOIE FICHIERS //
////////////////////////////////////

int send_file_to_server(int sock, int num_fil, char* file_path) {
  client_msg msg;
  msg.CODEREQ = 5;
  msg.ID = ID;
  msg.NUMFIL = num_fil;
  msg.NB = 0;
  msg.DATALEN = strlen(file_path);
  memset(msg.DATA, 0, 256);
  strncpy(msg.DATA, file_path, msg.DATALEN);

  if (query_server(sock, &msg) != 0)
    return send_error(sock, "Error send query\n");

  //printf("**Server notification**: CODEREQ: %d ID: %d NUMFIL: %d DATA: %s\n", msg.CODEREQ, msg.ID, msg.NUMFIL, msg.DATA);
  client_msg s_msg;
  if(recv_server_query(sock, &s_msg, 0) != 0)
    return send_error(sock, "error recv from server\n");

  if(!(s_msg.CODEREQ == msg.CODEREQ && s_msg.ID == msg.ID &&
       s_msg.NUMFIL == num_fil)) {
    return send_error(sock, "Wrong server answer\n");
  }

  // On termine la connexion TCP avec le serveur
  close(sock);

  // On récupère le port UDP pour envoyer le fichier
  int UDP_port = s_msg.NB;

  //adresse de destination
  struct sockaddr_in6 servadr;
  memset(&servadr, 0, sizeof(servadr));
  servadr.sin6_family = AF_INET6;
  inet_pton(AF_INET6, IP_SERVER, &servadr.sin6_addr);
  servadr.sin6_port = UDP_port;

  printf("PORT UDP: %d\n", UDP_port);

  if(send_file(servadr, s_msg, file_path) < 0) {
    fprintf(stderr, "error send file with UDP\n");
    return -1;
  }

  return 0;
}

int download_server_file(int sock, int num_fil, int UDP_port, char* filename) {
  client_msg msg;
  msg.CODEREQ = 6;
  msg.ID = ID;
  msg.NUMFIL = num_fil;
  msg.NB = UDP_port;
  msg.DATALEN = strlen(filename);
  memset(msg.DATA, 0, 256);
  strncpy(msg.DATA, filename, msg.DATALEN);
  
  if (query_server(sock, &msg) != 0) {
      fprintf(stderr, "Error send query\n");
      return -1;
  }

  client_msg s_msg;
  if(recv_server_query(sock, &s_msg, 0))
    return send_error(sock, "error recv from server\n");

  if(!(s_msg.CODEREQ == msg.CODEREQ && s_msg.ID == msg.ID &&
       s_msg.NUMFIL == msg.NUMFIL && s_msg.NB == msg.NB)) {
    return send_error(sock, "Wrong server answer\n");
  }

  // On termine la connexion TCP avec le serveur
  close(sock);

  printf("PORT UDP: %d\n", UDP_port);

  Node* packets_list = NULL;
  if((packets_list = download_file(UDP_port, msg.ID, msg.CODEREQ)) == NULL) {
    fprintf(stderr, "Error in download_file\n");
    return -1;
  }

  char file_path[1024];
  memset(file_path, 0, 1024);
  snprintf(file_path, 1024, "fil%d_%s", msg.NUMFIL, filename);

  // On écrit les paquets dans le fichier
  if(write_packets_to_file(packets_list, file_path) < 0) {
    fprintf(stderr, "Erreur lors de l'écriture des packets dans le fichier\n");
    free_list(packets_list);
    return -1;
  }

  // On free la liste
  free_list(packets_list);

  return 0;
}

int cli(int sock) {
  printf("Megaphone says: Hi user! What do you want to do?\n");
  printf("(1) Subscription\n(2) Post ticket\n(3) Get tickets\n(4) Subscribe to fil\n(5) Send file\n(6) Download file\n(7) Close connection\n");
  int num, numfil;
  scanf("%d", &num);
  char filename[255];
  memset(filename, 0, 255);
  switch (num)
  {
  case 1:
    if(!check_subscription()) {
      printf("[Server response]: Type your username: \n");
      char username[100];
      scanf("%99s", username);
      query_subscription(sock, username);
      return 0;
    }else {
      printf("You already subscribed to megaphone\n");
      return 1;
    }
  case 2:
    check_subscription();
    printf("[Server response]: Type the numfil \n");
    scanf("%d", &numfil);
    printf("[Server response]: Enter your message \n");
    char data[100];
    scanf(" %[^\n]", data);
    data[99] = '\0';
    return send_ticket(sock, numfil, data);
  case 3:
    check_subscription();
   printf("[Server response]: Type the numfil and the number of messages (e.g. 1 2) \n");
   int a, b;
   scanf("%d %d", &a, &b);
   return get_tickets(sock, a, b);
  case 4:
    check_subscription();
    printf("[Server response]: Type the numfil \n");
    int num;
    scanf("%d", &num);
    return subscribe_to_fil(sock, num);
  case 5:
    check_subscription();
    printf("File : ");
    scanf("%s", filename);
    printf("Fil number : ");
    scanf("%d", &numfil);
    return send_file_to_server(sock, numfil, filename);
  case 6:
    check_subscription();
    printf("File : ");
    scanf("%s", filename);
    printf("Fil number : ");
    scanf("%d", &numfil);
    return download_server_file(sock, numfil, DEFAULT_UDP_PORT, filename);
  case 7:
    printf("Megaphone says: Closing connection...\n");
    return -1;
  default:
    printf("[Server response]: Nothing here so far...\n");
    return -1;
  }
  return 0;
}

void * multicast_receiver(void * arg) {
  while(1) {
    int port = 4321; // tmp
    int max = 10; // tmp
    char ** addresses = malloc(sizeof(char *) * max);
    FILE * file = fopen(CLIENT_MCADDRESS, "r");

    int address_len = 100;
    char buffer[101];
    int following = 0;

    if (file == NULL) {
      perror("Failed to open file");
      goto cleanup;
    }

    while(fgets(buffer, address_len, file) != NULL) {
      buffer[strcspn(buffer, "\n")] = '\0';
      addresses[following] = malloc(strlen(buffer) + 1);
      strcpy(addresses[following], buffer);
      following++;
      if(following >= max) break;
    }

    fclose(file);
    // Print addresses
    // for (int j = 0; j < following; j++) {
    //     printf("%s\n", addresses[j]);
    // }

    int sock;
    if((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      perror("echec de socket");
      goto cleanup;
    }

    int ok = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
      perror("echec de SO_REUSEADDR");
      close(sock);
      goto cleanup;
    }

    struct sockaddr_in6 grsock;
    memset(&grsock, 0, sizeof(grsock));
    grsock.sin6_family = AF_INET6;
    grsock.sin6_addr = in6addr_any;
    grsock.sin6_port = htons(port);

    if(bind(sock, (struct sockaddr*) &grsock, sizeof(grsock))) {
      perror("echec de bind");
      close(sock);
      goto cleanup;
    }

    int ifindex = if_nametoindex ("enp4s0"); // wlp2s0"); //tmp

    for(int j=0; j<following; j++) {
      struct ipv6_mreq group;
      inet_pton (AF_INET6, addresses[j], &group.ipv6mr_multiaddr.s6_addr);
      group.ipv6mr_interface = ifindex;

      if(setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group)) < 0) {
        perror("echec abonnement groupe");
        close(sock);
        goto cleanup;
      }
    }
    struct sockaddr_in6 cliaddr;
    socklen_t len = sizeof(cliaddr);
    int loops = 0;
    while (loops < following) {
      // memset(buf, 0, sizeof(buf));
      notification msg;
      recv_server_thread_notification(sock, &msg, (struct sockaddr *)&cliaddr, &len);
      loops++;
    }
    close(sock);    
    cleanup:
      for (int j = 0; j < following; j++) {
        free(addresses[j]);
      }
      free(addresses);
      continue;
  }
}

int main(int argc, char* argv[]) {
  pthread_t tid;
  int thread_started = 0;
  if (access(CLIENT_MCADDRESS, F_OK) != -1) {
    pthread_create(&tid, NULL, multicast_receiver, NULL);
    thread_started = 1;
  }

  int sock = socket(PF_INET6, SOCK_STREAM, 0);
  struct sockaddr_in6 adrso;
  memset(&adrso, 0, sizeof(adrso)); // On remplit de 0 la structure (important)
  adrso.sin6_family = AF_INET6;
  adrso.sin6_port = htons(PORT); // Convertir le port en big-endian
  
  inet_pton(AF_INET6, IP_SERVER, &adrso.sin6_addr); // On écrit l'ip dans adrso.sin_addr
  if (connect(sock, (struct sockaddr *) &adrso, sizeof(adrso)) < 0) send_error(sock, "connect failed"); // 0 en cas de succès, -1 sinon  
  // int loop = 1;
  // while (loop != -1) {
  cli(sock);
  // }
  close(sock);
  if (thread_started) {
    pthread_join(tid, NULL); // Wait for the thread to finish
  }
  return EXIT_SUCCESS;
}