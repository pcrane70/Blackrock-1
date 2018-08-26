/*** This file handles the in game UI and the input for playing the game ***/

#include <string.h>     // for message functions

#include "blackrock.h"
#include "game.h"
#include "map.h"    // for walls array

#include "ui/ui.h"
#include "ui/console.h"
#include "ui/gameUI.h"

#include "item.h"

#include "input.h"

#include "utils/list.h"       // for messages

#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

#define LOG_WIDTH		60
#define LOG_HEIGHT		5

/*** UI ***/

Player *playerComp;

u8 layerRendered[MAP_WIDTH][MAP_HEIGHT];
extern u32 fovMap[MAP_WIDTH][MAP_HEIGHT];

u32 wallsFgColor = 0xFFFFFFFF;
u32 wallsBgColor = 0x000000FF;
u32 wallsFadedColor;
asciiChar wallGlyph = '#';  // 19/08/2018 -- 18:00 -- we are assuming that are walls are the same

// FIXME:
static void renderMap (Console *console) {

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
        
    // render the player
    Position *playerPos = (Position *) getComponent (player, POSITION);
    Graphics *playerGra = (Graphics *) getComponent (player, GRAPHICS);
    putCharAt (console, playerGra->glyph, playerPos->x, playerPos->y, playerGra->fgColor, playerGra->bgColor);

}

// FIXME: create a more efficient way
static void rednderStats (Console *console) {

    UIRect rect = { 0, 0, STATS_WIDTH, STATS_HEIGHT };
    drawRect (console, &rect, 0x222222FF, 0, 0xFF990099);

    char *str = createString ("%s the warrior", playerComp->name);
    putStringAt (console, str, 0, 0, 0xFFFFFFFF, 0x00000000);

    Combat *playerCombat = (Combat *) getComponent (player, COMBAT);
    // TODO: make this have dynamic colors
    str = createString ("HP: %i/%i", playerCombat->baseStats.health, playerCombat->baseStats.maxHealth);
    putStringAt (console, str, 0, 1, 0xFF990099, 0x00000000);

    free (str);

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

    if (LIST_SIZE (messageLog) > 15) removeElement (messageLog, NULL);  // remove the oldest message

}

// TODO: add scrolling
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


/*** LOOT ***/

#define LOOT_LEFT       30
#define LOOT_TOP        9
#define LOOT_WIDTH      20
#define LOOT_HEIGHT     24

#define LOOT_COLOR     0x69777DFF
#define LOOT_TEXT      0xEEEEEEFF

UIView *lootView = NULL;

extern Loot *currentLoot;

#define LOOT_RECT_WIDTH     18
#define LOOT_RECT_HEIGHT    4

#define LOOT_IMG_WIDTH    4
#define LOOT_IMG_HEIGHT   4

typedef struct LootRect {

    UIRect *bgRect;
    UIRect *imgRect;
    Item *item;

} LootRect;

List *lootRects = NULL;

LootRect *createLootRect (u8 y, Item *i) {

    LootRect *new = (LootRect *) malloc (sizeof (LootRect));
    new->bgRect = (UIRect *) malloc (sizeof (UIRect));
    new->imgRect = (UIRect *) malloc (sizeof (UIRect));

    new->bgRect->x = 1;
    new->bgRect->y = y + 4 + (4 * y);
    new->bgRect->w = LOOT_RECT_WIDTH;
    new->bgRect->h = LOOT_RECT_HEIGHT;

    new->imgRect->x = 1;
    new->imgRect->y = y + 4 + (4 * y);
    new->imgRect->w = LOOT_IMG_WIDTH;
    new->imgRect->h = LOOT_IMG_HEIGHT;

    new->item = i;

    return new;

}

void destroyLootRect (LootRect *lr) {

    if (lr != NULL) {
        if (lr->bgRect != NULL) free (lr->bgRect);
        if (lr->imgRect != NULL) free (lr->imgRect);
        lr->item = NULL;
    }

}

