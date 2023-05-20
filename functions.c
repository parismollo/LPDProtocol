#include "megaphone.h"

// Vérifie si pre est bien préfix de str
int prefix(char* str, char* pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void clear_pseudo(char * pseudo){
    char* pos_hstg = strchr(pseudo, '#');
    if(pos_hstg == NULL)
      return;
    size_t length_cleared = pos_hstg - pseudo;
    pseudo[length_cleared] = '\0';
}

// maxsize vaut exactement la taille allouée à str
void replace_after(char* str, char target, char replace_char, size_t maxsize) {
  int found = 0;
  for(int i=0;i<maxsize-1;i++) {
    if(str[i] == target)
      found = 1;
    if(found)
      str[i] = replace_char;
  }
  str[maxsize-1] = '\0';
}


////////////////////////////////////
// TELECHARGEMENT/ENVOIE FICHIERS //
////////////////////////////////////

// Fonction pour insérer un nouveau packet dans la liste chaînée triée
void insert_packet_sorted(Node** head, FilePacket packet) {
  Node* new_node = malloc(sizeof(Node));
  if(new_node == NULL) {
    perror("error malloc");
    return;
  }
  new_node->packet = packet;
  new_node->next = NULL;
  // Il n'y a pas de premier alors packet devient le head
  // Sinon, si son numero est plus petit que le premier il devient egalement le head
  if (*head == NULL || packet.num_bloc < (*head)->packet.num_bloc) {
    new_node->next = *head;
    *head = new_node;
  }
  else { // Si >= a numero de head. On se deplace pour l'inserer au bon endroit (voir tout a la fin)
    Node* current = *head;
    while (current->next != NULL && current->next->packet.num_bloc < packet.num_bloc) {
      current = current->next;
    }
    new_node->next = current->next;
    current->next = new_node;
  }
}

// Fonction pour parcourir la liste chaînée triée et écrire les données dans le fichier
int write_packets_to_file(Node* head, char* file_path) {
  // Écrire les données dans le fichier dans le bon ordre
  FILE* file = fopen(file_path, "w");
  if(file == NULL) {
    fprintf(stderr, "error file creation\n");
    return -1;
  }

  printf("ECRITURE DES PAQUETS DANS LE FICHIER: %s\n", file_path);
  while(head != NULL) {
    fwrite(head->packet.data, 1, strlen(head->packet.data), file);
    head = head->next;
  }
  // On ferme le fichier
  fclose(file);
  return 0;
}

void free_list(Node* head) {
  while (head != NULL) {
    Node* current = head;
    head = head->next;
    free(current);
  }
}

int send_file(struct sockaddr_in6 addr, client_msg msg, char* file_path) {
  // On crée le socket UDP
  int sock_udp = socket(PF_INET6, SOCK_DGRAM, 0);
  if (sock_udp < 0) {
    fprintf(stderr, "Error creation sock udp");
    return -1;
  }

  FILE* file = fopen(file_path, "r");
  if(file == NULL) {
    close(sock_udp);
    fprintf(stderr, "error open file");
    return -1;
  }

  FilePacket packet;
  memset(&packet, 0, sizeof(packet));
  socklen_t len = sizeof(addr);

  // Combine le codereq (5 bits de poids faible) avec l'ID (11 bits restants)
  packet.codreq_id = ((uint16_t)msg.CODEREQ) | (msg.ID << 5);
  packet.codreq_id = htons(packet.codreq_id);

  uint16_t num_bloc = 1;
  int nb_read;
  printf("BEGIN TRANSMISSION FILE\n");
  do {
    memset(packet.data, 0, 513);
    nb_read = fread(packet.data, sizeof(char), 512, file); // on lit 512 octets dans le fichier
    packet.num_bloc = htons(num_bloc);

    printf("SEND PACKET, SIZE: %d\n", nb_read);
    if(sendto(sock_udp, &packet, sizeof(packet), 0, (struct sockaddr *)&addr, len) < 0) {
      close(sock_udp);
      fprintf(stderr, "send failed");
      return -1;
    }
    num_bloc++;
    // Permet de ralentir l'envoie. Car celui qui recoit doit trier les paquets et ça prend du temps
    usleep(200); // Il faut en changer en fonction de la taille du fichier par exemple
    // Attention les printf dans le serveur et le client peuvent ralentir beaucoup !
  } while(nb_read == 512);
  printf("END TRANSMISSION FILE\n");
  
  fclose(file);
  close(sock_udp);
  return 0;
}


// Permet de recevoir un fichier en UDP
Node* download_file(int UDP_port, int ID, int CODREQ) {
  int sock_udp = socket(PF_INET6, SOCK_DGRAM, 0);
  if (sock_udp < 0) {
    perror("error creation socket udp");
    return NULL;
  }
  
  // On met notre port UDP
  struct sockaddr_in6 address_sock;
  memset(&address_sock, 0, sizeof(address_sock));
  address_sock.sin6_family = AF_INET6;
  address_sock.sin6_port = UDP_port; // Pas de htons ici pour bind !
  address_sock.sin6_addr = in6addr_any;

  printf("PORT UDP : %d\n", address_sock.sin6_port);

  // Pour avoir une socket polymorphe : 
  int no = 0;
  int r = setsockopt(sock_udp, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
  if(r < 0) {
    fprintf(stderr, "Echec de setsockopt() : (%d)\n", errno);
    close(sock_udp);
    return NULL;
  }
  
  int yes = 1;
  r = setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if(r < 0) {
    fprintf(stderr, "Echec de setsockopt() : (%d)\n", errno);
    close(sock_udp);
    return NULL;
  }

  if (bind(sock_udp, (struct sockaddr *)&address_sock, sizeof(address_sock)) < 0) {
    perror("Error bind");
    close(sock_udp);
    return NULL;
  }

  struct sockaddr_in6 cliadr;
  socklen_t len = sizeof(cliadr);

  Node* packets_list = NULL;
  FilePacket packet;
  uint8_t codreq;
  uint16_t id;
  // On reçoit les packets du client et on les stocke dans une liste chainée
  printf("**RECEPTION D'UN FICHIER**\n");
  do {
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(sock_udp, &rfd);

    // On attend maximum 5s qu'un message arrive
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ret_select = select(sock_udp + 1, &rfd, NULL, NULL, &timeout);
    
    if(ret_select < 0) {
      perror("error select");
      break;
    }
    else if(ret_select == 0) {
      fprintf(stderr, "Timeout select reception fichier\n");
      break;
    }

    int num_bytes = 0;
    // Si select indique qu'on peut lire sur la socket
    if (FD_ISSET(sock_udp, &rfd)) {
      memset(&packet, 0, sizeof(packet));
      num_bytes = recvfrom(sock_udp, &packet, sizeof(packet), 0, (struct sockaddr *)&cliadr, &len);
      
      if(num_bytes < 0) {
        fprintf(stderr, "erreur recvfrom");
        break;
      }
      else if(num_bytes == 0) {
        // Fin de la transmission
        break;
      }

      packet.codreq_id = ntohs(packet.codreq_id);
      packet.num_bloc = ntohs(packet.num_bloc);

      codreq = (packet.codreq_id & 0x001F); // On mask avec 111110000..
      id = (packet.codreq_id & 0xFFE0) >> 5;

      // On skip les paquets qui ne viennent pas du bon destinataire
      // Ou qui sont mal formatés
      if(codreq != CODREQ || id != ID) {
        printf("Bad id or bad codreq. Skipping this packet...\n");
        continue;
      }

      printf("**FilePacket** codreq_id:%d, num_bloc: %d et strlen(data): %ld\n", packet.codreq_id, packet.num_bloc, strlen(packet.data));

      // On ajoute ce packet a la liste triée
      insert_packet_sorted(&packets_list, packet);
    }
    else {
      printf("socket pas prête en lecture. On attend...\n");
      continue;
    }

  } while(packet.data[511] != 0); // Tant que on recoit 512 octets de texte

  close(sock_udp);

  printf("**FICHIER RECU**\n");

  return packets_list;
}

void print_error(char * message) {
  fprintf(stderr, "\033[32m[Server Log]:\033[0m \033[31m%s\n\033[0m", message);
}