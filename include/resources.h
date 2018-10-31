#ifndef RESOURCES_H
#define RESOURCES_H

/*** IMAGES ***/

typedef struct BitmapImage {

    uint32_t *pixels;
    uint32_t width;
    uint32_t height;

} BitmapImage;

extern BitmapImage *loadImageFromFile (char *filename);
extern void destroyImage (BitmapImage *);

#endif