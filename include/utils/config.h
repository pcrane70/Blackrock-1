#ifndef CONFIG_H
#define CONFIG_H

#include "blackrock.h"

#include "utils/dlist.h"

#define CONFIG_MAX_LINE_LEN     128

typedef struct ConfigKeyValuePair {

    char *key;
    char *value;

} ConfigKeyValuePair;

typedef struct ConfigEntity {

    char *name;
    DoubleList *keyValuePairs;

} ConfigEntity;

typedef struct Config {

    DoubleList *entities;

} Config;


extern Config *parseConfigFile (char *filename);
extern char *getEntityValue (ConfigEntity *entity, char *key);
extern ConfigEntity *getEntityWithId (Config *cfg, u8 id);

extern void setEntityValue (ConfigEntity *entity, char *key, char *value);
extern void writeConfigFile (const char *filename, Config *config);

extern void clearConfig (Config *);


#endif
