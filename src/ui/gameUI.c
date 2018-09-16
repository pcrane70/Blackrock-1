/*** This file handles the in game UI and the input for playing the game ***/

#include <string.h>     // for message functions

#include "blackrock.h"
#include "game.h"
#include "item.h"
#include "player.h"
#include "map.h"    // for walls array

#include "ui/ui.h"
#include "ui/console.h"
#include "ui/gameUI.h"

#include "input.h"

#include "utils/list.h"       // for messages
#include "objectPool.h"

extern UIView *activeView;

/*** STATS ***/

#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

char *statsPlayerName = NULL;

u8 layerRendered[MAP_WIDTH][MAP_HEIGHT];
extern u32 fovMap[MAP_WIDTH][MAP_HEIGHT];

u32 wallsFgColor = 0xFFFFFFFF;
u32 wallsBgColor = 0x000000FF;
u32 wallsFadedColor;
asciiChar wallGlyph = '#';  // 19/08/2018 -- 18:00 -- we are assuming that are walls are the same

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
    for (u8 layer = GROUND_LAYER; layer <= TOP_LAYER; layer++) {
        for (ListElement *ptr = LIST_START (gameObjects); ptr != NULL; ptr = ptr->next) {
            go = (GameObject *) ptr->data;
            p = (Position *) getComponent (go, POSITION);
            if (p != NULL && p->layer == layer) {
                g = getComponent (go, GRAPHICS);
                if (fovMap[p->x][p->y] > 0) {
                    g->hasBeenSeen = true;
                    putCharAt (console, g->glyph, p->x, p->y, g->fgColor, g->bgColor);
                    layerRendered[p->x][p->y] = p->layer;
                }

                else if (g->visibleOutsideFov && g->hasBeenSeen) {
                    fullColor = g->fgColor;
                    fadedColor = COLOR_FROM_RGBA (RED (fullColor), GREEN (fullColor), BLUE (fullColor), 0x77);
                    putCharAt (console, g->glyph, p->x, p->y, fadedColor, 0x000000FF);
                    layerRendered[p->x][p->y] = p->layer;
                }
            }
            
        }
    }    

    // 19/08/2018 -- 17:53 -- we are assuming all walls are visible outside fov
    for (u32 i = 0; i < wallCount; i++) {
        if (fovMap[walls[i].x][walls[i].y] > 0) {
            walls[i].hasBeenSeen = true;
            putCharAt (console, wallGlyph, walls[i].x, walls[i].y, wallsFgColor, wallsBgColor);
        }

        else if (walls[i].hasBeenSeen) 
            putCharAt (console, wallGlyph, walls[i].x, walls[i].y, wallsFadedColor, wallsBgColor);
        
    }

    // FIXME: i dont like this!!
    Item *item = NULL;
    Position *itemPos = NULL;
    Graphics *itemGra = NULL;
    for (ListElement *e = LIST_START (items); e != NULL; e = e->next) {
        item = (Item *) e->data;
        itemPos = getGameComponent (item, POSITION);
        if (itemPos != NULL) {
            itemGra = getGameComponent (item, GRAPHICS);

            if (fovMap[itemPos->x][itemPos->y] > 0) {
                itemGra->hasBeenSeen = true;
                putCharAt (console, itemGra->glyph, itemPos->x, itemPos->y, itemGra->fgColor, itemGra->bgColor);
                layerRendered[itemPos->x][itemPos->y] = itemPos->layer;
            }

            else if (itemGra->visibleOutsideFov && itemGra->hasBeenSeen) {
                fullColor = itemGra->fgColor;
                fadedColor = COLOR_FROM_RGBA (RED (fullColor), GREEN (fullColor), BLUE (fullColor), 0x77);
                putCharAt (console, g->glyph, itemPos->x, itemPos->y, fadedColor, 0x000000FF);
                layerRendered[itemPos->x][itemPos->y] = itemPos->layer;
            }
        }
    }
        
    // render the player
    putCharAt (console, player->graphics->glyph, player->pos->x, player->pos->y, 
        player->graphics->fgColor, player->graphics->bgColor);

}

