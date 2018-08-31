#ifndef GAMEUI_H
#define GAMEUI_H

#include "ui.h"

#include "item.h"

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


/*** MESSAGE COLORS ***/

#define DEFAULT_COLOR    0xFFFFFFFF

#define SUCCESS_COLOR   0x009900FF
#define WARNING_COLOR   0x990000FF

#define HIT_COLOR       0xF2F2F2FF  // succesfull attack
#define CRITICAL_COLOR  0xFFDB22FF  // critical hit
#define MISS_COLOR      0xCCCCCCFF  // missed attack
#define STOPPED_COLOR   0xFBFBFBFF  // parry, dodge, block
#define KILL_COLOR      0xFF9900FF  // you kill a mob

#define HEALTH_COLOR    0x00FF22FF  // player gains health
#define DAMAGE_COLOR    0x8C2020FF  // player loses health


/*** LOOT ***/

extern UIView *lootView;

extern void toggleLootWindow (void);
extern void updateLootUI (u8 yIdx);

extern List *lootRects;
extern u8 lootYIdx;

/*** INVENTORY ***/

extern UIView *inventoryView;

extern u8 inventoryXIdx;
extern u8 inventoryYIdx;

extern void toggleInventory (void);
extern Item *getSelectedItem (void);

/*** CHARACTER ***/

extern UIView *characterView;

extern u8 characterXIdx;
extern u8 characterYIdx;

extern void toggleCharacter (void);

/*** PAUSE MENU ***/

extern void togglePauseMenu (void);


// THIS SHOULD ONLY BE CALLED FROM THE UI CONTROLLER
extern UIScreen *gameScene (void);

/*** CLEAN UP ***/

extern void cleanGameUI (void);

#endif