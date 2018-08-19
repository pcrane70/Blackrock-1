#ifndef CONFIG_H
#define CONFIG_H

#include "blackrock.h"

#define CONFIG_MAX_LINE_LEN     128

typedef struct ConfigKeyValuePair {

    char *key;
    char *value;

} ConfigKeyValuePair;

typedef struct ConfigEntity {

    char *name;
    List *keyValuePairs;

} ConfigEntity;

typedef struct Config {

    List *entities;

} Config;


extern Config *parseConfigFile (char *filename);
extern char *getEntityValue (ConfigEntity *entity, char *key);
extern ConfigEntity *getEntityWithId (Config *cfg, u8 id);


#endif
