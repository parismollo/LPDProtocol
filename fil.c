#include "megaphone.h"

/* Ce fichier contient toutes les fonctions qui permettent
   de gérer les fils + statistiques (nombre total de msg etc..).
   C'est grâce à ces fonctions qu'on peut intéragir avec les fils. */



// Parcourt tous les fils et additionne le
// nombre de message
int total_msg_fils(uint8_t nb_msg_by_fil) {
  // nb total de msg ds ts les fils
  // Appel nb_fils()
  // Pour chaque fil, sum += nb_msg(i)
  int n = nb_fils();
  int sum = 0;
  for(int i=1;i<=n;i++) {
    int nb_msg = nb_msg_fil(i);
    if(nb_msg_by_fil > nb_msg || nb_msg_by_fil == 0)
      sum += nb_msg;
    else
      sum += nb_msg_by_fil;
  }
  return sum;
}

// Récupère le nombre de messages dans un fil
// Le nombre est écrit sur la derniere ligne sur fichier du filX.txt
int nb_msg_fil(int fil) {
  char buf[100] = {0};
  sprintf(buf, "fil%d/fil%d.txt", fil, fil);
  int fd = open(buf, O_RDONLY, 0666);
  if(fd < 0) {
    return 0;
  }
  goto_last_line(fd);
  memset(buf, 0, 100);
  int n = read(fd, buf, 100);
  if(n<0){
    perror("read in nb_msg_fil");
    close(fd);
    return 0;
  }
  close(fd);

  if(n <= 0)
    return 0;

  return atoi(buf);
}

// Permet de récupérer le nom du createur d'un fil
// On ouvre le fichier du fil et on récupère l'auteur du premier message
int get_fil_initiator(int fil, char* initiator, size_t buf_size) {
  char buffer[1024], *ptr;
  memset(buffer, 0, 1024);
  sprintf(buffer, "fil%d/fil%d.txt", fil, fil);
  FILE* file = fopen(buffer, "r");
  if(file == NULL) {
    // perror("impossible d'ouvrir le fil"); // impossible d'ouvrir le fil
    return 1;
  }
  if(fgets(buffer, 1024, file) != NULL) {
    if((ptr = strstr(buffer, "PSEUDO:")) != NULL) {
      ptr += strlen("PSEUDO:");
      strncpy(initiator, ptr, buf_size);
      fclose(file);
      return 0;
    }
  }
  fclose(file);
  return 1;
}

// Verifie si un utilisateur est inscrit dans la base de donnée
int is_user_registered(int id) {
  FILE* f = fopen(DATABASE, "r");
  if(f < 0) {
    perror("open in is_user_registered");
    return 0;
  }

  char buffer[128];
  memset(buffer, 0, sizeof(buffer));

  while(fgets(buffer, sizeof(buffer), f) > 0) {
    int current_id = atoi(strtok(buffer, " "));
    if(current_id == id) {
      fclose(f);
      return 1;
    }
    memset(buffer, 0, sizeof(buffer));
  }

  fclose(f);
  return 0;
}

// Permet de modifier un fichier avec des lignes
// key;value <-- pour une clée key on écrit une nouvelle valeur dans le fichier
int change_infos(char* key, char* new_value) {
  char buffer[100];
  
  FILE * source = fopen(INFOS, "r");
  // On ne vérifie pas si source est nulle (c'est normal)

  FILE * dest = fopen("temp", "w");
  if (dest == NULL) {
    print_error("change_infos failed");
    return 1;
  }

  int found = 0;
  while(source && fgets(buffer, sizeof(buffer), source)) {
    if(strstr(buffer, key)) {
      found = 1;
      fprintf(dest, "%s;%s", key, new_value);
    } else {
      fprintf(dest, "%s", buffer);
    }
  }

  if(!found) {
    fprintf(dest, "%s;%s", key, new_value);
  }
  
  if(source)
    fclose(source);
  fclose(dest);

  remove(INFOS);
  rename("temp", INFOS);

  return 0;
}

