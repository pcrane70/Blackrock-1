#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_DEFAULT_WINDOW     1
#define SETTINGS_DEFAULT_SCALE      1
#define SETTINGS_DEFAULT_MUTE       0

typedef struct Settings {

    bool window;
    char *resolution;
    int scale;
    bool mute;
    char *serverIp;

} Settings;

extern Settings *main_settings;

extern Settings *settings_load (void);

#endif