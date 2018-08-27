#ifndef RESOURCES_H
#define RESOURCES_H

#include "blackrock.h"

typedef struct BitmapImage {

    u32 *pixels;
    u32 width;
    u32 height;

} BitmapImage;

extern BitmapImage *loadImageFromFile (char *filename);

#endif