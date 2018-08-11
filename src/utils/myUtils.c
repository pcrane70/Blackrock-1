#include <stdlib.h>

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