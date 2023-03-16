#include "megaphone.h"

#define SIZE_MESS 1024
// #define IP_SERVER "fdc7:9dd5:2c66:be86:4849:43ff:fe49:79bf"
#define IP_SERVER "::1"
#define PORT 7777

int send_error(int sock, char* msg) {
    close(sock);
    perror(msg);
    exit(1);
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

int query_subscription(int sock, char* pseudo) {
    uint16_t a = htons(1);

    int ecrit = send(sock, &a, sizeof(uint16_t), 0);
    printf("nombre d'octets envoyés: %d\n", ecrit);

    char buffer[10];
    // On remplit le buffer avec des '#'
    memset(buffer, '#', sizeof(buffer));
    memmove(buffer, pseudo, strlen(pseudo));

    if (send(sock, buffer, 10, 0)< 0) send_error(sock, "send failed");
    // printf("nombre d'octets envoyés: %d\n", ecrit);
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
    if (connect(sock, (struct sockaddr *) &adrso, sizeof(adrso)) < 0) send_error(sock, "connect failed"); // 0 en cas de succès, -1 sinon
    
    query_subscription(sock, "daniel");

    // RECEPTION MSG SERVEUR
    char bufrecv[SIZE_MESS+1];
    memset(bufrecv, 0, SIZE_MESS+1);
    
    int n = -1;
    if ((n = recv(sock, bufrecv, SIZE_MESS * sizeof(char), 0)) < 0) send_error(sock, "recv failed");
    printf("Taille msg recu: %d\n", n);
    
    bufrecv[n] = '\0';

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