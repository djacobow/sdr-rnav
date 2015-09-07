#include <string.h>
#include <stdlib.h>

/* routine to make concatenated string */
char *create_cat_str(const char *a, const char *b) {
 char *o = malloc(strlen(a) + strlen(b) + 1);
 strcpy(o,a);
 strcat(o,b);
 return o;
}

