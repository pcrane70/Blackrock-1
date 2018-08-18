/*** This file handles the in game UI and the input for playing the game ***/

#include <SDL2/SDL.h>

#include "blackrock.h"
#include "game.h"

#include "ui/ui.h"
#include "ui/console.h"

#include "list.h"

#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

#define LOG_WIDTH		60
#define LOG_HEIGHT		5

#define INVENTORY_LEFT		20
#define INVENTORY_TOP		7
#define INVENTORY_WIDTH		40
#define INVENTORY_HEIGHT	30


/*** UI ***/

// Game

static void renderMap (Console *console) {



}

// TODO: 
static void rednderStats () {}

// FIXME: where do we want to put these?
// This will store the log messages
typedef struct Message {

    char *msg;
    u32 fgColor;

} Message;

List *messageLog = NULL;

static void renderLog (Console *console) {

    Rect rect = { 0, 0, LOG_WIDTH, LOG_HEIGHT };
    drawRect (console, &rect, 0x191919FF, 0, 0xFF990099);

    if (messageLog == NULL) return; // we don't have any messages to display

    // get the last 5 messages from the log
    ListElement *e = LIST_END (messageLog);
    i32 msgCount = LIST_SIZE (messageLog);
    u32 row = 4;
    u32 col = 1;

    if (msgCount < 5) row -= (5 - msgCount);
    else msgCount = 5;

    for (u32 i = 0; i < msgCount; i++) {
        if (e != NULL) {
            Message *m = (Message *) LIST_DATA (e);
            Rect rect = { .x = col, .y = row, .w = LOG_WIDTH, .h = 1 };
            putStringAtRect (console, m->msg, rect, false, m->fgColor, 0x00000000);
            e = e->prev;
            row -= 1;
        }
    }

}
 
UIScreen *gameScene () {

    List *igViews = initList (NULL);

    UIRect mapRect = { 0, 0, (16 * MAP_WIDTH), (16 * MAP_HEIGHT) };
    char *tileset;
    bool colorize = true;
    u32 bgColor;

    // TODO: maybe in the future we acn add more graphics, but for now we are sticking with
    // only ascii chars
    tileset = "./resources/terminal-art.png";  // In linux we have to take the path from the makefile 
    colorize = true;
    bgColor = 0x000000FF;

    UIView *mapView = newView (mapRect, MAP_WIDTH, MAP_HEIGHT, tileset, 0, bgColor, colorize, renderMap);
    insertAfter (igViews, NULL, mapView);

    UIRect statsRect = { 0, (16 * MAP_HEIGHT), (16 * STATS_WIDTH), (16 * STATS_HEIGHT) };
    UIView *statsView = newView (statsRect, STATS_WIDTH, STATS_HEIGHT, tileset, 0, 0x000000FF, true, rednderStats);
    insertAfter (igViews, NULL, statsView);

    UIRect logRect = { (16 * 20), (16 * MAP_HEIGHT), (16 * LOG_WIDTH), (16 * LOG_HEIGHT) };
    UIView *logView = newView (logRect, LOG_WIDTH, LOG_HEIGHT, tileset, 0, 0x000000FF, true, renderLog);
    insertAfter (igViews, NULL, logView);

    UIScreen *inGameScreen = (UIScreen *) malloc (sizeof (UIScreen));
    inGameScreen->views = igViews;
    inGameScreen->activeView = mapView;
    void hanldeGameEvent (UIScreen *, SDL_Event);
    inGameScreen->handleEvent = hanldeGameEvent;

    return inGameScreen;

}

// Inventory

UIView *inventoryView = NULL;

void toggleInventory (UIScreen *screen) {

    if (inventoryView == NULL) {
       Rect inventoryRect = { (16 * INVENTORY_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT) };
        // FIXME: 
       // inventoryView = 
    }

}


/*** INPUT ***/

// Position *playerPos = NULL;
// Position newPos;

// TODO: maybe later we will want to move using the numpad insted to allow diagonal movement
void hanldeGameEvent (UIScreen *activeScreen, SDL_Event event) {

//     playerPos = (Position *) getComponent (player, POSITION);

//     if (event.type == SDL_KEYDOWN) {
//         SDL_Keycode key = event.key.keysym.sym;

//         switch (key) {
//             // Movement
//             // TODO: how do we want to handle combat logic?
//             case SDLK_w: 
//                 newPos.x = playerPos->x;
//                 newPos.y = playerPos->y - 1;
//                 if (canMove (newPos)) playerPos->y = newPos.y;
//                 playerTookTurn = true; break;
//             case SDLK_s: 
//                 newPos.x = playerPos->x;
//                 newPos.y = playerPos->y + 1;
//                 if (canMove (newPos)) playerPos->y = newPos.y;
//                 playerTookTurn = true; break;
//             case SDLK_a: 
//                 newPos.x = playerPos->x - 1;
//                 newPos.y = playerPos->y;
//                 if (canMove (newPos)) playerPos->x = newPos.x;
//                 playerTookTurn = true; break;
//             case SDLK_d:
//                 newPos.x = playerPos->x + 1;
//                 newPos.y = playerPos->y;
//                 if (canMove (newPos)) playerPos->x = newPos.x;
//                 playerTookTurn = true; break;

//             // TODO: thi is used as a master key to have interaction with various items
//             // case SDLK_e: break;

//             // case SDLK_g: getItem (); break;

//             // TODO: drop an item
//             // case SDLK_d: break;

//             // TODO: toggle inventory
//             // case SDLK_i: break;

//             // TODO: equip an item
//             // case SDLK_q: break;

//             // TODO: toggle character equipment
//             // case SDLK_c: break;

//             // TODO: player rests?
//             // case SDLK_z: break;

//             // TODO: toggle pause menu
//             // case SDLK_p: break;

//             //TODO: what other things do we want?

//             default: break;
//         }
//     }

}