#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

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

char **splitString (char *str, const char delim) {

    char **result = 0;
    size_t count = 0;
    char *temp = str;
    char *last = 0;
    char dlm[2];
    dlm[0] = delim;
    dlm[1] = 0;

    // count how many elements will be extracted
    while (*temp) {
        if (delim == *temp) {
            count++;
            last = temp;
        }

        temp++;
    }

    count += last < (str + strlen (str) - 1);

    count++;

    result = (char **) calloc (count, sizeof (char *));

    if (result) {
        size_t idx = 0;
        char *token = strtok (str, dlm);

        while (token) {
            assert (idx < count);
            *(result + idx++) = strdup (token);
            token = strtok (0, dlm);
        }

        assert (idx == count - 1);
        *(result + idx) = 0;
    }

    return result;

}


bool system_is_little_endian (void) {

    unsigned int x = 0x76543210;
    char *c = (char *) &x;
    if (*c == 0x10) return true;
    else return false;

}