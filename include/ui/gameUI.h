#ifndef GAMEUI_H
#define GAMEUI_H

#include "ui.h"

#include "utils/list.h"

/*** MESSAGES ***/

// FIXME: where do we want to put these?
// This will store the log messages
typedef struct Message {

    char *msg;
    u32 fgColor;

} Message;

// TODO: maybe later, if we add multiplayer, we might wanna have a more advanced message log like in wow
// think about a similar functionality to fprintf
extern List *messageLog;

// create a new message in the log
extern void logMessage (char *msg, u32 color);
extern char *createString (const char *stringWithFormat, ...);

/*** LOOT ***/

extern void toggleLootWindow (void);

/*** INVENTORY ***/

extern void toggleInventory (void);


// THIS SHOULD ONLY BE CALLED FROM THE UI CONTROLLER
extern UIScreen *gameScene (void);

#endif