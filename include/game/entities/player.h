#ifndef PLAYER_H
#define PLAYER_H

#include "blackrock.h"

#include "game/entities/entity.h"

#include "collections/dlist.h"

typedef enum PlayerState {

    PLAYER_IDLE = 0,
    PLAYER_MOVING,
    PLAYER_ATTACK,

} PlayerState;

typedef struct PlayerProfile {

    u32 profileID;
    char *username;

    u32 kills;
    u32 gamesPlayed;
    u32 highscore;

    u32 n_friends;
    char *friends;
    // char *guild;

} PlayerProfile;

typedef struct Character {

    LivingEntity *entity;

    u16 money [3];          // gold, silver, copper
    GameObject ***inventory;
    GameObject **weapons;
    GameObject **equipment;

    DoubleList *animations;

} Character;

typedef struct Player {

    u32 goID;

    PlayerState currState;
    PlayerProfile *profile;
    Character *character;

} Player;

extern Player *player_comp_new (u32 goID);
extern void player_comp_delete (Player *player);

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

extern GameObject *player_init (void);
extern void player_update (void *data);

extern GameObject *main_player_go;

/*** SERIALIZATION ***/

// TODO:
/* struct _SPlayerProfile {

    u32 profileID;
    char username[64];

    u32 kills;
    u32 gamesPlayed;
    u32 highscore;

    u32 n_friends;
    // char friends[64];
    // char guild[64];

};

typedef struct _SPlayerProfile SPlayerProfile; */

#endif