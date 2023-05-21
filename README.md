# megaphone_pr6

## Résumé du projet
Le projet megaphone consiste à mettre en place un protocole de communication client/serveur appelé Mégaphone. Les utilisateurs peuvent se connecter au serveur via un client, participer à des fils de discussion, poster des messages et fichiers, s'abonner à des fils et recevoir des notifications.

## Lancer le projet
Vous devez construire le projet, puis lancer le serveur et un (ou plusieurs) clients à l'aide des commandes suivantes :
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