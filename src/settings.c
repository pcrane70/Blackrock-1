#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "blackrock.h"

#include "settings.h"

#include "utils/log.h"
#include "utils/config.h"
#include "utils/myUtils.h"

static const char *settingsConfig = "settings.cfg";

Settings *main_settings = NULL;

static Settings *settings_new (void) {

    Settings *settings = (Settings *) malloc (sizeof (Settings));
    return settings;

}

static void settings_destroy (Settings *settings) {

    if (settings) {
        if (settings->resolution) free (settings->resolution);
        if (settings->serverIp) free (settings->serverIp);

        free (settings);
    }

}

static Settings *settings_fill (Config *cfg) {

    Settings *settings = settings_new ();
    if (settings) {
        ConfigEntity *entity = getEntityWithId (cfg, 1);

        char *window = getEntityValue (entity, "window");
        if (window) {
            int window_int = atoi (window);
            window_int ? settings->window = true : false;
            free (window);
        }

        else settings->window = SETTINGS_DEFAULT_WINDOW;

        settings->resolution = getEntityValue (entity, "resolution");

        char *scale = getEntityValue (entity, "scale");
        if (scale) {
            settings->scale = atoi (scale);
            free (scale);
        }

        else settings->scale = SETTINGS_DEFAULT_SCALE;

        char *mute = getEntityValue (entity, "mute");
        if (mute) {
            int mute_int = atoi (mute);
            mute_int ? settings->mute = true : false;
            free (mute);
        }

        else settings->mute = SETTINGS_DEFAULT_MUTE;

        settings->serverIp = getEntityValue (entity, "serverIP");
    }

    return settings;

}

Settings *settings_load (void) {

    Config *config = parseConfigFile (createString ("%s%s", CONFIG_PATH, settingsConfig));
    if (!config) {
        // this could be because we are on a new machine and we do not have settings yet
        // the file is corrupted 
        // error reading the config
        logMsg (stderr, ERROR, NO_TYPE, "Failed to load settings cfg file!");

        // create a config file with our settings
    }

    else {
        Settings *retval = settings_fill (config);
        clearConfig (config);
        return retval;
    } 

}

// TODO:
// write the settings into the save file
u8 settings_save (Settings *settings) {



}