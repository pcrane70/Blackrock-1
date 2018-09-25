#ifndef GAMEUI_H
#define GAMEUI_H

#include "ui.h"

#include "item.h"

#include "utils/list.h"

#define NO_COLOR        0x00000000
#define WHITE           0xFFFFFFFF
#define BLACK           0x000000FF

#define FULL_GREEN      0x00FF00FF
#define FULL_RED        0xFF0000FF

#define YELLOW          0xFFD32AFF
#define SAPPHIRE        0x1E3799FF

/*** MAP ***/

extern UIView *mapView;

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

extern void deleteMessage (Message *msg);
extern void cleanMessageLog (void);

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


/*** FULL SCREEN ***/

#define FULL_SCREEN_LEFT		0
#define FULL_SCREEN_TOP		    0
#define FULL_SCREEN_WIDTH		80
#define FULL_SCREEN_HEIGHT	    45


/*** LOOT ***/

extern UIView *lootView;

extern void toggleLootWindow (void);
extern void updateLootUI (u8 yIdx);

extern List *activeLootRects;
extern u8 lootYIdx;

/*** TOOLTIP ***/

extern UIView *tooltipView;
extern void toggleTooltip (u8);

extern Item *lootItem;

/*** INVENTORY ***/

extern UIView *inventoryView;

extern u8 inventoryXIdx;
extern u8 inventoryYIdx;

extern void toggleInventory (void);
extern Item *getInvSelectedItem (void);
extern void resetInventoryRects (void);

/*** CHARACTER ***/

extern UIView *characterView;

extern u8 characterXIdx;
extern u8 characterYIdx;

extern Item *getCharSelectedItem (void);

extern void resetCharacterRects (void);
extern void toggleCharacter (void);

/*** PAUSE MENU ***/

extern void togglePauseMenu (void);

extern UIScreen *gameScene (void);

/*** CLEAN UP GAME UI ***/

extern void resetGameUI (void);
extern void destroyGameUI (void);

/*** POST GAME ***/

extern UIScreen *postGameScene;

extern UIView *deathScreen;
extern void  deleteDeathScreen (void);

extern UIView *scoreScreen;
extern void toggleScoreScreen (void);

extern UIScreen *postGameScreen (void);

extern void destroyPostGameScreen (void);

#endif