#ifndef MYUTILS_H_
#define MYUTILS_H_

#include <stdbool.h>

extern void random_set_seed (unsigned int seed);
extern int random_int_in_range (int min, int max);

extern int xtoi (char *hexString);
extern char* itoa (int i, char b[]);

extern bool system_is_little_endian (void);

/*** STRINGS ***/

extern void copy (char *to, const char *from);
extern char **splitString (char *str, const char delim);
extern char *createString (const char *stringWithFormat, ...);
extern char *reverseString (char *str);

#endif