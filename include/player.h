#include "blackrock.h"

#include "game.h"
#include "item.h"

typedef enum CharClass {

    WARRIOR = 1,
    PALADIN,
    ROGUE,
    PRIEST,
    DEATH_KNIGHT,
    MAGE

} CharClass;

typedef struct Player {

    char *name;
    u8 genre;     // 0 female, 1 male
    // TODO: races
    CharClass cClass;
    u32 color;  // for accessibility
    u8 level;
    u16 money [3];  // gold, silver, copper
    Item ***inventory;
    Item **weapons;      // a player can have 2 on-handed or a 2 handed or equip a shield
    Item **equipment;

    Position *pos;
    Graphics *graphics;
    Physics *physics;
    Combat *combat;

} Player;

#define MAIN_HAND       0
#define OFF_HAND        1

#define EQUIPMENT_ELEMENTS      10

// head         0
// necklace     1
// shoulders    2
// cape         3
// chest        4

// hands        5
// belt         6
// legs         7
// shoes        8
// ring         9


extern Player *player;

extern u8 inventoryItems;

extern Player *createPlayer (void);
void initPlayer (Player *);
extern void resetPlayer (Player *);
extern char *getPlayerClassName (u8);
extern u32 getPlayerClassColor (u8);
extern void destroyPlayer (void);