void drawLootRect (Console *console, LootRect *rect, u32 bgColor) {

    drawRect (console, rect->bgRect, bgColor, 0, 0x00000000);
    drawRect (console, rect->imgRect, 0x000000FF, 0, 0x00000000);

    Graphics *g = (Graphics *) getItemComp (rect->item, GRAPHICS);
    if (g != NULL) {
        u32 color;
        switch (rect->item->rarity) {
            case 0: color = RUBISH_COLOR; break;
            case 1: color = COMMON_COLOR; break;
            case 2: color = RARE_COLOR; break;
            case 3: color = EPIC_COLOR; break;
            case 4: color = LEGENDARY_COLOR; break;
            default: color = COMMON_COLOR; break;
        }
        
        putStringAt (console, g->name, 7, (rect->bgRect->y) + 2, color, 0x00000000);
    }

}

u8 lootYIdx = 0;

void updateLootUI (u8 yIdx) {

    u8 count = 0;
    if (lootRects != NULL && (LIST_SIZE (lootRects) > 0)) {
        for (ListElement *e = LIST_START (lootRects); e != NULL; e = e->next) {
            if (count == yIdx) {
                destroyLootRect ((LootRect *) removeElement (lootRects, e));
                break;
            }

            count ++;
        }
    }

    if (lootYIdx >= LIST_SIZE (lootRects)) lootYIdx -= 1;

}

static void renderLoot (Console *console) {

    if (currentLoot != NULL) {
        UIRect looRect = { 0, 0, LOOT_WIDTH, LOOT_HEIGHT };
        drawRect (console, &looRect, LOOT_COLOR, 0, 0xFF990099);

        putStringAt (console, "Loot", 8, 2, LOOT_TEXT, 0x00000000);

        if (LIST_SIZE (lootRects) == 0) {
            // reset highlighted
            lootYIdx = 0;

            if (currentLoot->lootItems != NULL) {
                u8 y = 0;
                for (ListElement *e = LIST_START (currentLoot->lootItems); e != NULL; e = e->next) { 
                    LootRect *lr = createLootRect (y, (Item *) e->data);
                    if (y == 0) drawLootRect (console, lr, 0x000000FF);
                    else drawLootRect (console, lr, 0xFFFFFFFF);
                    insertAfter (lootRects, LIST_END (lootRects), lr);
                    y++;
                }
            }
        }

        else {
            u8 count = 0;
            for (ListElement *e = LIST_START (lootRects); e != NULL; e = e->next) {
                if (count == lootYIdx) drawLootRect (console, (LootRect *) e->data, 0x000000FF);
                else drawLootRect (console, (LootRect *) e->data, 0xFFFFFFFF);
                count++;
            }
        }        

        // gold
        char *gold = createString ("%ig - %is - %ic", currentLoot->money[0], currentLoot->money[1], currentLoot->money[2]);
        putStringAt (console, gold, 3, 21, LOOT_TEXT, 0x00000000);
        free (gold);
    }

}

void createLootWindow () {

    UIRect lootRect = { (16 * LOOT_LEFT), (16 * LOOT_TOP), (16 * LOOT_WIDTH), (16 * LOOT_HEIGHT) };
    lootView = newView (lootRect, LOOT_WIDTH, LOOT_HEIGHT, tileset, 0, 0x000000FF, true, renderLoot);
    insertAfter (activeScene->views, LIST_END (activeScene->views), lootView);

    if (lootRects == NULL) lootRects = initList (free);
    else if (LIST_SIZE (lootRects) > 0) resetList (lootRects); 

}

void destroyLootWindow () {

    if (lootView != NULL) {
        ListElement *e = getListElement (activeScene->views, lootView);
        destroyView ((UIView *) removeElement (activeScene->views, e));
        lootView = NULL;

        if (lootRects != NULL && LIST_SIZE (lootRects) > 0) {
            for (ListElement *e = LIST_START (lootRects); e != NULL; e = e->next) 
                destroyLootRect ((LootRect *) e->data);

            resetList (lootRects);
        }
    }

}

void toggleLootWindow (void) {

    // show the loot window
    if (lootView == NULL) createLootWindow ();
    // hide the loot window
    else destroyLootWindow ();

}

/*** INVENTORY ***/

#define INVENTORY_LEFT		20
#define INVENTORY_TOP		7
#define INVENTORY_WIDTH		40
#define INVENTORY_HEIGHT	30

#define INVENTORY_CELL_WIDTH    4
#define INVENTORY_CELL_HEIGHT   4

#define INVENTORY_COLOR     0x967933FF
#define INVENTORY_TEXT      0xEEEEEEFF

UIView *inventoryView = NULL;