// FIXME: create a more efficient way
static void rednderStats (Console *console) {

    UIRect rect = { 0, 0, STATS_WIDTH, STATS_HEIGHT };
    drawRect (console, &rect, 0x222222FF, 0, 0xFF990099);

    putStringAt (console, statsPlayerName, 0, 0, 0xFFFFFFFF, 0x00000000);

    int currHealth = player->combat->baseStats.health;
    int maxHealth = player->combat->baseStats.maxHealth;
    char *str = createString ("HP: %i/%i", currHealth, maxHealth);

    if (currHealth >= (maxHealth * 0.75)) 
        putStringAt (console, str, 0, 1, SUCCESS_COLOR, 0x00000000);

    else if ((currHealth < (maxHealth * 0.75)) && (currHealth >= (maxHealth * 0.25)))
        putStringAt (console, str, 0, 1, 0xFF990099, 0x00000000);

    else putStringAt (console, str, 0, 1, WARNING_COLOR, 0x00000000);

    free (str);

}

/*** MESSAGE LOG ***/

#define LOG_WIDTH		60
#define LOG_HEIGHT		5

// TODO: after a while of inactivity, vanish the log until new activity -- just for astethics

List *messageLog = NULL;

void deleteMessage (Message *msg) {

    if (msg != NULL) {
        free (msg->msg);
        free (msg);
    }

}

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

    // remove the oldest message
    if (LIST_SIZE (messageLog) > 15)
        deleteMessage ((Message *) removeElement (messageLog, NULL));

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

