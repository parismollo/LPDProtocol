
#include "megaphone.h"

#define SIZE_MESS 1024

#define IP_SERVER "fdc7:9dd5:2c66:be86:4849:43ff:fe49:79bf"
#define PORT 7777

int send_error() {
    return 1;
}

int query(int server_sock, client_msg* msg) {
    char buffer[2];

    buffer[0] = (msg->CODEREQ << 3) | (msg->ID >> 8);
    buffer[1] = msg->ID & 0xFF;

    send(server_sock, buffer, sizeof(buffer), 0);
    // Vérifier erreur.
    
    u_int16_t tmp = htons(msg->NUMFIL);
    send(server_sock, &tmp, sizeof(u_int16_t), 0);

    tmp = htons(msg->NB);
    send(server_sock, &tmp, sizeof(u_int16_t), 0);

    send(server_sock, &msg->DATALEN, sizeof(u_int8_t), 0);

    send(server_sock, msg->DATA, msg->DATALEN, 0);

    return 0;
}

int query_subscription(int server_sock, char* pseudo) {
    uint16_t a = htons(1);

    int ecrit = send(server_sock, &a, sizeof(uint16_t), 0);
    printf("nombre d'octets envoyés: %d\n", ecrit);

    char buffer[10];
    // On remplit le buffer avec des '#'
    memset(buffer, '#', sizeof(buffer));
    memmove(buffer, pseudo, strlen(pseudo));

    ecrit = send(server_sock, buffer, 10, 0);
    if(ecrit <= 0){
        perror("erreur ecriture");
        close(server_sock);
        exit(3);
    }

    printf("nombre d'octets envoyés: %d\n", ecrit);
    
    return 0;
}

int main(int argc, char* argv[]) {

    int sock = socket(PF_INET6, SOCK_STREAM, 0);

    struct sockaddr_in6 adrso;
    memset(&adrso, 0, sizeof(adrso)); // On remplit de 0 la structure (important)
    adrso.sin6_family = AF_INET6;
    // On met le port de dict
    adrso.sin6_port = htons(PORT); // Convertir le port en big-endian
    
    inet_pton(AF_INET6, IP_SERVER, &adrso.sin6_addr); // On écrit l'ip dans adrso.sin_addr
    int r = connect(sock, (struct sockaddr *) &adrso, sizeof(adrso)); // 0 en cas de succès, -1 sinon

    if(r < 0) {
        perror("erreur connect");
        close(sock);
        exit(-1);
    }
    
    query_subscription(sock, "daniel");

    // RECEPTION MSG SERVEUR

    char bufrecv[SIZE_MESS+1];
    memset(bufrecv, 0, SIZE_MESS+1);
    
    int recu = recv(sock, bufrecv, SIZE_MESS * sizeof(char), 0);
    printf("Taille msg recu: %d\n", recu);
    if (recu <= 0){
      perror("erreur lecture");
      close(sock);
      exit(4);
    }
    
    bufrecv[recu] = '\0';

    printf("MSG hexa: ");
    for(int i=0;i<6;i++) {
      printf("%4x ", bufrecv[i]);
    }
    puts("");

    printf("MSG string: %s\n", bufrecv);

    //*** fermeture de la socket ***
    close(sock);

    return EXIT_SUCCESS;
}