u8 inventoryXIdx = 0;
u8 inventoryYIdx = 0;

static void renderInventory (Console *console) {

    UIRect rect = { 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT };
    drawRect (console, &rect, INVENTORY_COLOR, 0, 0xFFFFFFFF);

    putStringAt (console, "Inventory", 16, 2, INVENTORY_TEXT, 0x00000000);

    u8 inventoryIdx = 0;
    Item *item = NULL;

    // draw inventory cells
    for (u8 y = 0; y < 3; y++) {
         for (u8 x = 0; x < 7; x++) {
            UIRect rect = { x + 3 + (INVENTORY_CELL_WIDTH * x), y + 5 + (INVENTORY_CELL_HEIGHT * y), INVENTORY_CELL_WIDTH, INVENTORY_CELL_HEIGHT };
            
            // highlight selected cell
            if (inventoryXIdx == x && inventoryYIdx == y) {
                drawRect (console, &rect, 0x000000FF, 0, 0x000000FF);

                u8 count = 0;
                for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
                    if (count == inventoryIdx) {
                        item = (Item *) e->data;
                        break;
                    }
                    count++;
                }
            } 
            else drawRect (console, &rect, 0xFFFFFFFF, 0, 0x000000FF);

            inventoryIdx++;
        }
    }

    if (item != NULL) {
        Graphics *g = (Graphics *) getItemComp (item, GRAPHICS);
        if (g != NULL) {
            u32 color;
            switch (item->rarity) {
                case 0: color = RUBISH_COLOR; break;
                case 1: color = COMMON_COLOR; break;
                case 2: color = RARE_COLOR; break;
                case 3: color = EPIC_COLOR; break;
                case 4: color = LEGENDARY_COLOR; break;
                default: color = COMMON_COLOR; break;
            }
            
            putStringAt (console, g->name, 4, 21, color, 0x00000000);
        }
    }

    // Render additional info
    // char *weightInfo = createString ("Carrying: %i - Max: %i", getCarriedWeight (), playerComp->maxWeight);
    // putStringAt (console, weightInfo, 9, 25, 0x000044FF, 0x00000000);

    // gold
    char *gold = createString ("%ig - %is - %ic", playerComp->money[0], playerComp->money[1], playerComp->money[2]);
    putStringAt (console, gold, 13, 25, INVENTORY_TEXT, 0x00000000);

    putStringAt (console, "[wasd] to move", 4, 27, INVENTORY_TEXT, 0x00000000);
    putStringAt (console, "[e] to (un)equip, [Spc] to drop", 4, 28, INVENTORY_TEXT, 0x00000000);
    // free (weightInfo);
    free (gold);

}

void hideInventory () {

    if (inventoryView != NULL) {
        ListElement *inv = getListElement (activeScene->views, inventoryView);
        destroyView ((UIView *) removeElement (activeScene->views, inv));
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

void toggleInventory (void) {

    if (inventoryView == NULL) showInventory (activeScene);
    else hideInventory (activeScene);

}

/*** INIT GAME SCREEN ***/

extern bool inGame;
extern bool wasInGame;

UIScreen *inGameScreen = NULL;

UIScreen *gameScene (void) {

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

    if (inGameScreen == NULL) inGameScreen = (UIScreen *) malloc (sizeof (UIScreen));
    
    inGameScreen->views = igViews;
    inGameScreen->activeView = mapView;
    inGameScreen->handleEvent = hanldeGameEvent;

    playerComp = (Player *) getComponent (player, PLAYER);

    wallsFadedColor = COLOR_FROM_RGBA (RED (wallsFgColor), GREEN (wallsFgColor), BLUE (wallsFgColor), 0x77);

    inGame = true;
    wasInGame = true;

    return inGameScreen;

}

/*** CLEAN UP ***/

void cleanGameUI (void) {

    if (inGameScreen != NULL) {
        hideInventory ();

        destroyLootWindow ();

        ListElement *view = getListElement (inGameScreen->views, activeScene);
        destroyView ((UIView *) removeElement (inGameScreen->views, view));
        inGameScreen->activeView = NULL;

        for (ListElement *e = LIST_START (inGameScreen->views); e != NULL; e = e->next) 
            destroyView ((UIView *) e->data);
        
        destroyList (inGameScreen->views);

        free (inGameScreen);
    }

}
