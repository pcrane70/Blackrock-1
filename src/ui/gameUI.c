/*** This file handles the in game UI and the input for playing the game ***/

#include <SDL2/SDL.h>
#include <string.h>     // for message functions

#include "blackrock.h"
#include "game.h"
#include "map.h"    // for walls array

#include "ui/ui.h"
#include "ui/console.h"
#include "ui/gameUI.h"

#include "list.h"       // for messages

#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

#define LOG_WIDTH		60
#define LOG_HEIGHT		5

#define INVENTORY_LEFT		20
#define INVENTORY_TOP		7
#define INVENTORY_WIDTH		40
#define INVENTORY_HEIGHT	30

// TODO: maybe in the future we acn add more graphics, but for now we are sticking with
// only ascii chars
// In linux we have to take the path from the makefile 
char* tileset = "./resources/terminal-art.png";  


/*** UI ***/

// Game

Player *playerComp;

u8 layerRendered[MAP_WIDTH][MAP_HEIGHT];
extern u32 fovMap[MAP_WIDTH][MAP_HEIGHT];

u32 wallsFgColor = 0xFFFFFFFF;
u32 wallsBgColor = 0x000000FF;
u32 wallsFadedColor;
asciiChar wallGlyph = '#';  // 19/08/2018 -- 18:00 -- we are assuming that are walls are the same

static void renderMap (Console *console) {

    // render the player
    Position *playerPos = (Position *) getComponent (player, POSITION);
    Graphics *playerGra = (Graphics *) getComponent (player, GRAPHICS);
    putCharAt (console, playerGra->glyph, playerPos->x, playerPos->y, playerGra->fgColor, playerGra->bgColor);

    // setup the layer rendering
    
    for (u32 x = 0; x < MAP_WIDTH; x++)
        for (u32 y = 0; y < MAP_HEIGHT; y ++)   
            layerRendered[x][y] = UNSET_LAYER;

    // render the gos with graphics
    GameObject *go = NULL;
    Position *p = NULL;
    Graphics *g = NULL;
    u32 fullColor;
    u32 fadedColor;
    for (int layer = GROUND_LAYER; layer <= TOP_LAYER; layer++) {
        for (ListElement *ptr = LIST_START (gameObjects); ptr != NULL; ptr = ptr->next) {
            go = (GameObject *) ptr->data;
            p = (Position *) getComponent (go, POSITION);
            if (p != NULL && p->layer == layer) {
                g = getComponent (go, GRAPHICS);
                // FIXME: FOV
                // if (fovMap[p->x][p->y] > 0) {
                //     g->hasBeenSeen = true;
                //     putCharAt (console, g->glyph, p->x, p->y, g->fgColor, g->bgColor);
                //     layerRendered[p->x][p->y] = p->layer;
                // }

                // else if (g->visibleOutsideFov && g->hasBeenSeen) {
                //     fullColor = g->fgColor;
                //     fadedColor = COLOR_FROM_RGBA (RED (fullColor), GREEN (fullColor), BLUE (fullColor), 0x77);
                //     putCharAt (console, g->glyph, p->x, p->y, fadedColor, 0x000000FF);
                //     layerRendered[p->x][p->y] = p->layer;
                // }

                putCharAt (console, g->glyph, p->x, p->y, g->fgColor, g->bgColor);
                layerRendered[p->x][p->y] = p->layer;
            }
            
        }
    }    

    // FIXME: FOV
    // 19/08/2018 -- 17:53 -- we are assuming all walls are visible outside fov
    // for (unsigned int i = 0; i < wallCount; i++) {
    //     if (fovMap[walls[i].x][walls[i].y] > 0) {
    //         walls[i].hasBeenSeen = true;
    //         putCharAt (console, wallGlyph, walls[i].x, walls[i].y, wallsFgColor, wallsBgColor);
    //     }

    //     else if (walls[i].hasBeenSeen) 
    //         putCharAt (console, wallGlyph, walls[i].x, walls[i].y, wallsFadedColor, wallsBgColor);
        
    // }

    for (short unsigned int i = 0; i < wallCount; i++) 
        putCharAt (console, wallGlyph, walls[i].x, walls[i].y, wallsFgColor, wallsBgColor);
        
}

