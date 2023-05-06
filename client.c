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
    close(sock);
    perror(msg);
    exit(1);
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
// et les informations nécessaires, puis appelle la fonction query pour envoyer la requête au serveur.
int send_ticket(int sock, int numfil, char* text) {
    client_msg msg;
    msg.CODEREQ = 2;
    msg.ID = ID;
    msg.NUMFIL = numfil;
    msg.NB = 0;
    msg.DATALEN = strlen(text);
    memset(msg.DATA, 0, 256);
    strncpy(msg.DATA, text, strlen(text));
    if (query(sock, &msg) == 0) {
        client_msg msg; //TODO : pourquoi ?
        return server_notification_post(sock, &msg);
    }
    return 1;
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
    if (send(sock, &msg->DATALEN, sizeof(u_int8_t), 0) < 0) send_error(sock, "send failed");

    if(msg->DATALEN > 0) {
      if (send(sock, msg->DATA, msg->DATALEN, 0) < 0) send_error(sock, "send failed");
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
// puis appelle la fonction query pour envoyer la requête au serveur.
int process_ticket(int sock) {
  // read numfil
  uint16_t numfil;
  if(recv(sock, &numfil, sizeof(uint16_t), 0) == -1 ) {
    return send_error(sock, "recv failed");
  }
  numfil = ntohs(numfil);
  // read origine
  char origine[11];
  if(recv(sock, &origine, 10, 0) == -1) return send_error(sock, "recv failed");
  origine[11] = '\0';
  clear_pseudo(origine);
  // read pseudo
  char pseudo[11];
  if(recv(sock, &pseudo, 10, 0) == -1) return send_error(sock, "recv failed");
  pseudo[11] = '\0';
  clear_pseudo(pseudo);
  // read datalen
  uint8_t len;
  if(recv(sock, &len, sizeof(uint8_t), 0) == -1) {
    return send_error(sock, "recv failed");
  }
  // read data
  char *data = malloc(len+1);
  if (data == NULL) {
    return send_error(sock, "malloc failed");
  }
  memset(data, 0, len+1);
  if(recv(sock, data, len, 0) <= 0) {
    free(data);
    return send_error(sock, "recv failed");
  }

  printf("[Server response]: NUMFIL: %hu ORIGIN: %s PSEUDO: %s DATA: %s\n", numfil, origine, pseudo, data);
  free(data);
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
  
  recu = recv(sock, &res, sizeof(uint8_t), 0);
  if (recu <= 0) {
    perror("erreur recv");
    return 1;
  }
  cmsg->DATALEN = res;

  if(cmsg->DATALEN > 0) {
    memset(cmsg->DATA, 0, 256);

    recu = recv(sock, cmsg->DATA, cmsg->DATALEN, 0);
    if (recu <= 0) {
      perror("erreur recv");
      return 1;
    }
  }
  printf("[Server response]: CODEREQ: %d ID: %d NUMFIL: %d :  NB: %d\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->NB);

  // Handle next n messages
  for(int i=0; i<cmsg->NB; i++) {
    process_ticket(sock);
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

int server_notification_abonnement(int sock, client_msg * cmsg) {
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
    if (query(sock, &msg) == 0) {
        client_msg msg;
        return server_notification_get(sock, &msg);
    }
    return 1; 
}

// Permet de s'abonner à un fil de discussion en envoyant une requête avec le code 4 (abonnement à un fil) 
// et le numéro de fil souhaité. Elle utilise également la fonction query pour envoyer la requête au serveur.
int abonner_au_fil(int sock, int num_fil) {
  client_msg msg;
  msg.CODEREQ = 4;
  msg.ID = ID;
  msg.NUMFIL = num_fil;
  msg.NB = 0;
  msg.DATALEN = 0;
  memset(msg.DATA, 0, 256);
  if (query(sock, &msg) == 0) {
    client_msg msg;
    return server_notification_abonnement(sock, &msg);
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

int send_file(int sock, int num_fil, char* filename) {
  client_msg msg;
  msg.CODEREQ = 5;
  msg.ID = ID;
  msg.NUMFIL = num_fil;
  msg.NB = 0;
  msg.DATALEN = strlen(filename);
  memset(msg.DATA, 0, 256);
  strncpy(msg.DATA, filename, strlen(filename));

  if (query(sock, &msg) != 0)
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

  // On récupère le port sur lequel on va envoyer les données
  int udp_port = s_msg.NB;
  printf("PORT UDP: %d\n", udp_port);
  // On crée le socket UDP
  int sock_udp = socket(PF_INET6, SOCK_DGRAM, 0);
  if (sock_udp < 0)
    return send_error(sock, "Error creation sock udp");
  
  //adresse de destination
  struct sockaddr_in6 servadr;
  memset(&servadr, 0, sizeof(servadr));
  servadr.sin6_family = AF_INET6;
  inet_pton(AF_INET6, IP_SERVER, &servadr.sin6_addr);
  servadr.sin6_port = udp_port;
  socklen_t len = sizeof(servadr);

  FILE* file = fopen(filename, "r");
  if(file == NULL) {
    close(sock_udp);
    return send_error(sock, "error open file");
  }

  FilePacket packet;
  memset(&packet, 0, sizeof(packet));
  // Combine le codereq (5 bits de poids faible) avec l'ID (11 bits restants)
  packet.codreq_id = ((uint16_t)msg.CODEREQ) | (msg.ID << 5);
  packet.codreq_id = htons(packet.codreq_id);

  uint16_t num_bloc = 1;
  int nb_read;
  printf("**CLIENT** BEGIN TRANSMISSION FILE\n");
  do {
    memset(packet.data, 0, 513);
    nb_read = fread(packet.data, sizeof(char), 512, file); // on lit 512 octets dans le fichier
    packet.num_bloc = htons(num_bloc);

    printf("**CLIENT** SEND PACKET, SIZE: %d\n", nb_read);
    if (sendto(sock_udp, &packet, sizeof(packet), 0, (struct sockaddr *)&servadr, len) < 0) {
      close(sock_udp);
      return send_error(sock, "send failed");
    }
    num_bloc++;
  } while(nb_read == 512);
  printf("**CLIENT** END TRANSMISSION FILE\n");
  
  fclose(file);
  close(sock_udp);

  return 0;
}

int download_file(int sock, int num_fil, int num_port, char* filename) {
  client_msg msg;
  msg.CODEREQ = 6;
  msg.ID = ID;
  msg.NUMFIL = num_fil;
  msg.NB = num_port;
  msg.DATALEN = strlen(filename);
  memset(msg.DATA, 0, 256);
  strncpy(msg.DATA, filename, msg.DATALEN);
  
  if (query(sock, &msg) == 0) {
      // TODO
  }

  // TODO

  return 1; 
}

int cli(int sock) {
  printf("Megaphone says: Hi user! What do you want to do?\n");
  printf("(1) Inscription\n(2) Poster billet\n(3) Get billets\n(4) Abonner au fil\n(5) Send file\n(6) Close connection\n");
  int num, numfil;
  scanf("%d", &num);
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
    return abonner_au_fil(sock, num);
  case 5:
    check_subscription();
    char filename[255];
    memset(filename, 0, 255);
    printf("File : ");
    scanf("%s", filename);
    printf("Fil number : ");
    scanf("%d", &numfil);
    return send_file(sock, numfil, filename);
  case 6:
    printf("Megaphone says: Closing connection...\n");
    return -1;
  default:
    printf("[Server response]: Nothing here so far...\n");
    return -1;
  }
  return 0;
}


int main(int argc, char* argv[]) {

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
  return EXIT_SUCCESS;
}