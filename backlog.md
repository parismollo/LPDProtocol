- [ ] Fonction change_infos(key, ..): modifie la valeur pour la clée key dans le fichier info.data (MACRO: INFOS)

**Leopold**:
- [ ] Demander la liste des **n** derniers billets (**SERVEUR**)

**Paris**:
- [ ] gérer reception de la requete: abonner au fils **CLIENT**

**Daniel**:
- [x] RELIRE LE CODE AVEC GTP POUR BIEN COMPRENDRE
- [x] verify write and read (handle error)  --> juste voire la fct et faire perror, return ect en fct du type mais pas exit, dmder gtp
- [ ] Vérifier que le client fonctionne pour poster un billet sur le serveur de la prof --> copié file sur lulu et lancer 2-3 fois en regardant reponse et file crée et comparer avec notre serveur
- [ ] Ajouter tout teste de verification avant valider msg du client (si codereq ok, si id existe, etc...)
- [x] faire serveur qui accepte ipv6 et ipv4


---

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



