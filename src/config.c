/*** This file is used to read .cfg files to retrive data for our items, monsters, etc. 
 * 
 * As of 15/08/2018 -- 23:02 -- we will be reading from hardcoded internal files to retriev some
 * information of our various entities. Maybe later we will want to have a more advanced system
 * and even have the server with this type of data.
 * 
 * We also have to think of a better way for adding new stuff in our game in an easier way.
 * We will also want to have more values to tweak by config files.
 * 
 * ***/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

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


/*** PARSE THE FILE ***/

ConfigEntity *newEntity (char *buffer, Config *cfg) {

    // so grab the name
    char *name = strtok (buffer, "[]");

    ConfigEntity *entity = (ConfigEntity *) malloc (sizeof (ConfigEntity));
    char *copy = (char *) calloc (strlen (name) + 1, sizeof (char));
    strcpy (copy, name);
    entity->name = copy;
    entity->keyValuePairs = initList (free);
    insertAfter (cfg->entities, LIST_END (cfg->entities), entity);
    
    return entity;

}

void getData (char *buffer, ConfigEntity *currentEntity) {

    buffer[CONFIG_MAX_LINE_LEN - 1] = '\0';
    char *key = strtok (buffer, "=");
    char *value = strtok (NULL, "\n");
    if ((key != NULL) && (value != NULL)) {
        ConfigKeyValuePair *kvp = (ConfigKeyValuePair *) malloc (sizeof (ConfigKeyValuePair));
        char *copyKey = (char *) calloc (strlen (key) + 1, sizeof (char));
        strcpy (copyKey, key);
        kvp->key = copyKey;
        char *copyValue = (char *) calloc (strlen (value) + 1, sizeof (char));
        strcpy (copyValue, value);
        kvp->value = copyValue;

        insertAfter (currentEntity->keyValuePairs, LIST_END (currentEntity->keyValuePairs), kvp);
    }

}

// Parse the given file to memory
Config *parseConfigFile (char *filename) {

    if (filename == NULL) return NULL;

    Config *cfg = NULL;

    FILE *configFile = fopen (filename, "r");
    if (configFile == NULL) return NULL;

    cfg = (Config *) malloc (sizeof (Config));
    cfg->entities = initList (free);

    char buffer[CONFIG_MAX_LINE_LEN];

    ConfigEntity *currentEntity = NULL;

    while (fgets (buffer, CONFIG_MAX_LINE_LEN, configFile) != NULL) {
        // we have a new entity
        if (buffer[0] == '[') currentEntity = newEntity (buffer, cfg);

        // we have a key/value data, so parse it into its various parts 
        else if ((buffer[0] != '\n') && (buffer[0] != ' ')) 
            getData (buffer, currentEntity);
        
    }

    return cfg;

}


/*** RETURN THE PARSE DATA ***/

// get a value for a given key in an entity
char *getEntityValue (ConfigEntity *entity, char *key) {

    for (ListElement *e = LIST_START (entity->keyValuePairs); e != NULL; e = e->next) {
        ConfigKeyValuePair *kvp = (ConfigKeyValuePair *) e->data;
        if (strcmp (key, kvp->key) == 0) return kvp->value;
    }

    return NULL;

}