static void rednderStats (Console *console) {

    UIRect rect = { 0, 0, STATS_WIDTH, STATS_HEIGHT };
    drawRect (console, &rect, 0x222222FF, 0, 0xFF990099);

    putStringAt (console, playerComp->name, 0, 0, 0xFFFFFFFF, 0x00000000);

    // FIXME:
    // player health
    putCharAt (console, 'H', 0, 1, 0xFF990099, 0x00000000);
    putCharAt (console, 'P', 1, 1, 0xFF990099, 0x00000000);
    i32 leftX = 3;
    i32 barWidth = 16;

    // TODO: what other stats do we want to render?

}

/*** MESSAGE LOG ***/

List *messageLog = NULL;

// create a new message in the log
void logMessage (char *msg, u32 color) {

    Message *m = (Message *) malloc (sizeof (Message));

    if (m != NULL) {
        m->msg = (char *) calloc (strlen (msg) + 1, sizeof (char));
        // TODO: change this for my own function...
        strcpy (m->msg, msg);
    }

    else m->msg = "";

    m->fgColor = color;

    // add message to the log
    insertAfter (messageLog, LIST_END (messageLog), m);

    // TODO: how many messages do we want to keep?
    if (LIST_SIZE (messageLog) > 20) removeElement (messageLog, NULL);  // remove the oldest message

}

static void renderLog (Console *console) {

    UIRect rect = { 0, 0, LOG_WIDTH, LOG_HEIGHT };
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *createString (const char *stringWithFormat, ...) {

    char *fmt;

    if (stringWithFormat != NULL) fmt = strdup (stringWithFormat);
    else fmt = strdup ("");

    va_list argp;
    va_start (argp, stringWithFormat);
    char oneChar[1];
    int len = vsnprintf (oneChar, 1, fmt, argp);
    if (len < 1) return NULL;
    va_end (argp);

    char *str = (char *) calloc (len + 1, sizeof (char));
    if (!str) return NULL;

    va_start (argp, stringWithFormat);
    vsnprintf (str, len + 1, fmt, argp);
    va_end (argp);

    free (fmt);

    return str;

}
 

/*** INVENTORY ***/

UIView *inventoryView = NULL;

static void renderInventory (Console *console) {

    UIRect rect = { 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT };
    drawRect (console, &rect, 0x222222FF, 0, 0xFF990099);

    // TODO: do we want a background image??

    // list the inventory items
    if (LIST_SIZE (playerComp->inventory) == 0) {
        // FIXME: change text color and position
        putStringAt (console, "Inventory is empty!", 5, 10, 0x333333FF, 0x00000000);
        return;
    }

    GameObject *go = NULL;
    Graphics *graphics = NULL;
    Item *item = NULL;
    for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        go = (GameObject *) e->data;
        graphics = (Graphics *) getComponent (go, GRAPHICS);
        item = (Item *) getComponent (go, ITEM);
        if (graphics != NULL && item != NULL) {
            // FIXME:
        }
    }

}

void hideInventory (UIScreen *screen) {

    if (inventoryView != NULL) {
        ListElement *inv = getListElement (screen->views, inventoryView);
        destroyView ((UIView *) removeElement (screen->views, inv));
        inventoryView = NULL;
    }

}

void showInventory (UIScreen *screen) {

    if (inventoryView == NULL) {
        UIRect inv = { (16 * INVENTORY_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT) };
        inventoryView = newView (inv, INVENTORY_WIDTH, INVENTORY_HEIGHT, tileset, 0, 0x000000FF, true, renderInventory);
        insertAfter (screen->views, LIST_END (screen->views), inventoryView);
    }

}

void toggleInventory () {

    // TODO: how do we check that is the correct view?

    if (inventoryView == NULL) showInventory (activeScene);
    else hideInventory (activeScene);

}

/*** INIT GAME SCREEN ***/

UIScreen *gameScene () {

    // FIXME: are we cleanning up this?
    List *igViews = initList (NULL);

    UIRect mapRect = { 0, 0, (16 * MAP_WIDTH), (16 * MAP_HEIGHT) };
    bool colorize = true;
    u32 bgColor;

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

    playerComp = (Player *) getComponent (player, PLAYER);

    wallsFadedColor = COLOR_FROM_RGBA (RED (wallsFgColor), GREEN (wallsFgColor), BLUE (wallsFgColor), 0x77);

    return inGameScreen;

}
