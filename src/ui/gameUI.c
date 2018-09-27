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

#include "utils/myUtils.h"

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

    putStringAt (console, statsPlayerName, 0, 0, 0xFFFFFFFF, NO_COLOR);

    int currHealth = player->combat->baseStats.health;
    int maxHealth = player->combat->baseStats.maxHealth;
    char *str = createString ("HP: %i/%i", currHealth, maxHealth);

    if (currHealth >= (maxHealth * 0.75)) 
        putStringAt (console, str, 0, 1, SUCCESS_COLOR, NO_COLOR);

    else if ((currHealth < (maxHealth * 0.75)) && (currHealth >= (maxHealth * 0.25)))
        putStringAt (console, str, 0, 1, 0xFF990099, NO_COLOR);

    else putStringAt (console, str, 0, 1, WARNING_COLOR, NO_COLOR);

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

/*** LOOT ***/

#define LOOT_LEFT               26
#define LOOT_DUAL_INV_LEFT      4
#define LOOT_DUAL_TOOL_LEFT     9
#define LOOT_TOP                9
#define LOOT_WIDTH              28
#define LOOT_HEIGHT             24

#define LOOT_COLOR     0x69777DFF
#define LOOT_TEXT      0xEEEEEEFF

UIView *lootView = NULL;

extern Loot *currentLoot;

#define LOOT_RECT_WIDTH     26
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

    drawRect (console, rect->bgRect, bgColor, 0, NO_COLOR);
    drawRect (console, rect->imgRect, NO_COLOR, 0, NO_COLOR);

    Graphics *g = (Graphics *) getGameComponent (rect->item, GRAPHICS);
    if (g != NULL)        
        putStringAt (console, g->name, 7, (rect->bgRect->y) + 2,
            getItemColor (rect->item->rarity), NO_COLOR);

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
        if (count == lootYIdx) drawLootRect (console, (LootRect *) e->data, BLACK);
        else drawLootRect (console, (LootRect *) e->data, WHITE);
        
        count++;
    }

}

static void renderLoot (Console *console) {

    if (currentLoot != NULL) {
        UIRect looRect = { 0, 0, LOOT_WIDTH, LOOT_HEIGHT };
        drawRect (console, &looRect, LOOT_COLOR, 1, 0xFF990099);

        putStringAt (console, "Loot", 12, 2, LOOT_TEXT, 0x00000000);

        if ((currentLoot->lootItems != NULL) && (LIST_SIZE (currentLoot->lootItems) > 0))
            renderLootRects (console);   

        // gold
        char *gold = createString ("%ig - %is - %ic", currentLoot->money[0], currentLoot->money[1], currentLoot->money[2]);
        putStringAt (console, gold, 8, 21, LOOT_TEXT, 0x00000000);
        free (gold);
    }

}

void hideLoot (void) {

    ListElement *e = getListElement (activeScene->views, lootView);
    destroyView ((UIView *) removeElement (activeScene->views, e));
    lootView = NULL;

    // deactivate the loot rects and send them to the pool
    if (activeLootRects != NULL && LIST_SIZE (activeLootRects) > 0) {
        for (ListElement *e = LIST_START (activeLootRects); e != NULL; e = e->next) 
            push (lootRectsPool, removeElement (activeLootRects, e));

        resetList (activeLootRects);
    }

}

