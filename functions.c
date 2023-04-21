#include "megaphone.h"

// Vérifie si pre est bien préfix de str
int prefix(char* str, char* pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void clear_pseudo(char * pseudo){
    char* pos_hstg = strchr(pseudo, '#');
    if(pos_hstg == NULL)
      return;
    size_t length_cleared = pos_hstg - pseudo;
    pseudo[length_cleared] = '\0';
}

// maxsize vaut exactement la taille allouée à str
void replace_after(char* str, char target, char replace_char, size_t maxsize) {
  int found = 0;
  for(int i=0;i<maxsize-1;i++) {
    if(str[i] == target)
      found = 1;
    if(found)
      str[i] = replace_char;
  }
  str[maxsize-1] = '\0';
}