int get_last_messages(int nb, int fil, message* messages) {
  // Open fil
  // get nb last msg
  // store them (char** array)

  char buf[100];
  sprintf(buf, "fil%d/fil%d.txt", fil, fil);

  char line[1024];

  int fd = open(buf, O_RDONLY, 0666);
  
  if(fd < 0) {
    perror("open file");
    return -1;
  }

  long file_size = lseek(fd, 0, SEEK_END);
    
  long num_lines = 0, current_pos;
  int skip_last_line = 1; // Permet de skip la derniere ligne
  for (current_pos = file_size - 1; current_pos >= 0; current_pos--) {
    lseek(fd, current_pos, SEEK_SET);
    if(read(fd, buf, 1) < 0){
      perror("read in get_last_message");
      close(fd);
      return -1;
    }
    if(*buf == '\n') {
      // On skip la derniere ligne (soit la premiere ligne rencontrée en partant de la fin)
      if(skip_last_line) {
        skip_last_line = 0;
        continue;
      }
      num_lines++;
    }
    if(num_lines >= nb * 2)
      break;
  }

  if(current_pos == -1) {
    // On a lu le fichier entierement. 1ere ligne comprise
    // On doit donc ajouter cette 1ere ligne au compteur
    lseek(fd, 0, SEEK_SET); // Important.
    num_lines++;
  }

  if(num_lines < nb * 2) {
    fprintf(stderr, "\033[31mthere is less than %d msg\033[0m\n", nb);
    close(fd);
    return -1;
  }

  char* ptr;
  char* sep = ": \n";
  int j;
  for(int i=0;i<nb;i++) {
    memset(&messages[i], 0, sizeof(message));
    // On lit l'ID
    readline(fd, line, 1024); // GESTION ERREURS
    if(!prefix(line, "ID:")) {
      goto bad_file_format;
    }
    // On découpe la ligne en: [ID, 123, PSEUDO, leo]
    if((ptr = strtok(line, sep)) == NULL)
      goto bad_file_format;
      
    for(j=1;j<4 && (ptr = strtok(NULL, sep)) != NULL;j++) {
      if(j == 1) // ID
        messages[i].ID = atoi(ptr);
      else if(j == 3) // PSEUDO
        memmove(messages[i].pseudo, ptr, strlen(ptr));
    }
    if(j != 4)
      goto bad_file_format;

    
    // On lit le message
    readline(fd, line, 1024); // GESTION ERREURS
    if(!prefix(line, "DATA:"))
      goto bad_file_format;
    memmove(messages[i].text, line + 6, 255); // strlen("DATA: ") = 6
  }

  close(fd);
  return 0;

  bad_file_format:
    if(fd)
      close(fd);
    print_error("Bad file fil format");
    return -1;
}

// Enregistre le pseudo dans la variable pseudo
int get_pseudo(int id, char* pseudo, size_t pseudo_size) {
  FILE* f = fopen(DATABASE, "r");
  if(f == NULL)
    return 1;
  
  char buf[1024];
  memset(buf, 0, 1024);
  char* ptr, *sep = " \n";
  while(fgets(buf, 1024, f) != NULL) {
    ptr = strtok(buf, sep);
    if(ptr == NULL)
      return 1;
    
    if(atoi(ptr) == id) {
      
      ptr = strtok(NULL, sep);
      if(ptr == NULL)
        return 1;
      memset(pseudo, 0, 11);
      int min = strlen(ptr);
      if(min > 10)
        min = 10;
      memmove(pseudo, ptr, min);
      break;
    }
    memset(buf, 0, 1024);
  }
  fclose(f);

  return 0;
}

int increase_nb_fils() {
  int nb = nb_fils();
  char buffer[100];
  sprintf(buffer, "%d", nb+1);
  return change_infos("nb_fils", buffer);
}

int nb_fils() {
  char number[100];
  if(get_infos("nb_fils", number, 100) < 0) {
    // Le fichier n'existe pas ou alors la ligne
    // avec nb_fils n'existe pas donc on considère
    // qu'il n'y aucun fil
    return 0;
  }

  return atoi(number);
}

// On lui passe une clée
// Et la valeur sera enregistrée dans le pointeur value
int get_infos(char* key, char* value, size_t val_size) {
  FILE* file = fopen(INFOS, "r");
  if(file == NULL) {
    return -1;
  }
  char line[1024];
  char* delim = ";\n";
  while(fgets(line, 1024, file) != NULL) {
    char* ptr;
    if((ptr = strtok(line, delim)) != NULL) {
      if(strcmp(ptr, key) == 0) {
        // La clée correspond. On récupère la valeur
        ptr = strtok(NULL, delim);
        if(ptr == NULL) {
          // Le fichier est mal formé
          fclose(file);
          return -1;
        }
        strncpy(value, ptr, val_size);
        fclose(file);
        return 0;
      }
    }
  }
  fclose(file);
  return -1;
}