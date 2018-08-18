/*** This file handles the in game UI and the input for playing the game ***/

#include <SDL2/SDL.h>
#include <string.h>     // for message functions

#include "blackrock.h"
#include "game.h"

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


/*** UI ***/

// Game

// FIXME:
static void renderMap (Console *console) {

    // TODO: the logic for fov goes in here!!

    // render the player
    // TODO: do we want the player to have its own structure??
    Position *playerPos = (Position *) getComponent (player, POSITION);
    Graphics *playerGra = (Graphics *) getComponent (player, GRAPHICS);
    putCharAt (console, playerGra->glyph, playerPos->x, playerPos->y, playerGra->fgColor, playerGra->bgColor);

    // TODO:
    // render the go with graphics
    GameObject *go = NULL;
    Position *p = NULL;
    Graphics *g = NULL;
    for (ListElement *ptr = LIST_START (gameObjects); ptr != NULL; ptr = ptr->next) {
        go = (GameObject *) ptr->data;
        p = (Position *) getComponent (go, POSITION);
        g = (Graphics *) getComponent (go, GRAPHICS);
        putCharAt (console, g->glyph, p->x, p->y, g->fgColor, g->bgColor);
    }

    // FIXME: we don't want to this every frame!!
    for (unsigned int i = 0; i < wallCount; i++) 
        putCharAt (console, walls[i].glyph, walls[i].x, walls[i].y, walls[i].fgColor, walls[i].bgColor);

}

static void rednderStats (Console *console) {

    UIRect rect = { 0, 0, STATS_WIDTH, STATS_HEIGHT };
    drawRect (console, &rect, 0x222222FF, 0, 0xFF990099);

    // FIXME: player name
    // TODO: change the color depending on the player class
    putStringAt (console, "ermiry", 0, 0, 0xFFFFFFFF, 0x00000000);

    // FIXME:
    // player health
    putCharAt (console, 'H', 0, 1, 0xFF990099, 0x00000000);
    putCharAt (console, 'P', 1, 1, 0xFF990099, 0x00000000);
    i32 leftX = 3;
    i32 barWidth = 16;

    // TODO: what other stats do we want to render?

}

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
 
UIScreen *gameScene () {

    // FIXME: are we cleanning up this?
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