void cleanMessageLog (void) {

    while (LIST_SIZE (messageLog) > 0) 
        deleteMessage ((Message *) removeElement (messageLog, NULL));

    free (messageLog);

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

List *activeLootRects = NULL;
Pool *lootRectsPool = NULL;

LootRect *createLootRect (u8 y, Item *i) {

    LootRect *new = NULL;

    if (POOL_SIZE (lootRectsPool) > 0) {
        new = (LootRect *) pop (lootRectsPool);
        if (new == NULL) {
            new = (LootRect *) malloc (sizeof (LootRect));
            new->bgRect = (UIRect *) malloc (sizeof (UIRect));
            new->imgRect = (UIRect *) malloc (sizeof (UIRect));
        }
    }

    else {
        new = (LootRect *) malloc (sizeof (LootRect));
        new->bgRect = (UIRect *) malloc (sizeof (UIRect));
        new->imgRect = (UIRect *) malloc (sizeof (UIRect));
    }

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

void destroyLootRects (void) {

    LootRect *lr = NULL;

    if (activeLootRects != NULL) {
        if (LIST_SIZE (activeLootRects) > 0) {
            for (ListElement *e = LIST_START (activeLootRects); e != NULL; e = e->next) {
                lr = (LootRect *) e->data;
                if (lr->bgRect != NULL) free (lr->bgRect);
                if (lr->imgRect != NULL) free (lr->imgRect);
                lr->item = NULL;
            }
        }
        
        destroyList (activeLootRects);
    } 

    if (lootRectsPool != NULL) {
        if (POOL_SIZE (lootRectsPool) > 0) {
            for (PoolMember *e = POOL_TOP (lootRectsPool); e != NULL; e = e->next) {
                lr = (LootRect *) e->data;
                if (lr->bgRect != NULL) free (lr->bgRect);
                if (lr->imgRect != NULL) free (lr->imgRect);
                lr->item = NULL;
            }
        }

        clearPool (lootRectsPool);
    } 

}

void drawLootRect (Console *console, LootRect *rect, u32 bgColor) {

    drawRect (console, rect->bgRect, bgColor, 0, 0x00000000);
    drawRect (console, rect->imgRect, 0x000000FF, 0, 0x00000000);

    Graphics *g = (Graphics *) getGameComponent (rect->item, GRAPHICS);
    if (g != NULL)        
        putStringAt (console, g->name, 7, (rect->bgRect->y) + 2,
            getItemColor (rect->item->rarity), 0x00000000);

}

u8 lootYIdx = 0;

void updateLootUI (u8 yIdx) {

    u8 count = 0;
    if (activeLootRects != NULL && (LIST_SIZE (activeLootRects) > 0)) {
        for (ListElement *e = LIST_START (activeLootRects); e != NULL; e = e->next) {
            if (count == yIdx) {
                push (lootRectsPool, removeElement (activeLootRects, e));
                break;
            }

            count ++;
        }
    }

    if (lootYIdx >= LIST_SIZE (activeLootRects)) lootYIdx -= 1;

}

void renderLootRects (Console *console) {

    u8 count = 0;
    for (ListElement *e = LIST_START (activeLootRects); e != NULL; e = e->next) {
        if (count == lootYIdx) drawLootRect (console, (LootRect *) e->data, 0x000000FF);
        else drawLootRect (console, (LootRect *) e->data, 0xFFFFFFFF);
        
        count++;
    }

}

static void renderLoot (Console *console) {

    if (currentLoot != NULL) {
        UIRect looRect = { 0, 0, LOOT_WIDTH, LOOT_HEIGHT };
        drawRect (console, &looRect, LOOT_COLOR, 0, 0xFF990099);

        putStringAt (console, "Loot", 8, 2, LOOT_TEXT, 0x00000000);

        if ((currentLoot->lootItems != NULL) && (LIST_SIZE (currentLoot->lootItems) > 0))
            renderLootRects (console);   

        // gold
        char *gold = createString ("%ig - %is - %ic", currentLoot->money[0], currentLoot->money[1], currentLoot->money[2]);
        putStringAt (console, gold, 3, 21, LOOT_TEXT, 0x00000000);
        free (gold);
    }

}

void toggleLootWindow (void) {

    if (lootView == NULL) {
        UIRect lootRect = { (16 * LOOT_LEFT), (16 * LOOT_TOP), (16 * LOOT_WIDTH), (16 * LOOT_HEIGHT) };
        lootView = newView (lootRect, LOOT_WIDTH, LOOT_HEIGHT, tileset, 0, 0x000000FF, true, renderLoot);
        insertAfter (activeScene->views, LIST_END (activeScene->views), lootView);

        lootYIdx = 0;

        if (currentLoot->lootItems != NULL && LIST_SIZE (currentLoot->lootItems) > 0) {
            LootRect *lr = NULL;
            u8 y = 0;
            for (ListElement *e = LIST_START (currentLoot->lootItems); e != NULL; e = e->next) {
                lr = createLootRect (y, (Item *) e->data);
                insertAfter (activeLootRects, LIST_END (activeLootRects), lr);
                y++;
            }
        }

        activeView = lootView;
    } 
    
    else {
        ListElement *e = getListElement (activeScene->views, lootView);
        destroyView ((UIView *) removeElement (activeScene->views, e));
        lootView = NULL;

        // deactivate the loot rects and send them to the pool
        if (activeLootRects != NULL && LIST_SIZE (activeLootRects) > 0) {
            for (ListElement *e = LIST_START (activeLootRects); e != NULL; e = e->next) 
                push (lootRectsPool, removeElement (activeLootRects, e));

            resetList (activeLootRects);
        }

        activeView = (UIView *) (LIST_END (activeScene->views))->data;
    } 

}

/*** INVENTORY ***/

#define INVENTORY_LEFT		    20
#define INVENTORY_TOP		    7
#define INVENTORY_DUAL_LEFT     37

#define INVENTORY_WIDTH		40
#define INVENTORY_HEIGHT	30

#define INVENTORY_CELL_WIDTH    4
#define INVENTORY_CELL_HEIGHT   4

#define INVENTORY_COLOR     0xCC8E35FF
#define INVENTORY_TEXT      0xEEEEEEFF

#define INVENTORY_CELL_COLOR    0xD1C7B8FF
#define INVENTORY_SELECTED      0x847967FF

#define ZERO_ITEMS      48

#define INV_DESC_BOX_WIDTH      34
#define INV_DESC_BOX_HEIGHT     3

UIView *inventoryView = NULL;

u8 inventoryXIdx = 0;
u8 inventoryYIdx = 0;

typedef struct {

    u8 xIdx, yIdx;
    UIRect *bgRect;
    UIRect *imgRect;
    Item *item;

} ItemRect;

ItemRect ***inventoryRects = NULL;

ItemRect *createInvRect (u8 x, u8 y) {

    ItemRect *new = (ItemRect *) malloc (sizeof (ItemRect));

    new->bgRect = (UIRect *) malloc (sizeof (UIRect));
    new->bgRect->w = INVENTORY_CELL_WIDTH;
    new->bgRect->h = INVENTORY_CELL_HEIGHT;
    // new->imgRect = (UIRect *) malloc (sizeof (UIRect));
    // new->imgRect->w = INVENTORY_CELL_WIDTH;
    // new->imgRect->h = INVENTORY_CELL_HEIGHT;
    new->imgRect = NULL;
    new->item = NULL;

    new->xIdx = x;
    new->yIdx = y;
    new->bgRect->x = x + 3 + (INVENTORY_CELL_WIDTH * x);
    new->bgRect->y = y + 5 + (INVENTORY_CELL_HEIGHT * y);
    // new->imgRect->x = x + 3 + (INVENTORY_CELL_WIDTH * x);
    // new->imgRect->y = y + 5 + (INVENTORY_CELL_HEIGHT * y);

    return new;

}

ItemRect ***initInventoryRects (void) {

    ItemRect ***invRects = (ItemRect ***) malloc (7 * sizeof (ItemRect **));
    for (u8 i = 0; i < 7; i++)
        invRects[i] = (ItemRect **) malloc (3 * sizeof (ItemRect *));

    for (u8 y = 0; y < 3; y++) 
        for (u8 x = 0; x < 7; x++) 
            invRects[x][y] = createInvRect (x, y);

    return invRects;

}

void resetInventoryRects (void) {

    for (u8 y = 0; y < 3; y++) 
        for (u8 x = 0; x < 7; x++)
            inventoryRects[x][y]->item = NULL;

    // display the items that are currently on the players inventory
    for (u8 y = 0; y < 3; y++) {
        for (u8 x = 0; x < 7; x++) {
            if (player->inventory[x][y] != NULL)
                inventoryRects[x][y]->item = player->inventory[x][y];

        }
    }

}

void destroyInvRects (void) {

    for (u8 y = 0; y < 3; y++) {
        for (u8 x = 0; x < 7; x++) {
            free (inventoryRects[x][y]->bgRect);
            // free (inventoryRects[x][y]->imgRect);
            inventoryRects[x][y]->item = NULL;
            free (inventoryRects[x][y]);
        }
    }

    free (inventoryRects);

}

// TODO: draw here the item image
void renderInventoryItems (Console *console) {

    ItemRect *invRect = NULL;

    // draw inventory cells
    for (u8 y = 0; y < 3; y++) {
        for (u8 x = 0; x < 7; x++) {
            invRect = inventoryRects[x][y];
    
             // draw highlighted rect
            if (inventoryXIdx == invRect->xIdx && inventoryYIdx == invRect->yIdx) {
                drawRect (console, invRect->bgRect, INVENTORY_SELECTED, 0, 0x000000FF);
                // drawRect (console, invRect->imgRect, INVENTORY_SELECTED, 0, 0x00000000);
                if (invRect->item != NULL) {
                    // drawImageAt (console, apple, invRect->imgRect->x, invRect->imgRect->y);
                    Graphics *g = (Graphics *) getGameComponent (invRect->item, GRAPHICS);
                    if (g != NULL) 
                        putStringAt (console, g->name, 5, 22, getItemColor (invRect->item->rarity), 0x00000000);

                    u8 quantity = ZERO_ITEMS + invRect->item->quantity;
                    // putCharAt (console, quantity, invRect->imgRect->x, invRect->imgRect->y, 0xFFFFFFFF, 0x00000000);
                    putCharAt (console, quantity, invRect->bgRect->x, invRect->bgRect->y, 0xFFFFFFFF, 0x00000000);
                }
            }

            // draw every other rect with an item on it
            else if (invRect->item != NULL) {
                drawRect (console, invRect->bgRect, INVENTORY_CELL_COLOR, 0, 0x000000FF);

                u8 quantity = ZERO_ITEMS + invRect->item->quantity;
                putCharAt (console, quantity, invRect->bgRect->x, invRect->bgRect->y, 0xFFFFFFFF, 0x00000000);
            }

            // draw the empty rects
            else drawRect (console, invRect->bgRect, INVENTORY_CELL_COLOR, 0, 0x000000FF);
        }
    }

} 

static void renderInventory (Console *console) {

    UIRect rect = { 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT };
    drawRect (console, &rect, INVENTORY_COLOR, 0, 0xFFFFFFFF);

    putStringAt (console, "Inventory", 16, 2, INVENTORY_TEXT, 0x00000000);

    // draw item description rect
    UIRect descBox = { 3, 21, INV_DESC_BOX_WIDTH, INV_DESC_BOX_HEIGHT };
    drawRect (console, &descBox, INVENTORY_CELL_COLOR, 0, 0x00000000);

    // render items
    renderInventoryItems (console);
    
    // gold
    char *gold = createString ("%ig - %is - %ic", player->money[0], player->money[1], player->money[2]);
    putStringAt (console, gold, 13, 25, INVENTORY_TEXT, 0x00000000);

    putStringAt (console, "[wasd] to move", 4, 27, INVENTORY_TEXT, 0x00000000);
    putStringAt (console, "[e] to use, [Spc] to drop", 4, 28, INVENTORY_TEXT, 0x00000000);
    // free (weightInfo);
    free (gold);

}

void updateCharacterPos (bool);

void showInventory (bool dual) {

    if (dual) {
        UIRect inv = { (16 * INVENTORY_DUAL_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT) };
        inventoryView = newView (inv, INVENTORY_WIDTH, INVENTORY_HEIGHT, tileset, 0, 0x000000FF, true, renderInventory);
    }

    else {
        UIRect inv = { (16 * INVENTORY_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT) };
        inventoryView = newView (inv, INVENTORY_WIDTH, INVENTORY_HEIGHT, tileset, 0, 0x000000FF, true, renderInventory);
    }

    insertAfter (activeScene->views, LIST_END (activeScene->views), inventoryView);

    if (inventoryRects == NULL) {
        inventoryRects = initInventoryRects ();
        resetInventoryRects ();
    } 

    else resetInventoryRects ();

}

void hideInventory (void) {

    if (inventoryView != NULL) {
        ListElement *inv = getListElement (activeScene->views, inventoryView);
        destroyView ((UIView *) removeElement (activeScene->views, inv));
        inventoryView = NULL;
    }

}

void updateInventoryPos (bool dual) {

    if (inventoryView != NULL) {
        hideInventory ();
        showInventory (dual);
    }

}

void toggleInventory (void) {

    if (inventoryView == NULL) {
        if (characterView != NULL) {
            updateCharacterPos (true);
            showInventory (true);
        }

        else showInventory (false);

        activeView = inventoryView;
    } 

    else {
        if (characterView != NULL) updateCharacterPos (false);

        hideInventory ();

        activeView = (UIView *) (LIST_END (activeScene->views))->data;
    }
    
}

Item *getInvSelectedItem (void) { return inventoryRects[inventoryXIdx][inventoryYIdx]->item; }

/*** CHARACTER ***/

#define CHARACTER_LEFT		    25
#define CHARACTER_TOP		    2
#define CHARACTER_DUAL_LEFT     3

#define CHARACTER_WIDTH		29
#define CHARACTER_HEIGHT	40

#define CHARACTER_CELL_WIDTH    4
#define CHARACTER_CELL_HEIGHT   4

#define CHARACTER_COLOR     0x4B6584FF
#define CHARACTER_TEXT      0xEEEEEEFF

#define CHARACTER_CELL_COLOR    0xD1C7B8FF
#define CHARACTER_SELECTED      0x847967FF

#define CHAR_DESC_BOX_WIDTH     25
#define CHAR_DESC_BOX_HEIGHT    3

UIView *characterView = NULL;

ItemRect ***characterRects = NULL;

u8 characterXIdx = 0;
u8 characterYIdx = 0;

Item *getCharSelectedItem (void) { return characterRects[characterXIdx][characterYIdx]->item; }

ItemRect *createCharRect (u8 x, u8 y) {

    ItemRect *new = (ItemRect *) malloc (sizeof (ItemRect));

    new->bgRect = (UIRect *) malloc (sizeof (UIRect));
    new->bgRect->w = CHARACTER_CELL_WIDTH;
    new->bgRect->h = CHARACTER_CELL_HEIGHT;
    // new->imgRect = (UIRect *) malloc (sizeof (UIRect));
    // new->imgRect->w = INVENTORY_CELL_WIDTH;
    // new->imgRect->h = INVENTORY_CELL_HEIGHT;
    new->imgRect = NULL;
    new->item = NULL;

    new->xIdx = x;
    new->yIdx = y;

    new->bgRect->y = y + 5 + (CHARACTER_CELL_HEIGHT * y);
    // new->imgRect->y = y + 5 + (INVENTORY_CELL_HEIGHT * y);

    // armour
    if (y != 5) {
        if (x == 1) {
            new->bgRect->x = ((CHARACTER_LEFT + CHARACTER_WIDTH)) - 2;
            // new->imgRect->x = new->bgRect->x;
        }

        else {
            new->bgRect->x = x + 2 + (CHARACTER_CELL_WIDTH * x);
            // new->imgRect->x = new->bgRect->x;
        }
    }

    // weapons
    else {
        if (x == 0) {
            new->bgRect->x = 9;
            // new->imgRect->x = new->bgRect->x;
        }

        else {
            new->bgRect->x = 16;
            // new->imgRect->x = new->bgRect->x;
        }
        
    }

    return new;

}

ItemRect ***initCharacterRects (void) {

    ItemRect ***charRects = (ItemRect ***) malloc (2 * sizeof (ItemRect **));
    for (u8 i = 0; i < 2; i++)
        charRects[i] = (ItemRect **) malloc (6 * sizeof (ItemRect *));

    for (u8 y = 0; y < 6; y++) 
        for (u8 x = 0; x < 2; x++) 
            charRects[x][y] = createCharRect (x, y);
            
    return charRects;

}

void destroyCharRects (void) {

    for (u8 y = 0; y < 6; y++) {
        for (u8 x = 0; x < 2; x++) {
            if (characterRects[x][y] != NULL) {
                free (characterRects[x][y]->bgRect);
                // free (characterRects[x][y]->imgRect);
                characterRects[x][y]->item = NULL;
                free (characterRects[x][y]);
            }
        }
    }

    free (characterRects);

}

// FIXME:
void resetCharacterRects (void) {

    for (u8 y = 0; y < 6; y++) 
        for (u8 x = 0; x < 2; x++) 
            characterRects[x][y]->item = NULL;

    // FIXME:
    // equipment
    // for (u8 y = 0; y < 5; y++) 
    //     for (u8 x = 0; x < 2; x++) 
    //         characterRects[x][y]->item = 

    // weapons
    for (u8 i = 0; i < 2; i++) characterRects[i][5]->item = player->weapons[i];

}

// TODO: draw images
void renderCharacterRects (Console *console) {

    ItemRect *charRect = NULL;

    // draw inventory cells
    for (u8 y = 0; y < 6; y++) {
        for (u8 x = 0; x < 2; x++) {
            charRect = characterRects[x][y];
    
             // draw highlighted rect
            if (characterXIdx == charRect->xIdx && characterYIdx == charRect->yIdx) {
                drawRect (console, charRect->bgRect, CHARACTER_SELECTED, 0, 0x000000FF);
                // drawRect (console, invRect->imgRect, INVENTORY_SELECTED, 0, 0x00000000);
                // if (charRect->item != NULL) {
                //     // drawImageAt (console, apple, invRect->imgRect->x, invRect->imgRect->y);
                //     // Graphics *g = (Graphics *) getGameComponent (invRect->item, GRAPHICS);
                //     // if (g != NULL) 
                //     //     putStringAt (console, g->name, 5, 22, getItemColor (invRect->item->rarity), 0x00000000);
                // }
            }

            // draw every other rect with an item on it
            else if (charRect->item != NULL) 
                drawRect (console, charRect->bgRect, CHARACTER_CELL_COLOR, 0, 0x000000FF);

            // draw the empty rects
            else drawRect (console, charRect->bgRect, CHARACTER_CELL_COLOR, 0, 0x000000FF);
        }
    }

}

// I don't like this function...
char *getCharRectSlot (void) {

    char slot[15];

    if (characterXIdx == 0 && characterYIdx == 0) strcpy (slot, "Head");
    else if (characterXIdx == 0 && characterYIdx == 1) strcpy (slot, "Necklace");
    else if (characterXIdx == 0 && characterYIdx == 2) strcpy (slot, "Shoulders");
    else if (characterXIdx == 0 && characterYIdx == 3) strcpy (slot, "Cape");
    else if (characterXIdx == 0 && characterYIdx == 4) strcpy (slot, "Chest");
    else if (characterXIdx == 1 && characterYIdx == 0) strcpy (slot, "Hands");
    else if (characterXIdx == 1 && characterYIdx == 1) strcpy (slot, "Waist");
    else if (characterXIdx == 1 && characterYIdx == 2) strcpy (slot, "Legs");
    else if (characterXIdx == 1 && characterYIdx == 3) strcpy (slot, "Shoes");
    else if (characterXIdx == 1 && characterYIdx == 4) strcpy (slot, "Ring");

    else if (characterXIdx == 0 && characterYIdx == 5) strcpy (slot, "Main");
    else if (characterXIdx == 1 && characterYIdx == 5) strcpy (slot, "Off");

    char *retVal = (char *) calloc (strlen (slot), sizeof (char));
    strcpy (retVal, slot);

    return retVal;

}

static void renderCharacter (Console *console) {

    UIRect rect = { 0, 0, CHARACTER_WIDTH, CHARACTER_HEIGHT };
    drawRect (console, &rect, CHARACTER_COLOR, 0, 0xFFFFFFFF);

    // render character info
    putStringAt (console, statsPlayerName, 6, 2, CHARACTER_TEXT, 0x00000000);

    renderCharacterRects (console);

    // draw item description rect
    UIRect descBox = { 2, 36, CHAR_DESC_BOX_WIDTH, CHAR_DESC_BOX_HEIGHT };
    drawRect (console, &descBox, INVENTORY_CELL_COLOR, 0, 0x00000000);
    Item *selected = getCharSelectedItem ();
    if (selected != NULL) {
        Graphics *g = (Graphics *) getGameComponent (selected, GRAPHICS);
        if (g != NULL) {
            char *slot = getItemSlot (selected);
            // FIXME: add dynamic color strings
            char *str = createString ("%s: %s", slot, g->name);
            putStringAt (console, str, 3, 37, getItemColor (selected->rarity), 0x00000000);
            free (slot);
            free (str);
        }
    }

    // the slot is empty
    else {
        char *slot = getCharRectSlot ();
        // FIXME: CHANGE COLOR
        putStringAt (console, slot, 3, 37, 0x000000FF, 0x00000000);
        free (slot);
    }
    
}

void showCharacter (bool dual) {

    if (dual) {
        UIRect c = { (16 * CHARACTER_DUAL_LEFT), (16 * CHARACTER_TOP), (16 * CHARACTER_WIDTH), (16 * CHARACTER_HEIGHT) };
        characterView = newView (c, CHARACTER_WIDTH, CHARACTER_HEIGHT, tileset, 0, 0x000000FF, true, renderCharacter);
    }

    else {
        UIRect c = { (16 * CHARACTER_LEFT), (16 * CHARACTER_TOP), (16 * CHARACTER_WIDTH), (16 * CHARACTER_HEIGHT) };
        characterView = newView (c, CHARACTER_WIDTH, CHARACTER_HEIGHT, tileset, 0, 0x000000FF, true, renderCharacter);
    }

    insertAfter(activeScene->views, LIST_END (activeScene->views), characterView);

    if (characterRects == NULL) {
        characterRects = initCharacterRects ();
        resetCharacterRects ();
    } 

    else resetCharacterRects ();

}

void hideCharacter () {

    if (characterView != NULL) {
        ListElement *c = getListElement (activeScene->views, characterView);
        destroyView ((UIView *) removeElement (activeScene->views, c));
        characterView = NULL;
    }

}

void updateCharacterPos (bool dual) {

    if (characterView != NULL) {
        if (dual) {
            hideCharacter (true);
            showCharacter (true);
        } 

        else {
            hideCharacter (false);
            showCharacter (false);
        } 
        
    }

}

void toggleCharacter (void) {

    if (characterView == NULL) {
        if (inventoryView != NULL) {
            updateInventoryPos (true);
            showCharacter (true);
        } 

        else showCharacter (false);

        activeView = characterView;
    } 

    else {
        if (inventoryView != NULL) updateInventoryPos (false);

        hideCharacter ();

        activeView = (UIView *) (LIST_END (activeScene->views))->data;
    } 
     
}

/*** PAUSE MENU ***/

#define PAUSE_LEFT		20
#define PAUSE_TOP		7
#define PAUSE_WIDTH		40
#define PAUSE_HEIGHT	30

#define PAUSE_COLOR     0x4B6584FF

UIView *pauseMenu = NULL;

static void renderPauseMenu (Console *console) {

    UIRect rect = { 0, 0, PAUSE_WIDTH, PAUSE_HEIGHT };
    drawRect (console, &rect, PAUSE_COLOR, 0, 0xFFFFFFFF);

    putStringAt (console, "Pause Menu", 15, 2, INVENTORY_TEXT, 0x00000000);

}

void togglePauseMenu (void) {

    if (pauseMenu == NULL) {
        UIRect pause = { (16 * PAUSE_LEFT), (16 * PAUSE_TOP), (16 * PAUSE_WIDTH), (16 * PAUSE_HEIGHT) };
        pauseMenu = newView (pause, PAUSE_WIDTH, PAUSE_HEIGHT, tileset, 0, 0x000000FF, true, renderPauseMenu);
        insertAfter(activeScene->views, LIST_END (activeScene->views), pauseMenu);

        activeView = pauseMenu;
    }

    else {
        if (pauseMenu != NULL) {
            ListElement *pause = getListElement (activeScene->views, pauseMenu);
            destroyView ((UIView *) removeElement (activeScene->views, pause));
            pauseMenu = NULL;

            activeView = (UIView *) (LIST_END (activeScene->views))->data;
        }
    }

}

/*** DEATH SCREEN ***/

BitmapImage *deathImg = NULL;
char *deathImgPath = "./resources/death-720.png"; 

UIView *deathScreen = NULL;

static void renderDeathScreen (Console *console) {

    if (deathImg == NULL) deathImg = loadImageFromFile (deathImgPath);

    drawImageAt (console, deathImg, 0, 0);

}

void toggleDeathScreen (void) {

    if (deathScreen == NULL) {
        UIRect bgRect = { 0, 0, (16 * FULL_SCREEN_WIDTH), (16 * FULL_SCREEN_HEIGHT) };
        deathScreen = newView (bgRect, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, tileset, 0, 0x000000FF, true, renderDeathScreen);
        insertAfter (activeScene->views, LIST_END (activeScene->views), deathScreen);

        activeView = deathScreen;
    }

    else {
        if (deathScreen != NULL) {
            ListElement *death = getListElement (activeScene->views, deathScreen);
            destroyView ((UIView *) removeElement (activeScene->views, death));
            deathScreen = NULL;

            activeView = (UIView *) (LIST_END (activeScene->views))->data;
        }
    }

}

/*** SCORE SCREEN ***/

BitmapImage *scoreImg = NULL;
char *scoreImgPath = "./resources/score-720.png"; 

UIView *scoreScreen = NULL;

static void renderScoreScreen (Console *console) {

    if (scoreImg == NULL) scoreImg = loadImageFromFile (scoreImgPath);

    drawImageAt (console, scoreImg, 0, 0);

}

void toggleScoreScreen (void) {

    if (scoreScreen == NULL) {
        UIRect bgRect = { 0, 0, (16 * FULL_SCREEN_WIDTH), (16 * FULL_SCREEN_HEIGHT) };
        scoreScreen = newView (bgRect, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, tileset, 0, 0x000000FF, true, renderScoreScreen);
        insertAfter (activeScene->views, LIST_END (activeScene->views), scoreScreen);

        activeView = scoreScreen;
    }

    else {
        if (scoreScreen != NULL) {
            ListElement *death = getListElement (activeScene->views, scoreScreen);
            destroyView ((UIView *) removeElement (activeScene->views, death));
            scoreScreen = NULL;

            activeView = (UIView *) (LIST_END (activeScene->views))->data;
        }
    }

}


/*** INIT GAME SCREEN ***/

UIScreen *inGameScreen = NULL;

UIView *mapView = NULL;

UIScreen *gameScene (void) {

    List *igViews = initList (NULL);

    UIRect mapRect = { 0, 0, (16 * MAP_WIDTH), (16 * MAP_HEIGHT) };
    bool colorize = true;
    u32 bgColor;

    colorize = true;
    bgColor = 0x000000FF;

    mapView = newView (mapRect, MAP_WIDTH, MAP_HEIGHT, tileset, 0, bgColor, colorize, renderMap);
    insertAfter (igViews, NULL, mapView);

    UIRect statsRect = { 0, (16 * MAP_HEIGHT), (16 * STATS_WIDTH), (16 * STATS_HEIGHT) };
    UIView *statsView = newView (statsRect, STATS_WIDTH, STATS_HEIGHT, tileset, 0, 0x000000FF, true, rednderStats);
    insertAfter (igViews, NULL, statsView);

    UIRect logRect = { (16 * 20), (16 * MAP_HEIGHT), (16 * LOG_WIDTH), (16 * LOG_HEIGHT) };
    UIView *logView = newView (logRect, LOG_WIDTH, LOG_HEIGHT, tileset, 0, 0x000000FF, true, renderLog);
    insertAfter (igViews, NULL, logView);

    if (inGameScreen == NULL) inGameScreen = (UIScreen *) malloc (sizeof (UIScreen));
    
    inGameScreen->views = igViews;
    // FIXME: do we need this?
    inGameScreen->activeView = mapView;
    inGameScreen->handleEvent = hanldeGameEvent;

    statsPlayerName = createString ("%s the %s", player->name, getPlayerClassName ());

    wallsFadedColor = COLOR_FROM_RGBA (RED (wallsFgColor), GREEN (wallsFgColor), BLUE (wallsFgColor), 0x77);

    inventoryRects = initInventoryRects ();
    characterRects = initCharacterRects ();

    activeLootRects = initList (free);
    lootRectsPool = initPool ();

    activeView = mapView;

    return inGameScreen;

}

/*** CLEAN UP ***/

void cleanGameUI (void) {

    if (inGameScreen != NULL) {
        // message log
        cleanMessageLog ();

        if (inventoryRects != NULL) destroyInvRects ();
        if (characterRects != NULL) destroyCharRects ();

        destroyLootRects ();

        if (deathImg != NULL) destroyImage (deathImg);

        for (ListElement *e = LIST_START (inGameScreen->views); e != NULL; e = e->next) 
            destroyView ((UIView *) e->data);
        
        destroyList (inGameScreen->views);

        free (inGameScreen);

        fprintf (stdout, "Done cleanning up game UI.\n");
    }

}
