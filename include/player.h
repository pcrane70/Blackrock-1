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

typedef struct PlayerProfile {

    u32 profileID;
    char *username;

    u32 kills;
    u32 gamesPlayed;
    u32 highscore;

    u32 n_friends;
    char *friends;
    // char *clan;

} PlayerProfile;

struct _SPlayerProfile;

extern void player_profile_get_from_server (struct _SPlayerProfile *s_profile);

typedef struct Player {

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

extern Player *main_player;

extern u8 inventoryItems;

extern Player *player_create (void);
void player_init (Player *);
extern void player_reset (Player *);
extern char *player_get_class_name (u8);
extern u32 player_get_class_color (u8);
extern void player_destroy (Player *);

/*** SERIALIZATION ***/

struct _SPlayerProfile {

    u32 profileID;
    char username[64];

    u32 kills;
    u32 gamesPlayed;
    u32 highscore;

    u32 n_friends;
    // char friends[64];
    // char clan[64];

};

typedef struct _SPlayerProfile SPlayerProfile;