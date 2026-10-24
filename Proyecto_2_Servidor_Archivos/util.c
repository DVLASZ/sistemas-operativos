/**
 * @file
 * @brief Funciones de utilidad
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
*/

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "util.h"


/* Rutina para convertir una cadena en minusculas */
char * lcase(char * s) {
  char * aux;

  aux = s;
  while (*aux != '\0' ) {
    if (isalpha(*aux) && isupper(*aux)) {
      *aux = tolower(*aux);
    }
    aux++;
  }
  return s;
}

char * replace_extension(char* path, char * oldext, char * newext) {

  char * ret;

  //Obtener una copia al reves de la ruta original
  char * rev_path = strrev(strdup(path));

  //Obtener una copia al reves de la extension anterior
  char * rev_oldext = strrev(strdup(oldext));

  if (strstr(rev_path, rev_oldext) == rev_path) {
    //La extension anterior estaba al final de la ruta original
    ret = (char*) malloc(strlen(path) - strlen(oldext) + strlen(newext) + 1);
    strncpy(ret, path, strlen(path) - strlen(oldext));
    strcat(ret, newext);
  }else {
    //La extension anterior no se encontraba al final de la ruta original
    ret = (char*) malloc(strlen(path)  + strlen(newext) + 1);
    sprintf(ret, "%s%s", path, newext);
  }

  return ret;
}

char * remove_extension(char * path) {
  return truncate_to(path, '.', 1);
}

char * concat(char * str1, char * str2) {
  char * str;

  str = (char*)malloc(strlen(str1) + strlen(str2) + 1);
  sprintf(str, "%s%s", str1, str2);

  return str;
}


char * find_first_not_of(char * str, char * set) {
    char * str_ptr;
    const char * set_ptr;

    if (str == NULL) {
	   return NULL;
    }

    str_ptr = str;

    for (str_ptr = str; *str_ptr != 0 && strchr(set, *str_ptr) != NULL; str_ptr++);

    return str_ptr;

}


char * find_first_of(char * str, char * set) {
    char * str_ptr;
    const char * set_ptr;

    if (str == NULL) {
	     return NULL;
    }


    str_ptr = str;

    for (str_ptr = str; *str_ptr != 0 && strchr(set, *str_ptr) == NULL; str_ptr++);

    return str_ptr;

}


char * truncate_to(char * str, char c, int from_end) {

  char * ret;


  char * ptr;

  if (from_end)  {
    ptr = strrchr(str, c);
  }else {
    ptr = strchr(str, c);
  }

  //Si c se encuentra al inicio de la cadena, retornar nulo.
  if (ptr == str) {
    return NULL;
  }

  //Si la ruta no contiene c, tomarla toda.
  if (ptr == NULL) {
    ptr = str + strlen(str);
  }

  ret = (char *)malloc(ptr - str + 1);
  memset(ret, 0, ptr - str + 1 );

  strncpy(ret, str, (ptr - str));

  return ret;

}

#ifndef strrev

char * strrev(char * str) {

	if (str == NULL || *str == 0) {
		return str;
		}

	char * start = str; // Inicio de la cadena 
	char * end = start + strlen(str) - 1; // Final de la cadena

	while (end > start) {
		//Intercambiar los caracteres
		char tmp = *start;
		*start = *end;
		*end = tmp;
		start++; //Avanzar el apuntador de la izquierda
		end--; //Retroceder el apuntador de la derecha
		}

	return str;
}

#endif

int file_exists(char * path) {
	return (access(path, F_OK | R_OK) == 0);
}
