## TODO
- [ ] Vérifier si l'option qui renvoie une liste de billets fonctionne **CLIENT/SERVEUR** sur lulu
- [ ] Fix CODEREQ too large. Context: When user tries to subscribe but is already, CODEREQ too large error is raised in the server side.
- [ ] Bug - if more than one user creates an account (inscription) for some reason it does not work
- [ ] Bug - affichage pas bon dans list tickets (--> �)
- [ ] Organiser le code (si possible). Créer un dossier server et client. Mettre server.c dans server et client.c dans client. Decouper le code en plusieurs fichiers. Modifier le Makefile en conséquence

```bash
paris@paris-pc:~/Documents/UniversiteParis/L3S6/PR6/megaphone_pr6$ ./client 
Megaphone says: Hi user! What do you want to do?
(1) Inscription
(2) Poster billet
(3) Get billets
(4) Abonner au fil
(5) Close connection
3
Megaphone says: Type the numfil and the number of messages (e.g. 1 2) 
1 0
**Server notification**: CODEREQ: 3 ID: 1 NUMFIL: 1 :  NB: 1 
**Server Notification**: NUMFIL: 1 ORIGIN: parismollo�parismollo PSEUDO: parismollo DATA: Hello World 
paris@paris-pc:~/Documents/UniversiteParis/L3S6/PR6/megaphone_pr6$ 

```
- [ ] download_file (client): fonction pour télécharger un fichier provenant d'un fil sur le server. (SIMILAIRE A recv_file de server.c) **CLIENT**
- [ ] send_file (SERVEUR): fonction pour envoyer le fichier (correspondant a un certain fil) demandé au client (SIMILAIRE A send_file de client.c) **SERVEUR**
- [ ] Read fil contents and send over in multicast messages
- [ ] 

## Doing
**Leopold**:
- [x] send_file (client): fonction pour envoyer un fichier à un fil au serveur **CLIENT**
- [x] recv_file (SERVEUR): fonction pour récupérer un fichier envoyer par le client puis le stocker correspondant au fil sur le server **SERVEUR**
- [ ] le serveur ne doit pas attendre indéfiniment le transfert. S’il ne reçoit rien après quelques secondes, il termine son attente et le transfert est annulé. **SERVEUR**
- [ ] Port UDP ? Verifier que ce port n'est pas deja utilisé ailleurs ?
- [ ] IPV6 pour UDP ? Mais on verifie pas si le client est IPV4 ?
- [ ] Attention. Au moment du stockage il faut recuperer le nom du fichier pas le chemin absolu. Par exemple avec fil6/file6.txt vers fil5 ca crash

**Paris**:



**Daniel**:



## Done

- [x] Recevoir un billet (creation de fil si nécessaire, check id, bla bla)
- [x] Client verifie si id existe et stocke dans une variable globale
- [x] return id to client
- [x] client doit savoir son id - doit retenir son id.
- [x] makefile
- [x] enlever # du pseudo
- [x] Bug pour écrire dans file users.data (e.g. 9 -> 10 ou 99 -> 100) voir options.
- [x] query client regler les bits
- [x] send error
- [x] faire recv pour inscription
- [x] tricks pour reset port
- [x] cote serveur: generate id
- [x] store id in file
- [x] Demander la liste des **n** derniers billets (**CLIENT**)
- [x] Abonner au fil (**CLIENT**)
- [x] bug segmentation false à l'inscription
- [x] gérer reception des tickets **CLIENT**
- [x] repondre au client après avoir poster un billet
- [x] **server** : gérer cas f=0 pour handle_ticket
- [x] gérer reception de la requete: demander n derniers billets **CLIENT**
- [x] handle error message from server codereq == 31 
- [x] faire serveur qui accepte ipv6 et ipv4
- [x] gérer reception de la requete: abonner au fils **CLIENT**
- [x] Fonction total_msg_fils: utilise nb_fil(). Puis pour chaque fil, on appelle nb_msg_fil(fil) pour tout additionner
- [x] Fonction change_infos(key, ..): modifie la valeur pour la clée key dans le fichier info.data (MACRO: INFOS). Comme dans getinfo, fgets pour avancer ligne par ligne, et faire strtok sur virgule, deuxième truc c'est la valeur. 
- [x] Demander la liste des **n** derniers billets (**SERVEUR**).
- [x] Vérifier: On a une fonction recv_client_msg dans serveur. Qui fait un recv de DATALEN et DATA. Même quand on a pas besoin ?
- [x] server main
  - [x] accept multiple clients
- [x] client main
  - [x] ask user what he wants to do.
- [x] Fix minor bugs on validate_exec
- [x] Ajouter tout teste de verification avant valider msg du client (si codereq ok, si id existe, etc...)
- [x] Changer DATA dans la structure client_msg. Mettre char DATA[256]. Modifier tout le code en conséquence
- [x] Multicast implementation
  - [x] Server assign address to fil, save it locally and send it back to client
  - [x] Client save address locally
  - [x] Server send message in multicast address
  - [x] Client reads message in multicast address
