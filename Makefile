CC = gcc
CFLAGS = -Wall -pthread

# Fichiers sources pour le client et le serveur
CLIENT_SRC = client.c fil.c functions.c
SERVER_SRC = server.c fil.c functions.c

# Fichiers objets pour le client et le serveur
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

all: client server

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(CLIENT_OBJ) $(SERVER_OBJ) client server

# Règle pour réinitialiser la base de données
reset_database:
	$(call confirm)
	rm -rf client_id.data server_users.data infos.data address.data fil[0-9]*