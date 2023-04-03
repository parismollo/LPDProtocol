- [ ] Mettre le nombre de msg dans un fil au debut du fichier ?
- [ ] Mettre le pseudo du createur au debut du fichier ou son ID ?

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