#ifndef GAME_H_
#define GAME_H_

#include "blackrock.h"

#include "vector2d.h"

#include "engine/sprites.h"

#include "utils/llist.h"
#include "utils/dlist.h"
#include "utils/objectPool.h"

/*** GAME MANAGER ***/

typedef enum State {

    LOGO = 0,
    PRELOADING = 1, 
    MAIN_MENU = 2,
    IN_GAME = 3,
    GAME_OVER = 4,

} State;

typedef enum SessionType {

    SINGLE_PLAYER = 0,
    MULTIPLAYER

} SessionType;

SessionType sessionType; 

typedef struct GameSate {

    State state;

    void (*update)(void);
    void (*render)(void);

    void (*onEnter)(void);
    void (*onExit)(void);

} GameState;

typedef struct GameManager {

    GameState *currState;

} GameManager;

extern GameManager *game_manager;

extern GameManager *game_manager_new (GameState *initState);
extern State game_state_get_current (void);
extern void game_state_change_state (GameState *newState);

extern void game_cleanUp (void);

// TODO: maybe add a function to register when we change to a state,
// so that we can load many things with like an array of events?

/*** GAME STATE ***/

extern GameState *game_state;

extern GameState *game_state_new (void);

/*** GAME OBJECTS ***/

#define DEFAULT_MAX_GOS     200

#define COMP_COUNT      7

// FIXME: better tag manager!! -> create a list for each tag?
typedef struct GameObject {
    
    i32 id;
    // FIXME: i dont want this here!!!
    u32 dbId;   // 06/09/2018 -- we need to this somewhere

    char *name;
    char *tag;
    void *components[COMP_COUNT];

    LList *children;
    void (*update)(void *data);

} GameObject;

extern GameObject *game_object_new (const char *name, const char *tag);
extern void game_object_destroy (GameObject *go);

extern void game_object_add_child (GameObject *parent, GameObject *child);
extern GameObject *game_object_remove_child (GameObject *parent, GameObject *child);

/*** COMPONENTS ***/

typedef enum GameComponent {

    TRANSFORM_COMP = 0,
    GRAPHICS_COMP,
    ANIMATOR_COMP,
    BOX_COLLIDER_COMP,

    PLAYER_COMP,

} GameComponent;

extern void *game_object_add_component (GameObject *go, GameComponent component);
extern void *game_object_get_component (GameObject *go, GameComponent component);
extern void game_object_remove_component (GameObject *go, GameComponent component);

typedef struct Transform {

    u32 goID;
    Vector2D position;

} Transform;

typedef struct Position {

    u32 objectId;
    u8 x, y;
    u8 layer;   

} Position;

typedef enum Layer {

    UNSET_LAYER = 0,
    GROUND_LAYER = 1,
    LOWER_LAYER = 2,
    MID_LAYER = 3,
    TOP_LAYER = 4,

} Layer;

typedef enum Flip {

    NO_FLIP = 0x00000000,
    FLIP_HORIZONTAL = 0x00000001,
    FLIP_VERTICAL = 0x00000002

} Flip;

typedef struct Graphics {

    u32 goID;

    Sprite *sprite;
    SpriteSheet *spriteSheet;

    u32 x_sprite_offset, y_sprite_offset;
    bool multipleSprites;
    Layer layer; 
    Flip flip;
    bool hasBeenSeen;
    bool visibleOutsideFov;

} Graphics;

extern void graphics_set_sprite (Graphics *graphics, const char *filename);
extern void graphics_set_sprite_sheet (Graphics *graphics, const char *filename);

// old!
// typedef struct Graphics {

//     u32 objectId;
//     asciiChar glyph;
//     u32 fgColor;
//     u32 bgColor;
//     bool hasBeenSeen;
//     bool visibleOutsideFov;
//     char *name;     // 19/08/2018 -- 16:47

// } Graphics;

typedef struct Physics {

    u32 objectId;
    bool blocksMovement;
    bool blocksSight;

} Physics;

typedef struct Movement {

    u32 objectId;
    u32 speed;
    u32 frecuency;
    u32 ticksUntilNextMov;
    Point destination;
    bool hasDestination;
    bool chasingPlayer;
    u32 turnsSincePlayerSeen;

} Movement;

/*** EVENTS ***/

typedef void (*EventListener)(void *);

typedef struct Event {

    u32 objectId;
    EventListener callback;

} Event;


/*** OUR LISTS ***/

extern DoubleList *positions;
extern DoubleList *graphics;
extern DoubleList *physics;

/*** POOLS ***/

extern Pool *goPool;
extern Pool *posPool;
extern Pool *graphicsPool;
extern Pool *physPool;


/*** GAME STATE ***/

// FIXME: move this into our new game manager and states -- 16/01/2018
extern void startGame (void);
extern void initGame (void);

extern bool playerTookTurn;

extern void *updateGame (void *);


/*** Game Objects ***/

extern GameObject *createGO (void);
extern void *getComponent (GameObject *, GameComponent);
extern void addComponent (GameObject *go, GameComponent type, void *data);
extern DoubleList *getObjectsAtPos (u32 x, u32 y);
extern GameObject *searchGameObjectById (u32);

/*** LOOT ***/

typedef struct Loot {

    u32 objectId;
    u8 money[3];
    DoubleList *lootItems;
    bool empty;

} Loot;

void collectGold (void);

/*** MOVEMENT ***/

extern bool canMove (Position pos, bool isPlayer);
extern bool recalculateFov;

/*** LEVEL MANAGER ***/

typedef struct {

    u8 levelNum;
    bool **mapCells;    // dungeon map

} Level;

extern Level *currentLevel;

extern unsigned int wallCount;

extern void retry (void);

/*** SCORE ***/

typedef struct Score {

    u32 killCount;
    u32 score;

} Score;

extern void showScore (void);

/*** LEADERBOARDS ***/

typedef struct LBEntry {

    char *playerName;
    char *completeName;
    u8 charClass;
    u32 nameColor;
    char *level;
    char *kills;
    u32 score;
    char *reverseScore;

} LBEntry;

extern DoubleList *localLBData;
extern DoubleList *globalLBData;

extern DoubleList *getLocalLBData (void);
extern DoubleList *getGlobalLBData (void);

/*** MULTIPLAYER ***/

#pragma region MULTIPLAYER

typedef enum BlackErrorType {

    BLACK_ERROR_SERVER = 0,

    BLACK_ERROR_WRONG_CREDENTIALS,
    BLACK_ERROR_USERNAME_TAKEN,

} BlackErrorType;

typedef struct BlackError {

    BlackErrorType errorType;
    char msg[128];

} BlackError;

/*** BLACKROCK PACKETS ***/

extern bool multiplayer;

typedef enum BlackPacketType {

    PLAYER_PROFILE,

} BlackPacketType;

typedef struct BlackPacketData {

    BlackPacketType blackPacketType;

} BlackPacketData;

struct _BlackCredentials {

    bool login;
    char username[64];
    char password[64];

};

typedef struct _BlackCredentials BlackCredentials;

typedef struct BlackAuthData {

    Connection *connection;
    BlackCredentials *credentials;

} BlackAuthData;

extern u8 multiplayer_start (BlackCredentials *black_credentials);
extern u8 multiplayer_stop (void);

extern void multiplayer_createLobby (void *);
extern void multiplayer_joinLobby (void *);
extern void multiplayer_leaveLobby (void);

extern void multiplayer_submit_credentials (void *);

#pragma endregion

/*** CLEANING UP ***/

extern void cleanUpGame (void);

#endif