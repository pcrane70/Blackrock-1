#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

// TODO: is there a better way of generating random ints??
unsigned int randomInt (unsigned int min, unsigned int max) {

    unsigned int low = 0, high = 0;

    if (min < max) {
        low = min;
        high = max + 1;
    }

    else {
        low = max + 1;
        high = min;
    }

    return (rand () % (high - low)) + low;

}

// convert a string representing a hex to a string
int xtoi (char *hexString) {

    int i = 0;

    if ((*hexString == '0') && (*(hexString + 1) == 'x')) hexString += 2;

    while (*hexString) {
        char c = toupper (*hexString++);
        if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A'))) break;
        c -= '0';
        if (c > 9) c-= 7;
        i = (i << 4) + c;
    }

    return i;

}

void copy (char *to, const char *from) {

    while (*from)
        *to++ = *from++;

    *to = '\0';

}


bool system_is_little_endian (void) {

    unsigned int x = 0x76543210;
    char *c = (char *) &x;
    if (*c == 0x10) return true;
    else return false;

}