void showLoot (bool dual) {

    if (dual) {
        // tooltip
        if (tooltipView != NULL) {
            UIRect lootRect = { (16 * LOOT_DUAL_TOOL_LEFT), (16 * LOOT_TOP), (16 * LOOT_WIDTH), (16 * LOOT_HEIGHT) };
            lootView = newView (lootRect, LOOT_WIDTH, LOOT_HEIGHT, tileset, 0, 0x000000FF, true, renderLoot);
        }

        // with inventory
        else {
            UIRect lootRect = { (16 * LOOT_DUAL_INV_LEFT), (16 * LOOT_TOP), (16 * LOOT_WIDTH), (16 * LOOT_HEIGHT) };
            lootView = newView (lootRect, LOOT_WIDTH, LOOT_HEIGHT, tileset, 0, 0x000000FF, true, renderLoot);
        }
    }

    else {
        UIRect lootRect = { (16 * LOOT_LEFT), (16 * LOOT_TOP), (16 * LOOT_WIDTH), (16 * LOOT_HEIGHT) };
        lootView = newView (lootRect, LOOT_WIDTH, LOOT_HEIGHT, tileset, 0, 0x000000FF, true, renderLoot);
    }

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

void updateLootPos (bool dual) {

    if (lootView != NULL) {
        hideLoot ();
        showLoot (dual);
    }

}

void updateInventoryPos (bool);
void updateCharacterPos (bool);

void toggleLootWindow (void) {

    if (lootView == NULL) showLoot (false);
    
    else {
        if (inventoryView != NULL) updateInventoryPos (false);
        else if (characterView != NULL) updateCharacterPos (false);

        hideLoot ();

        activeView = (UIView *) (LIST_END (activeScene->views))->data;
    } 

}

Item *getSelectedLootItem (void) {

    Item *retVal = NULL;

    u8 count = 0;
    for (ListElement *e = LIST_START (activeLootRects); e != NULL; e = e->next) {
        if (count == lootYIdx) {
            LootRect *lr = (LootRect *) e->data;
            if (lr->item != NULL) {
                retVal = lr->item;
                break;
            } 
        }

        count++;
    }

    return retVal;

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

// TODO: image rects
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
                drawRect (console, invRect->bgRect, INVENTORY_SELECTED, 0, NO_COLOR);
                // drawRect (console, invRect->imgRect, INVENTORY_SELECTED, 0, 0x00000000);
                if (invRect->item != NULL) {
                    // drawImageAt (console, apple, invRect->imgRect->x, invRect->imgRect->y);
                    Graphics *g = (Graphics *) getGameComponent (invRect->item, GRAPHICS);
                    if (g != NULL) 
                        putStringAt (console, g->name, 5, 22, getItemColor (invRect->item->rarity), NO_COLOR);

                    u8 quantity = ZERO_ITEMS + invRect->item->quantity;
                    // putCharAt (console, quantity, invRect->imgRect->x, invRect->imgRect->y, 0xFFFFFFFF, 0x00000000);
                    putCharAt (console, quantity, invRect->bgRect->x, invRect->bgRect->y, WHITE, NO_COLOR);
                }
            }

            // draw every other rect with an item on it
            else if (invRect->item != NULL) {
                drawRect (console, invRect->bgRect, INVENTORY_CELL_COLOR, 0, NO_COLOR);

                u8 quantity = ZERO_ITEMS + invRect->item->quantity;
                putCharAt (console, quantity, invRect->bgRect->x, invRect->bgRect->y, WHITE, NO_COLOR);
            }

            // draw the empty rects
            else drawRect (console, invRect->bgRect, INVENTORY_CELL_COLOR, 0, NO_COLOR);
        }
    }

} 

static void renderInventory (Console *console) {

    UIRect rect = { 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT };
    drawRect (console, &rect, INVENTORY_COLOR, 0, NO_COLOR);

    putStringAt (console, "Inventory", 16, 2, INVENTORY_TEXT, NO_COLOR);

    // draw item description rect
    UIRect descBox = { 3, 21, INV_DESC_BOX_WIDTH, INV_DESC_BOX_HEIGHT };
    drawRect (console, &descBox, INVENTORY_CELL_COLOR, 0, NO_COLOR);

    // render items
    renderInventoryItems (console);
    
    // gold
    char *gold = createString ("%ig - %is - %ic", player->money[0], player->money[1], player->money[2]);
    putStringAt (console, gold, 13, 25, INVENTORY_TEXT, NO_COLOR);

    putStringAt (console, "[wasd] to move", 4, 27, INVENTORY_TEXT, NO_COLOR);
    putStringAt (console, "[e] to use, [Spc] to drop", 4, 28, INVENTORY_TEXT, NO_COLOR);
    free (gold);

}

void updateCharacterPos (bool);

