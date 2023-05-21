# megaphone_pr6

Résumé du projet :
Le projet consiste à mettre en place un protocole de communication client/serveur appelé Mégaphone. Les utilisateurs peuvent se connecter au serveur via un client, participer à des fils de discussion, poster des messages et fichiers, s'abonner à des fils et recevoir des notifications.

Pour lancer le projet :

Exécutez la commande "make" dans le répertoire contenant le fichier Makefile pour compiler les fichiers source du client et du serveur.
Une fois la compilation terminée, vous obtiendrez les exécutables "client" et "server".
Lancez le serveur en exécutant la commande "./server" dans votre terminal.
Lancez un ou plusieurs clients en exécutant la commande "./client" dans des terminaux séparés.


Pour réinitialiser la base de données :
Utilisez la commande "make reset_database" pour supprimer les fichiers de données de la base de données.

Pour nettoyer les fichiers objets et exécutables :
Utilisez la commande "make clean" pour supprimer les fichiers objets et les exécutables du client et du serveur.