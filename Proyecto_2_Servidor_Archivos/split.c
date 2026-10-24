/**
 * @file
 * @brief  String split library
 * @author Erwin Meza Vega <emezav@gmail.com>
*/

#include <string.h>
#include <stdlib.h>

#include "split.h"
#include "util.h"

/** @brief Delimitadores por defecto */
char * split_default_delimiters = " \t\r\n";

/**
 * @brief Delimitadores por defecto
 * @return Delimitadores por defecto
 * */
char * default_delimiters();

split_list * split(char * str, char * delim) {
    char * str_ptr;
    char * pos;
    int i;
    int count;

    split_list * ret;

     if (delim == NULL || strlen(delim) == 0) {
    	delim = default_delimiters();
    }

    ret = (split_list *) malloc(sizeof(split_list));

    ret->count = 0;
    for (i=0; i< MAX_PARTS; i++) {
    	ret->parts[i] = NULL;
    }

    i = 0;
    str_ptr = find_first_not_of(str, delim);
    while (str_ptr != 0 && *str_ptr != 0) {
	      pos = find_first_of(str_ptr, delim);

      	if (pos == NULL) {
      	    break;
      	}

      	count = pos - str_ptr;

      	ret->parts[i] = (char*)malloc(count + 1);
      	memset(ret->parts[i], 0, count + 1);
      	strncpy(ret->parts[i], str_ptr, count);
      	i++;
      	str_ptr = find_first_not_of(pos, delim);
    }

    ret->count = i;

    return ret;
}


char * default_delimiters() {
    return split_default_delimiters;
}

void free_split_list(split_list * l) {
    //Validar apuntador invalido
    if (l == 0) {
        return;
    }

    //Si la lista no se encuentra vacia, liberar primero las subcadenas
    for (int i = 0; i < l->count; i++) {
        free(l->parts[i]);
    }

    //Liberar la memoria de la estructura
    free(l);
}
