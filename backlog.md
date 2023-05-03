## TODO
- [ ] Vérifier si l'option qui renvoie une liste de billets fonctionne **CLIENT/SERVEUR** sur lulu
- [ ] download_file (client): fonction pour télécharger un fichier provenant d'un fil sur le server **CLIENT**
- [ ] send_file (SERVEUR): fonction pour envoyer le fichier (correspondant a un certain fil) demandé au client **SERVEUR**

## Doing
**Leopold**:
- [ ] send_file (client): fonction pour envoyer un fichier à un fil au serveur **CLIENT**
- [ ] recv_file (SERVEUR): fonction pour récupérer un fichier envoyer par le client puis le stocker correspondant au fil sur le server **SERVEUR**

**Paris**:

**Daniel**:
- [x] Ajouter tout teste de verification avant valider msg du client (si codereq ok, si id existe, etc...)
- [ ] faire feature "abonner fils" **SERVER**


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