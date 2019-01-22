#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_DEFAULT_WINDOW     1
#define SETTINGS_DEFAULT_SCALE      1
#define SETTINGS_DEFAULT_MUTE       0

#include "blackrock.h"

typedef struct Resolution {

    u32 width, height;

} Resolution;

typedef struct Settings {

    bool window;
    Resolution resolution;
    int scale;
    bool mute;
    char *serverIp;

} Settings;

extern Settings *main_settings;

extern Settings *settings_load (void);

#endif