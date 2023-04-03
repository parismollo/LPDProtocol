- [x] fonction getPseudo(int id): recupère le pseudo dans le fichier d'inscription
- [ ] fonction nb_fils: compte tous les dossier commençant par fil
- [ ] Fonction getInitator: renvoie le createur du fil (premier id)
- [ ] fonction nb_msg_fil: Mettre le nombre de msg dans un fil a la fin du fichier ? Ou alors fonction qui ouvre le fichier et compte tous les msg
- [ ] repondre au client après avoir poster un billet (regler cas f=0)
- [ ] gérer reception des tickets **CLIENT**
- [ ] gérer reception des abonnés fils

**Leopold**:
- [ ] Demander la liste des **n** derniers billets (**SERVEUR**)

**Paris**:


**Daniel**:
- [ ] Vérifier que le client fonctionne pour poster un billet sur le serveur de la prof
- [ ] faire serveur qui accepte ipv6 et ipv4
- [ ] Ajouter tout teste de verification avant valider msg du client (si codereq ok, si id existe, etc...)
- [ ] verify write and read (handle error)

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