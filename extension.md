# Projet de Messagerie : Utilisation de Fichiers pour Implémenter des Fils de Discussion

## Introduction
Dans le cadre de notre projet, nous avons opté pour une approche originale en implémentant les fils de discussion comme des fichiers. Chaque fil est matérialisé par un dossier distinct, qui comprend un fichier texte pour le stockage des messages et des fichiers supplémentaires pour la gestion du multicast.

## Organisation des Fichiers pour la Gestion des Données
Le stockage des données nécessaires à notre projet s'appuie sur plusieurs fichiers :

- `CLIENT_ID_FILE` (client_id.data) : Ce fichier contient l'ID du client actuel. L'ID est conservé dans ce fichier pour une utilisation ultérieure.
- `DATABASE` (server_users.data) : Ce fichier est consacré à la sauvegarde des informations relatives aux utilisateurs du serveur. Il contient le nom de tous les utilisateurs.
- `INFOS` (infos.data) : Ce fichier comprend des informations générales concernant le serveur, telles que le nombre total de fils de discussion créés jusqu'à présent.

## Gestion des Fils de Discussion et du Multicast
### Configuration des Fils de Discussion
Chaque fil de discussion se matérialise par un dossier nommé "filX", où X représente le numéro du fil. Par exemple, le fil numéro 1 correspond au dossier "fil1", le fil numéro 2 au dossier "fil2", et ainsi de suite. Dans chaque dossier de fil, un fichier de configuration nommé "filX.info" est généré. Ce fichier renferme des informations spécifiques au fil, comme l'auteur initial du fil et la date de création.

### Gestion des Abonnements Multicast
Pour la gestion du multicast, nous avons recours à des fichiers supplémentaires. Lorsqu'un client s'abonne à un fil de discussion, un fichier spécifique à ce fil est généré. Par exemple, si un client s'abonne au fil 1, le fichier "multicast_address_fil_1" est créé. Ce fichier comprend l'adresse multicast associée au fil auquel le client est abonné. Cette adresse est utilisée pour l'émission et la réception des messages multicast au sein du fil.

### Gestion des Adresses des Clients
Lorsqu'un client s'abonne à un ou plusieurs fils de discussion, un fichier appelé "address.data" est généré. Ce fichier conserve les adresses multicast auxquelles le client est abonné, permettant ainsi au client de recevoir les messages multicast des fils auxquels il participe.

## Fonctions pour la Gestion des Fils de Discussion et du Multicast
Plusieurs fonctions ont été développées dans les fichiers `fil.c` et `multicast.c` pour gérer les fils de discussion et les fonctionnalités de multicast. Voici un aperçu des fonctions clés :

### `increase_nb_fils()`
Cette fonction augmente le nombre total de fils en mettant à jour la valeur dans le fichier `INFOS`. Elle est utilisée lors de la création d'un nouveau fil de discussion.

### `subscribe_to_fil(int fil)`
Cette fonction permet à un client de s'abonner à un fil de discussion spécifié. Elle génère le fichier "multicast_address_fil_X" pour

 le fil concerné et y ajoute l'adresse multicast correspondante.

### `unsubscribe_from_fil(int fil)`
Cette fonction permet à un client de se désabonner d'un fil de discussion spécifique. Elle supprime le fichier "multicast_address_fil_X" relatif au fil dont le client se désabonne.

### `load_client_addresses()`
Cette fonction récupère les adresses multicast auxquelles le client est abonné à partir du fichier "address.data" et initialise les connexions multicast correspondantes.

## Fonctionnement de la Gestion des Fils de Discussion et du Multicast
Lors de la création d'un nouveau fil de discussion, nous utilisons la fonction `increase_nb_fils()` pour incrémenter le nombre total de fils enregistrés. Nous créons ensuite un dossier nommé "filX" pour le fil, où nous stockons le fichier de configuration "filX.info" qui conserve les informations spécifiques au fil.

Quand un client s'abonne à un fil de discussion, la fonction `subscribe_to_fil(int fil)` est exécutée. Cette fonction génère le fichier "multicast_address_fil_X" pour le fil spécifié, et l'adresse multicast correspondante est ajoutée à ce fichier. Le client peut alors utiliser cette adresse pour recevoir les messages multicast du fil.

Quand un client se désabonne d'un fil de discussion, la fonction `unsubscribe_from_fil(int fil)` est exécutée. Cette fonction supprime le fichier "multicast_address_fil_X" pour le fil spécifié, ce qui empêche le client de recevoir les messages multicast de ce fil.

Le fichier "address.data" sert à conserver les adresses multicast auxquelles le client est abonné. Ce fichier est généré lorsque le client s'abonne à un fil de discussion et est mis à jour en conséquence. La fonction `load_client_addresses()` récupère les adresses multicast de ce fichier et initialise les connexions multicast correspondantes, permettant ainsi au client de participer aux discussions multicast.

Ainsi, l'organisation des fichiers de configuration des fils, des fichiers d'adresse multicast et du fichier "address.data" participe à la gestion efficace des fils de discussion et des fonctionnalités de multicast dans notre projet de messagerie.
