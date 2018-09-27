#ifndef MYUTILS_H_
#define MYUTILS_H_

extern int randomInt (int min, int max);

extern int xtoi (char *hexString);
extern char* itoa (int i, char b[]);

extern bool system_is_little_endian (void);

/*** STRINGS ***/

extern void copy (char *to, const char *from);
extern char **splitString (char *str, const char delim);
extern char *createString (const char *stringWithFormat, ...);
extern char *reverseString (char *str);

#endif