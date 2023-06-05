# LPDProtocol

## Project Summary
The LPDProtocol project aims to establish a client/server communication protocol called LPDProtocol. Users can connect to the server through a client, engage in discussions, post messages and files, subscribe to threads, and receive notifications.

## Getting Started
To start the project, you need to build it and then launch the server and one (or multiple) clients using the following commands:
```bash
make

# Lancer le serveur
./server           # le serveur se lance sur le port 7777
./server 3333      # Ici, sur le port 3333

# Lancer le client
./client           # IP: '::1', port: 7777
./client 4444      # IP: '::1', port: 4444
./client "fdc7:9dd5:2c66:be86:4849:43ff:fe49:79be" 3333
```

## Réinitialiser la base de données
```bash
make reset_database
```

## Nettoyer les fichiers objets et exécutables
```bash
make clean
```
