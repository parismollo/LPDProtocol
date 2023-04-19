#include "megaphone.h"

#define SIZE_MESS 1024
// #define IP_SERVER "fdc7:9dd5:2c66:be86:4849:43ff:fe49:79bf"
#define IP_SERVER "::1"
#define PORT 7777
#define CLIENT_ID_FILE "client_id.data"

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
    perror("erreur lecture");
    return(1);
  }

  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if (cmsg->CODEREQ == 31) return 1;
  cmsg->ID = (res & 0xFFE0) >> 5;
  recu = recv(sockclient, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    perror("erreur lecture");
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
  cmsg->DATA = malloc(sizeof(char) * cmsg->DATALEN);
  
  if(cmsg->DATA == NULL) {
    perror("malloc");
    return 1;
  }
  memset(cmsg->DATA, 0, cmsg->DATALEN);
  recu = recv(sockclient, cmsg->DATA, cmsg->DATALEN, 0);
  if (recu != cmsg->DATALEN) {
    return send_error(sockclient, "error DATALEN");
  }

  printf("**Server notification**: CODEREQ: %d ID: %d NUMFIL: %d DATA: %s\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->DATA);

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
    msg.DATA = text;
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
    if (send(sock, msg->DATA, msg->DATALEN, 0) < 0) send_error(sock, "send failed");

    return 0;
}

// Permet de demander au serveur de s'abonner au service en fournissant un pseudo. 
// Elle envoie une requête avec le code 1 (abonnement) et le pseudo du client. 
// Elle attend ensuite une réponse du serveur contenant l'ID du client, qu'elle stocke 
// dans un fichier pour une utilisation ultérieure.
int query_subscription(int sock, char* pseudo) {

    if(!check_subscription()) {
        close(sock);
        return EXIT_FAILURE;
    }

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
    printf("**Server notification**: CODEREQ: %d ID: %d\n", codereq, id);
    
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
  uint16_t numfil = ntohs(recv(sock, &numfil, sizeof(uint16_t), 0));  
  // read origine
  char origine[11];
  recv(sock, &origine, 10, 0);
  origine[11] = '\0';
  clear_pseudo(origine);
  // read pseudo
  char pseudo[11];
  recv(sock, &pseudo, 10, 0);
  pseudo[11] = '\0';
  clear_pseudo(pseudo);
  // read datalen
  uint8_t len = recv(sock, &len, sizeof(uint8_t), 0);
  // read data
  char *data = malloc(sizeof(char) * len);
  if (data == NULL) {
    // handle error
  }
  memset(data, 0, len);
  if(recv(sock, data, len, 0) <= 0) {
    // handle error
  }
  printf("**Server Notification**: NUMFIL: %d PSEUDO: %s DATA: %s\n", numfil, pseudo, data);
  return 0;
}

int server_notification_get(int sock, client_msg* cmsg) {
  // Handle first message
  if (sock < 0)
    return 1;
  uint16_t res;
  int recu = recv(sock, &res, sizeof(uint16_t), 0);
  if (recu <= 0){
    perror("erreur lecture");
    return(1);
  }

  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if(cmsg->CODEREQ == 31) return 1; 
  cmsg->ID = (res & 0xFFE0) >> 5;
  recu = recv(sock, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    perror("erreur lecture");
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

//   if (recu <= 0){ No data here for server notification
//     return send_error(sockclient, "xerror recv");
//   }
  
  cmsg->DATA = malloc(sizeof(char) * cmsg->DATALEN);
  if(cmsg->DATA == NULL) {
    perror("malloc");
    return 1;
  }
  memset(cmsg->DATA, 0, cmsg->DATALEN);
  recu = recv(sock, cmsg->DATA, cmsg->DATALEN, 0);
  
  // if (recu != cmsg->DATALEN) { nothing here in this context
  //   return send_error(sock, "error DATALEN");
  // }

  printf("**Server notification**: CODEREQ: %d ID: %d NUMFIL: %d :  NB: %d\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->NB);

  // Handle next n messages
  for(int i=0; i<cmsg->NB; i++) {
    process_ticket(sock);
  }

  return 0;
}


int server_notification_abonnement(int sock, client_msg * cmsg) { //Pourquoi
  if (sock < 0)
    return 1;
  uint16_t res;
  int recu = recv(sock, &res, sizeof(uint16_t), 0);
  if (recu <= 0){
    perror("erreur lecture");
    return(1);
  }

  res = ntohs(res);
  cmsg->CODEREQ = (res & 0x001F); // On mask avec 111110000..
  if(cmsg->CODEREQ == 31) return 1; 
  cmsg->ID = (res & 0xFFE0) >> 5;
  recu = recv(sock, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    perror("erreur lecture");
    return 1;
  }
  cmsg->NUMFIL = ntohs(res);
  recu = recv(sock, &res, sizeof(uint16_t), 0);
  
  if (recu <= 0){
    return send_error(sock, "error recv");
  }
  cmsg->NB = ntohs(res);

  char addrmult[17]; // faire nhtons?
  if(recv(sock, &addrmult, sizeof(addrmult), 0)< 0) {
    // handle error
    return 1;
  }
  addrmult[17] = '\0';
  printf("**Server notification**: CODEREQ: %d ID: %d NUMFIL: %d :  NB: %d ADDRM: %s\n", cmsg->CODEREQ, cmsg->ID, cmsg->NUMFIL, cmsg->NB, addrmult);
  return 0;
}

int get_tickets(int sock, int num_fil, int nombre_billets) {
    client_msg msg;
    msg.CODEREQ = 3;
    msg.ID = ID;
    msg.NUMFIL = num_fil;
    msg.NB = nombre_billets;
    msg.DATALEN = 0;
    msg.DATA = "";
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
    msg.DATA = "";
    if (query(sock, &msg) == 0) {
      client_msg msg;
      return server_notification_abonnement(sock, &msg);
    }
    return 1;
}

int main(int argc, char* argv[]) {

    int sock = socket(PF_INET6, SOCK_STREAM, 0);

    struct sockaddr_in6 adrso;
    memset(&adrso, 0, sizeof(adrso)); // On remplit de 0 la structure (important)
    adrso.sin6_family = AF_INET6;
    // On met le port de dict
    adrso.sin6_port = htons(PORT); // Convertir le port en big-endian
    
    inet_pton(AF_INET6, IP_SERVER, &adrso.sin6_addr); // On écrit l'ip dans adrso.sin_addr
    if (connect(sock, (struct sockaddr *) &adrso, sizeof(adrso)) < 0) send_error(sock, "connect failed"); // 0 en cas de succès, -1 sinon
    

    // query_subscription(sock, "Paris");
    
    // send_ticket(sock, 0, "Hello World! This is another one");
    get_tickets(sock, 1, 5);

    // RECEPTION MSG SERVEUR
    // char bufrecv[SIZE_MESS+1];
    // memset(bufrecv, 0, SIZE_MESS+1);
    
    // int n = -1;

    // if ((n = recv(sock, bufrecv, SIZE_MESS * sizeof(char), 0)) < 0) send_error(sock, "recv failed");
    // printf("Taille msg recu: %d\n", n);
    
    // bufrecv[n] = '\0';

    // printf("MSG hexa: ");
    // for(int i=0;i<6;i++) {
    //     printf("%4x ", bufrecv[i]);
    // }
    // puts("");
    // printf("MSG string: %s\n", bufrecv);

    //*** fermeture de la socket ***
    close(sock);
    return EXIT_SUCCESS;
}