void showInventory (bool dual) {

    if (lootView != NULL) dual = true;

    if (dual) {
        UIRect inv = { (16 * INVENTORY_DUAL_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT) };
        inventoryView = newView (inv, INVENTORY_WIDTH, INVENTORY_HEIGHT, tileset, 0, NO_COLOR, true, renderInventory);
    }

    else {
        UIRect inv = { (16 * INVENTORY_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT) };
        inventoryView = newView (inv, INVENTORY_WIDTH, INVENTORY_HEIGHT, tileset, 0, NO_COLOR, true, renderInventory);
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
        if (tooltipView != NULL) {
            if (lootView != NULL) toggleTooltip (true);
            else toggleTooltip (false);
        } 

        if (lootView != NULL) {
            updateLootPos (true);
            showInventory (true);
        }

        else if (characterView != NULL) {
            updateCharacterPos (true);
            showInventory (true);
        }

        else showInventory (false);

        activeView = inventoryView;
    } 

    else {
        if (lootView != NULL) updateLootPos (false);
        else if (characterView != NULL) updateCharacterPos (false);

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

// TODO: image rects
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
            if (characterXIdx == charRect->xIdx && characterYIdx == charRect->yIdx) 
                drawRect (console, charRect->bgRect, CHARACTER_SELECTED, 0, NO_COLOR);

            // draw every other rect with an item on it
            else if (charRect->item != NULL) 
                drawRect (console, charRect->bgRect, CHARACTER_CELL_COLOR, 0, NO_COLOR);

            // draw the empty rects
            else drawRect (console, charRect->bgRect, CHARACTER_CELL_COLOR, 0, NO_COLOR);
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
        characterView = newView (c, CHARACTER_WIDTH, CHARACTER_HEIGHT, tileset, 0, NO_COLOR, true, renderCharacter);
    }

    else {
        UIRect c = { (16 * CHARACTER_LEFT), (16 * CHARACTER_TOP), (16 * CHARACTER_WIDTH), (16 * CHARACTER_HEIGHT) };
        characterView = newView (c, CHARACTER_WIDTH, CHARACTER_HEIGHT, tileset, 0, NO_COLOR, true, renderCharacter);
    }

    insertAfter (activeScene->views, LIST_END (activeScene->views), characterView);

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

/*** TOOLTIP ***/

#define TOOLTIP_LEFT           43
#define TOOLTIP_TOP            9
#define TOOLTIP_WIDTH          28
#define TOOLTIP_HEIGHT         24

#define TOOLTIP_COLOR     0x69777DFF
#define TOOLTIP_TEXT      0xEEEEEEFF

#define TTIP_CHAR_LEFT      3
#define TTIP_CHAR_RIGHT     57
#define TTIP_CHAR_WIDTH     20
#define TTIP_CHAR_HEIGHT    18

#define TTIP_INV_LEFT       10
#define TTIP_INV_WIDTH      20
#define TTIP_INV_HEIGHT     18

UIView *tooltipView = NULL;

u8 tooltip;

typedef struct {

    char *health;
    char *dps;
    char *armor;
    char *strength;
    char *stamina;
    char *agility;
    char *intellect;

} ItemStatsUI;

typedef struct {

    char *name;
    u32 rarityColor;
    char *typeName;
    char *valueTxt;
    bool isEquipment;
    bool isWeapon;
    Weapon *w;
    Armour *a;
    ItemStatsUI stats;
    char *lifeTxt;
    u32 lifeColor;

} UIItemData;

// Tooltips items
UIItemData *lootItemUI = NULL;
UIItemData *equippedItemUI = NULL;
UIItemData *iData = NULL;

// just to be sure...
UIItemData *newItemUIData (void) {

    UIItemData *data = (UIItemData *) malloc (sizeof (UIItemData));

    if (data != NULL) {
        data->name = NULL;
        data->valueTxt = NULL;
        data->typeName = NULL;

        data->w = NULL;
        data->a = NULL;

        data->stats.health = NULL;
        data->stats.dps = NULL;
        data->stats.armor = NULL;
        data->stats.strength = NULL;
        data->stats.stamina = NULL;
        data->stats.agility = NULL;
        data->stats.intellect = NULL;

        data->lifeTxt = NULL;
    }

    return data;

}

void cleanItemData (UIItemData *data) {

    if (data != NULL) {
        if (data->name) free (data->name);
        if (data->valueTxt) free (data->valueTxt);
        if (data->typeName) free (data->typeName);

        data->w = NULL;
        data->a = NULL;

        if (data->stats.health) free (data->stats.health);
        if (data->stats.dps) free (data->stats.dps);
        if (data->stats.armor) free (data->stats.armor);
        if (data->stats.strength) free (data->stats.strength);
        if (data->stats.stamina) free (data->stats.stamina);
        if (data->stats.agility) free (data->stats.agility);
        if (data->stats.intellect) free (data->stats.intellect);

        if (data->lifeTxt) free (data->lifeTxt);

        free (data);
    }

}

// FIXME: add more stats
UIItemData *getUIItemData (Item *item) {

    UIItemData *data = newItemUIData ();

    Graphics *g = (Graphics *) getGameComponent (item, GRAPHICS);
    data->w = (Weapon *) getItemComponent (item, WEAPON);
    data->a = (Armour *) getItemComponent (item, ARMOUR);

    if (data->w != NULL || data->a != NULL) {
        data->isEquipment = true;
        data->typeName = getEquipmentTypeName (item);
    } 
    else data->isEquipment = false;

    if (data->w != NULL) data->isWeapon = true;
    else data->isWeapon = false;

    data->name = (char *) calloc (strlen (g->name), sizeof (char));
    strcpy (data->name, g->name);

    data->rarityColor = getItemColor (item->rarity);

    data->valueTxt = createString ("%ig - %is - %ic", item->value[0], item->value[1], item->value[2]);

    // FIXME:
    // get the item stats modifiers modifiers, this is for a food or a potion

    if (data->isEquipment) {
        if (data->isWeapon) data->stats.dps = createString ("Dps: %i", data->w->dps);

        if (data->isWeapon) data->lifeTxt = createString ("Lifetime: %i / %i", data->w->lifetime, data->w->maxLifetime);
        else data->lifeTxt = createString ("Lifetime: %i / %i", data->a->lifetime, data->a->maxLifetime);

        data->lifeColor = getLifeTimeColor (item);
    }

    return data;

}

// TODO: do we also add the colors here?
// This stores the difference in stats between the compared items
typedef struct {

    char *dpsDiff;
    char *armorDiff;
    char *strengthDiff;
    char *staminaDiff;
    char *agilityDiff;
    char *intellectDiff;

} CompareItems;

CompareItems *comp = NULL;
bool compWeapons;

// just to be sure...
CompareItems *newComp (void) {

    CompareItems *comp = (CompareItems *) malloc (sizeof (CompareItems));

    if (comp != NULL) {
        comp->dpsDiff = NULL;
        comp->armorDiff = NULL;
        comp->strengthDiff = NULL;
        comp->staminaDiff = NULL;
        comp->agilityDiff = NULL;
        comp->intellectDiff = NULL;
    }

    else fprintf (stderr, "Comp items is NULL!\n");

    return comp;

}

void destroyComp (void) {

    if (comp->dpsDiff) free (comp->dpsDiff);
    if (comp->armorDiff) free (comp->armorDiff);
    if (comp->strengthDiff) free (comp->strengthDiff);
    if (comp->staminaDiff) free (comp->staminaDiff);
    if (comp->agilityDiff) free (comp->agilityDiff);
    if (comp->intellectDiff) free (comp->intellectDiff);
    
    free (comp);

}

// search an equipped item to compare to
Item *itemToCompare (Item *item) {

    Item *compareTo = NULL;

    Weapon *w = (Weapon *) getItemComponent (item, WEAPON);
    if (w != NULL) {
        // we have a weapon, so compare it to the one we have equipped
        Item *equipped = player->weapons[w->slot];
        if (equipped != NULL) compareTo = equipped;
        
    }

    else {
        Armour *a = (Armour *) getItemComponent (item, ARMOUR);
        if (a != NULL) {
            // we have an armour, so compare it to the one we have equipped
            Item *equipped = player->equipment[a->slot];
            if (equipped != NULL) compareTo = equipped;
        }
    }

    return compareTo;

}
    
// FIXME: add more stats here!!
CompareItems *compareItems (Item *lootItem, Item *compareTo) {

    CompareItems *comp = newComp ();

    // first check if we are comparing weapons
    Weapon *lootWeapon = (Weapon *) getItemComponent (lootItem, WEAPON);
    Weapon *equippedWeapon = (Weapon *) getItemComponent (compareTo, WEAPON);

    if ((lootWeapon != NULL) && (equippedWeapon != NULL)) {
        i8 diff;

        // dps
        diff = lootWeapon->dps - equippedWeapon->dps;
        if (diff < 0) comp->dpsDiff = createString ("-%i", diff);
        else if (diff > 0) comp->dpsDiff = createString ("+%i", diff);
        else {
            comp->dpsDiff = (char *) calloc (3, sizeof (char));
            strcpy (comp->dpsDiff, "--");
        }
    }    

    return comp;

}

// FIXME: what happens when we have 2 one handed weapons equipped
// FIXME: add value?
// FIXME: how to handle the y idx??
// TODO: change to color depending if we can equip it or not
void renderLootTooltip (Console *console) {

    // render the loot item first
    if (lootItemUI != NULL) {
        putStringAt (console, lootItemUI->name, 2, 2, lootItemUI->rarityColor, NO_COLOR);
        if (lootItemUI->isEquipment)
            putStringAt (console, lootItemUI->typeName, 2, 4, TOOLTIP_TEXT, NO_COLOR);

        if (lootItemUI->isWeapon) {
            if (lootItemUI->w->twoHanded) putStringAt (console, "Two-Handed", 13, 4, WHITE, NO_COLOR);
            else putStringAt (console, "One-Handed", 13, 4, WHITE, NO_COLOR);
        }

        // FIXME: loot stats

        if (lootItemUI->isEquipment) putStringAt (console, lootItemUI->lifeTxt, 2, 8, lootItemUI->lifeColor, NO_COLOR);
    }

    // render the equipped item
    if (equippedItemUI != NULL) {
        putStringAtCenter (console, "Equipped", 12, SAPPHIRE, NO_COLOR);
        putStringAt (console, lootItemUI->name, 2, 14, lootItemUI->rarityColor, NO_COLOR);
        if (lootItemUI->isEquipment)
            putStringAt (console, lootItemUI->typeName, 2, 16, TOOLTIP_TEXT, NO_COLOR);

        if (lootItemUI->isWeapon) {
            if (lootItemUI->w->twoHanded) putStringAt (console, "Two-Handed", 13, 16, WHITE, NO_COLOR);
            else putStringAt (console, "One-Handed", 13, 16, WHITE, NO_COLOR);
        }

        // FIXME: equipped stats
        // FIXME: stats comparison

        if (lootItemUI->isEquipment) putStringAt (console, lootItemUI->lifeTxt, 2, 20, lootItemUI->lifeColor, NO_COLOR);
    }

}

// FIXME: render stats
// FIXME: what happens if the name doesn't fit?
void renderInventoryTooltip (Console *console) {

    if (iData != NULL) {
        putStringAt (console, iData->name, 1, 2, iData->rarityColor, NO_COLOR);
        if (iData->isEquipment) {
            putStringAt (console, iData->typeName, 1, 3, TOOLTIP_TEXT, NO_COLOR);
            if (iData->isWeapon) {
                if (iData->w->twoHanded) putStringAt (console, "Two-Handed", 1, 4, TOOLTIP_TEXT, NO_COLOR);
                else putStringAt (console, "Two-Handed", 1, 4, TOOLTIP_TEXT, NO_COLOR);
            }

            putStringAt (console, iData->lifeTxt, 1, 7, iData->lifeColor, NO_COLOR);
        }

        putStringAt (console, iData->valueTxt, 1, 9, TOOLTIP_TEXT, NO_COLOR);
    }

    if (equippedItemUI != NULL) {
        putStringAtCenter (console, "Equipped", 12, SAPPHIRE, NO_COLOR);
        putStringAt (console, equippedItemUI->name, 1, 14, equippedItemUI->rarityColor, NO_COLOR);
        if (iData->isEquipment) {
            putStringAt (console, equippedItemUI->typeName, 1, 15, TOOLTIP_TEXT, NO_COLOR);
            if (equippedItemUI->isWeapon) {
                if (equippedItemUI->w->twoHanded) putStringAt (console, "Two-Handed", 1, 16, TOOLTIP_TEXT, NO_COLOR);
                else putStringAt (console, "Two-Handed", 1, 16, TOOLTIP_TEXT, NO_COLOR);
            }

            putStringAt (console, equippedItemUI->lifeTxt, 1, 17, equippedItemUI->lifeColor, NO_COLOR);
        }

        putStringAt (console, equippedItemUI->valueTxt, 1, 18, TOOLTIP_TEXT, NO_COLOR);
    }

}

// FIXME: what if the name doesn't fit in the tooltip?
// FIXME: add value?
void renderCharacterTooltip (Console *console) {

    if (equippedItemUI != NULL) {
        putStringAt (console, equippedItemUI->name, 1, 2, equippedItemUI->rarityColor, NO_COLOR);
        if (equippedItemUI->isEquipment)
            putStringAt (console, equippedItemUI->typeName, 1, 4, TOOLTIP_TEXT, NO_COLOR);

        if (equippedItemUI->isWeapon) {
            if (equippedItemUI->w->twoHanded) putStringAt (console, "Two-Handed", 8, 4, WHITE, NO_COLOR);
            else putStringAt (console, "One-Handed", 8, 4, WHITE, NO_COLOR);
        }

        // FIXME: equipped stats
        // FIXME: stats comparison

        if (equippedItemUI->isEquipment) putStringAt (console, equippedItemUI->lifeTxt, 1, 12, equippedItemUI->lifeColor, NO_COLOR);
    }

}

// FIXME: create a more efficent way for the rect
static void renderTooltip (Console *console) {

    switch (tooltip) {
        // loot tooltip
        case 0: {
            UIRect tooltipRect = { 0, 0, TOOLTIP_WIDTH, TOOLTIP_HEIGHT };
            drawRect (console, &tooltipRect, TOOLTIP_COLOR, 1, 0xFF990099);

            renderLootTooltip (console);
        } break;

        // inventory tooltip
        case 1: {
            UIRect tooltipRect = { 0, 0, TTIP_INV_WIDTH, TOOLTIP_HEIGHT };
            drawRect (console, &tooltipRect, TOOLTIP_COLOR, 1, 0xFF990099);

            renderInventoryTooltip (console);
        } break;

        // character tooltip
        case 2: {
            UIRect tooltipRect = { 0, 0, TTIP_CHAR_WIDTH, TTIP_CHAR_HEIGHT };
            drawRect (console, &tooltipRect, TOOLTIP_COLOR, 1, 0xFF990099);

            renderCharacterTooltip (console);
            } break;
        default: break;
    }

}

void lootTooltip (void) {

    // get the selected loot item
    Item *lootItem = getSelectedLootItem ();
    if (lootItem != NULL) {
        if (lootItemUI != NULL) cleanItemData (lootItemUI);
        lootItemUI = getUIItemData (lootItem);

        // check if we have items to compare
        Item *compareTo = itemToCompare (lootItem);

        // get equipped item stats
        if (compareTo != NULL) {
            if (equippedItemUI != NULL) cleanItemData (equippedItemUI);
            equippedItemUI = getUIItemData (compareTo);

            // compare the two item stats
            if (comp != NULL) destroyComp ();
            comp = compareItems (lootItem, compareTo);
        }

        // as of 21/09/2018 -- we are displaying the same window size, no matter if we only have one item
        // render the item stats
        UIRect lootRect = { (16 * TOOLTIP_LEFT), (16 * TOOLTIP_TOP), (16 * TOOLTIP_WIDTH), (16 * TOOLTIP_HEIGHT) };
        tooltipView = newView (lootRect, TOOLTIP_WIDTH, TOOLTIP_HEIGHT, tileset, 0, NO_COLOR, true, renderTooltip);
        insertAfter (activeScene->views, LIST_END (activeScene->views), tooltipView);

        if (lootView != NULL) updateLootPos (true);

    }

}

void invTooltip (void) {

    // check if we have a selected item
    Item *selectedItem = getInvSelectedItem ();
    if (selectedItem != NULL) {
        if (iData != NULL) cleanItemData (iData);
        iData = getUIItemData (selectedItem);

        // check if we have items to compate
        Item *compareTo = itemToCompare (selectedItem);

        if (compareTo != NULL) {
            if (equippedItemUI != NULL) cleanItemData (equippedItemUI);
            equippedItemUI = getUIItemData (compareTo);

            // compare the two items
            if (comp != NULL) destroyComp ();
            comp = compareItems (selectedItem, compareTo);
        }

        // FIXME: we are displaying the same window size as in the loot menu
        UIRect lootRect = { (16 * TTIP_INV_LEFT), (16 * TOOLTIP_TOP), (16 * TTIP_INV_WIDTH), (16 * TOOLTIP_HEIGHT) };
        tooltipView = newView (lootRect, TTIP_INV_WIDTH, TOOLTIP_HEIGHT, tileset, 0, NO_COLOR, true, renderTooltip);
        insertAfter (activeScene->views, LIST_END (activeScene->views), tooltipView);

        if (inventoryView != NULL) updateInventoryPos (true);
    }

}

void characterTooltip (void) {

    // first check if we have an item selected
    Item *selectedItem = getCharSelectedItem ();
    if (selectedItem != NULL) {
        if (equippedItemUI != NULL) cleanItemData (equippedItemUI);
        equippedItemUI = getUIItemData (selectedItem);

        if (characterXIdx == 0) {
            UIRect lootRect = { (16 * TTIP_CHAR_LEFT), (16 * TOOLTIP_TOP), (16 * TTIP_CHAR_WIDTH), (16 * TTIP_CHAR_HEIGHT) };
            tooltipView = newView (lootRect, TTIP_CHAR_WIDTH, TTIP_CHAR_HEIGHT, tileset, 0, NO_COLOR, true, renderTooltip);
        }

        else {
            UIRect lootRect = { (16 * TTIP_CHAR_RIGHT), (16 * TOOLTIP_TOP), (16 * TTIP_CHAR_WIDTH), (16 * TTIP_CHAR_HEIGHT) };
            tooltipView = newView (lootRect, TTIP_CHAR_WIDTH, TTIP_CHAR_HEIGHT, tileset, 0, NO_COLOR, true, renderTooltip);
        }
        
        insertAfter (activeScene->views, LIST_END (activeScene->views), tooltipView);
    }  

}

void cleanTooltipData (void) {

    if (lootItemUI != NULL) cleanItemData (lootItemUI);
    lootItemUI = NULL;
    if (equippedItemUI != NULL) cleanItemData (equippedItemUI);
    equippedItemUI = NULL;
    if (iData != NULL) cleanItemData (iData);
    iData = NULL;

    if (comp != NULL) destroyComp ();
    comp = NULL;

}

void toggleTooltip (u8 view) {

    if (tooltipView == NULL) {
        switch (view) {
            case 0: lootTooltip (); tooltip = 0; break;
            case 1: invTooltip (); tooltip = 1; break;
            case 2: characterTooltip (); tooltip = 2; break;
            default: break;
        }
    }

    // FIXME: handle character, inventory and tooltip at the same time
    else if (tooltipView != NULL) {
        cleanTooltipData ();

        ListElement *e = getListElement (activeScene->views, tooltipView);
        if (e != NULL) destroyView ((UIView *) removeElement (activeScene->views, e));
        else destroyView (tooltipView);
        tooltipView = NULL;

        if (lootView != NULL) updateLootPos (false);
        if (characterView != NULL) updateCharacterPos (false);
        if (inventoryView != NULL) updateInventoryPos (false);

        activeView = (UIView *) LIST_END (activeScene->views)->data;
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
    drawRect (console, &rect, PAUSE_COLOR, 0, WHITE);

    putStringAt (console, "Pause Menu", 15, 2, INVENTORY_TEXT, NO_COLOR);

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

/*** INIT GAME SCREEN ***/

UIScreen *inGameScreen = NULL;

UIView *mapView = NULL;

List *initGameViews (void) {

    List *views = initList (free);

    UIRect mapRect = { 0, 0, (16 * MAP_WIDTH), (16 * MAP_HEIGHT) };
    bool colorize = true;
    u32 bgColor = 0x000000FF;

    mapView = newView (mapRect, MAP_WIDTH, MAP_HEIGHT, tileset, 0, bgColor, colorize, renderMap);
    insertAfter (views, NULL, mapView);

    UIRect statsRect = { 0, (16 * MAP_HEIGHT), (16 * STATS_WIDTH), (16 * STATS_HEIGHT) };
    UIView *statsView = newView (statsRect, STATS_WIDTH, STATS_HEIGHT, tileset, 0, 0x000000FF, true, rednderStats);
    insertAfter (views, NULL, statsView);

    UIRect logRect = { (16 * 20), (16 * MAP_HEIGHT), (16 * LOG_WIDTH), (16 * LOG_HEIGHT) };
    UIView *logView = newView (logRect, LOG_WIDTH, LOG_HEIGHT, tileset, 0, 0x000000FF, true, renderLog);
    insertAfter (views, NULL, logView);

    return views;

}

void destroyGameUI (void);

UIScreen *gameScene (void) {

    List *igViews = initGameViews ();

    if (inGameScreen == NULL) inGameScreen = (UIScreen *) malloc (sizeof (UIScreen));
    
    inGameScreen->views = igViews;
    inGameScreen->activeView = mapView;
    inGameScreen->handleEvent = hanldeGameEvent;

    statsPlayerName = createString ("%s the %s", player->name, getPlayerClassName (player->cClass));

    wallsFadedColor = COLOR_FROM_RGBA (RED (wallsFgColor), GREEN (wallsFgColor), BLUE (wallsFgColor), 0x77);

    inventoryRects = initInventoryRects ();
    characterRects = initCharacterRects ();

    activeLootRects = initList (free);
    lootRectsPool = initPool ();

    activeView = mapView;

    destroyCurrentScreen = destroyGameUI;

    return inGameScreen;

}

/*** CLEAN UP ***/

void resetGameUI (void) {

    messageLog = NULL;

    iData = NULL;
    lootItemUI = NULL;
    equippedItemUI = NULL;
    comp = NULL;

    inventoryRects = NULL;
    characterRects = NULL;

}

void destroyGameUI (void) {

    if (inGameScreen != NULL) {
        fprintf (stdout, "Cleaning in game UI...\n");

        // message log
        cleanMessageLog ();

        // tooltip
        cleanTooltipData ();

        if (inventoryRects != NULL) destroyInvRects ();
        if (characterRects != NULL) destroyCharRects ();

        destroyLootRects ();

        fprintf (stdout, "Cleaning in game views...\n");

        while (LIST_SIZE (inGameScreen->views) > 0)
            destroyView ((UIView *) removeElement (inGameScreen->views, LIST_END (inGameScreen->views)));
        
        free (inGameScreen->views);
        free (inGameScreen);
        inGameScreen = NULL;

        fprintf (stdout, "Done cleanning up game UI!\n");
    }

}

/*** POST GAME SCREEN ***/

UIScreen *postGameScene = NULL;

/*** DEATH SCREEN ***/

BitmapImage *deathImg = NULL;
char *deathImgPath = "./resources/death-720.png"; 

UIView *deathScreen = NULL;

static void renderDeathScreen (Console *console) {

    drawImageAt (console, deathImg, 0, 0);

}

void deleteDeathScreen (void) {

    if (deathScreen != NULL) {
        if (deathImg != NULL) {
            destroyImage (deathImg);
            deathImg = NULL;
        } 

        ListElement *death = getListElement (postGameScene->views, deathScreen);
        destroyView ((UIView *) removeElement (postGameScene->views, death));
        deathScreen = NULL;

        postGameScene->activeView = (UIView *) (LIST_END (postGameScene->views))->data;
    }

}

/*** SCORE SCREEN ***/

BitmapImage *scoreImg = NULL;
char *scoreImgPath = "./resources/score-720.png"; 

UIView *scoreScreen = NULL;

static void renderScoreScreen (Console *console) {

    drawImageAt (console, scoreImg, 0, 0);

}

// FIXME: handle the active view when toggling
void toggleScoreScreen (void) {

    if (scoreScreen == NULL) {
        UIRect bgRect = { 0, 0, (16 * FULL_SCREEN_WIDTH), (16 * FULL_SCREEN_HEIGHT) };
        scoreScreen = newView (bgRect, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, tileset, 0, 0x000000FF, true, renderScoreScreen);
        insertAfter (activeScene->views, LIST_END (activeScene->views), scoreScreen);

        if (scoreImg == NULL) scoreImg = loadImageFromFile (scoreImgPath);

        postGameScene->activeView = scoreScreen;
    }

    else {
        if (scoreScreen != NULL) {
            if (scoreImg != NULL) {
                destroyImage (scoreImg);
                scoreImg = NULL;
            } 

            ListElement *death = getListElement (activeScene->views, scoreScreen);
            destroyView ((UIView *) removeElement (activeScene->views, death));
            scoreScreen = NULL;

            // activeView = (UIView *) (LIST_END (activeScene->views))->data;
        }
    }

}


/*** LEADERBOARDS ***/

#define ODD_ROW_COLOR       0x485460FF
#define EVEN_ROW_COLOR      0X3C6382FF

UIView *leaderBoardView = NULL;
bool isLocalLB;

typedef struct {

    UIRect *bgRect;
    LBEntry *entry;

} LBRect;

List *lbRects = NULL;

LBRect *createLBRect (u8 y, LBEntry *entry) {

    LBRect *new = (LBRect *) malloc (sizeof (LBRect));

    new->bgRect = (UIRect *) malloc (sizeof (UIRect));
    new->bgRect->x = 4;
    new->bgRect->y = y;
    new->bgRect->w = 72;
    new->bgRect->h = 3;

    new->entry = entry;

    return new;

}

List *createLBUI (List *lbData) {

    List *rects = initList (free);

    LBEntry *entry = NULL;
    u8 yIdx = 10;

    for (ListElement *e = LIST_START (lbData); e != NULL; e = e->next) {
        insertAfter (rects, LIST_END (rects), createLBRect (yIdx, (LBEntry *) e->data));
        yIdx += 3;
    }

    return rects;

}

void renderLBRects (Console *console) {

    if (lbRects != NULL) {
        LBRect *rect = NULL;
        u8 count = 1;
        u8 yIdx = 11;
        for (ListElement *e = LIST_START (lbRects); e != NULL; e = e->next) {
            rect = (LBRect *) e->data;

            if (count % 2 == 0) drawRect (console, rect->bgRect, EVEN_ROW_COLOR, 0, NO_COLOR);
            else drawRect (console, rect->bgRect, ODD_ROW_COLOR, 0, NO_COLOR);

            putStringAt (console, rect->entry->name, 5, yIdx, rect->entry->nameColor, NO_COLOR);
            putStringAt (console, rect->entry->level, 37, yIdx, WHITE, NO_COLOR);
            putStringAt (console, rect->entry->kills, 50, yIdx, WHITE, NO_COLOR);
            putReverseString (console, rect->entry->score, 68, yIdx, WHITE, NO_COLOR);

            count++;
            yIdx += 3;
        }
    }

    // FIXME: else NO DATA!!

}

void renderLocalLB (Console *console) {

    // FIXME: where do we want this?
    if (localLB == NULL) {
        fprintf (stdout, "Getting local leaderboard data...\n");
        localLB = getLocalLBData ();
        if (localLB != NULL) lbRects = createLBUI (localLB);
        // FIXME: DISPLAY AN ERROR else 
    } 
    
    else {
        putStringAtCenter (console, "Local LeaderBoards", 2, WHITE, NO_COLOR);

        // display table titles
        putStringAt (console, "Name", 17, 7, WHITE, NO_COLOR);
        putStringAt (console, "Level", 35, 7, WHITE, NO_COLOR);
        putStringAt (console, "Kills", 48, 7, WHITE, NO_COLOR);
        putStringAt (console, "Score", 62, 7, WHITE, NO_COLOR);

        // display each player and its data on their rects
        renderLBRects (console);
    }

}

void renderGlobalLb (Console *console) {

    // FIXME: where do we want this?
    if (globalLB == NULL) globalLB = getGlobalLBData ();
    else {
        // TODO: render the list of players
    }

}

static void renderLeaderboard (Console *console) {

    // FIXME: color
    UIRect rect = { 0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT };
    drawRect (console, &rect, PAUSE_COLOR, 0, WHITE);

    if (isLocalLB) renderLocalLB (console);
    else renderGlobalLb (console);

}

void toggleLeaderBoards (void) {

    if (leaderBoardView == NULL) {
        UIRect bgRect = { 0, 0, (16 * FULL_SCREEN_WIDTH), (16 * FULL_SCREEN_HEIGHT) };
        leaderBoardView = newView (bgRect, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, tileset, 0, BLACK, true, renderLeaderboard);
        insertAfter (activeScene->views, LIST_END (activeScene->views), leaderBoardView);
        postGameScene->activeView = leaderBoardView;

        // render local leaderboard by default
        isLocalLB = true;
    }

    else {
        if (leaderBoardView != NULL) {
            ListElement *leader = getListElement (activeScene->views, leaderBoardView);
            if (leader != NULL) removeElement (activeScene->views, leader);
            destroyView (leaderBoardView);
            leaderBoardView = NULL;

            // FIXME: do we need this?
            // activeView = (UIView *) (LIST_END (activeScene->views))->data;
        }
    }

}

void destroyLBUI (void) {

    if (lbRects != NULL) {
        LBRect *rect = NULL;
        while (LIST_SIZE (lbRects) > 0) {
            rect = (LBRect *) removeElement (lbRects, LIST_END (lbRects));
            if (rect != NULL) {
                free (rect->bgRect);
                rect->entry = NULL;
                free (rect);
            }
        }

        destroyList (lbRects);
    }

}

// FIXME:
// TODO: delete all other UI elements!!
void destroyPostGameScreen (void) {

    if (postGameScene != NULL) {
        if (deathImg != NULL) {
            destroyImage (deathImg);
            deathImg = NULL;
        } 
        
        if (scoreImg != NULL) {
            destroyImage (scoreImg);
            scoreImg = NULL;
        } 

        destroyLBUI ();

        while (LIST_SIZE (postGameScene->views) > 0) 
            destroyView ((UIView *) removeElement (postGameScene->views, LIST_END (postGameScene->views)));
        
        free (postGameScene->views);
        free (postGameScene);
        postGameScene = NULL;

        fprintf (stdout, "Post game screen destroyed!\n");
    }

}

// default view is the game death screen
UIScreen *postGameScreen (void) {

    List *views = initList (free);

    UIRect bgRect = { 0, 0, (16 * FULL_SCREEN_WIDTH), (16 * FULL_SCREEN_HEIGHT) };
    deathScreen = newView (bgRect, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, tileset, 0, BLACK, true, renderDeathScreen);
    insertAfter (views, NULL, deathScreen);

    if (deathImg == NULL) deathImg = loadImageFromFile (deathImgPath);

    postGameScene = (UIScreen *) malloc (sizeof (UIScreen));
    
    postGameScene->views = views;
    postGameScene->activeView = deathScreen;
    postGameScene->handleEvent = handlePostGameEvent;

    destroyCurrentScreen = destroyPostGameScreen;

    fprintf (stdout, "Post game init!\n");

    return postGameScene;

}