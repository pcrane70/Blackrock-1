#include <stdbool.h>

#include "blackrock.h"

#include "game.h"
#include "player.h"
#include "item.h"

#include "utils/dlist.h"

#include "ui/ui.h"
#include "ui/gameUI.h"

extern bool inGame;

/*** MAIN MENU ***/

extern bool running;
extern bool wasInGame;

// TODO: do we want this here?
void startGame (void) {

    /* cleanUpMenuScene ();
    activeScene = NULL;

    initGame ();

    setActiveScene (gameScene ());

    inGame = true;
    wasInGame = true;  */

}

#pragma region MAIN MENU 

#include "ui/menu.h"

void hanldeMenuEvent (UIScreen *activeScreen, SDL_Event event) {

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_p: createMainMenu (); break;
            case SDLK_m: if (activeMenuView != MULTI_MENU_VIEW) toggleMultiplayerMenu (); break;
            case SDLK_b: if (activeMenuView == MULTI_MENU_VIEW) toggleMultiplayerMenu (); break;
            case SDLK_c: if (activeMenuView == MULTI_MENU_VIEW) printf ("\ncreate lobby\n"); break;
            // case SDLK_j: if (activeMenuView == MULTI_MENU_VIEW) joinGame (); break;

            // old events
            // FIXME: case SDLK_s: if (characterMenu != NULL) startGame (); break;
            // case SDLK_c: break;     // TODO: toggle credits window
            // case SDLK_e: running = false; break;
            default: break;
        }

    }

}

#pragma endregion

/*** GAME ***/

#pragma region GAME

void resolveCombat (Position newPos) {

    // check what is blocking the movement
    DoubleList *blockers = getObjectsAtPos (newPos.x, newPos.y);

    if (blockers == NULL || LIST_SIZE (blockers) <= 0) {
        free (blockers);
        return;
    }

    for (ListElement *e = LIST_START (blockers); e != NULL; e = e->next) {
        Combat *c = (Combat *) getComponent ((GameObject *) e->data, COMBAT);
        if (c != NULL) {
            fight (player->combat, c, true);
            break;
        }
    }

    dlist_clean (blockers);

}

Position *playerPos = NULL;

void move (u8 newX, u8 newY) {

    Position newPos = { .x = newX, .y = newY };
    if (canMove (newPos, true)) {
        recalculateFov = true;
        playerPos->x = newX;
        playerPos->y = newY;
    } 
    else resolveCombat (newPos);
    playerTookTurn = true; 

}

/*** GAME UI ***/

void moveInInventory (u8 newX, u8 newY) {

    if (newX >= 0 && newX <= 6) inventoryXIdx = newX;
    if (newY >= 0 && newY < 3) inventoryYIdx = newY;

}

void moveInLoot (u8 newY) { if (newY >= 0 && (newY < LIST_SIZE (activeLootRects))) lootYIdx = newY; }

void moveInCharacter (u8 newX, u8 newY, bool moveRight) {

    if (newX >= 0 && newX < 2) characterXIdx = newX;
    if (newY >= 0 && newY < 6) characterYIdx = newY;

}

bool isInUI (void) {

    if (lootView != NULL || inventoryView != NULL || characterView != NULL) return true;
    else return false;

}

// FIXME: tooltip logic
void closeUIMenu (void) {

    if (tooltipView != NULL) {
        if (lootView != NULL) toggleTooltip (0);
        else toggleTooltip (1);
    } 

    if (lootView != NULL) toggleLootWindow ();

    if (inventoryView != NULL) toggleInventory ();

    if (characterView != NULL) toggleCharacter ();

}

/*** NAVIGATION BETWEEN MENUS ***/

// when the game inits, the active view is the map
UIView *activeView = NULL;

// As of 02/09/2018 -- 01:25 -- we can only switch views between the inventory and character
void swicthView (void) {

    // switch the views in the list
    if ((characterView != NULL) && (inventoryView != NULL) || lootView != NULL) {
        void *prev = dlist_remove_element (activeScene->views, (LIST_END (activeScene->views))->prev);
        void *last = dlist_remove_element (activeScene->views, (LIST_END (activeScene->views)));

        dlist_insert_after (activeScene->views, LIST_END (activeScene->views), last);
        dlist_insert_after (activeScene->views, LIST_END (activeScene->views), prev);

        activeView = (UIView *) (LIST_END (activeScene->views))->data;
    }

}

/*** GAME EVENTS ***/

extern void gameOver (void);

// FIXME:
void triggerEvent (void) {

    if (activeView == inventoryView) {
        Item *item = getInvSelectedItem ();
        if (item) {
            // if (item->callback != NULL) item->callback (item);

        } 
    } 

    else if (activeView == characterView) {
        Item *item = getCharSelectedItem ();
        if (item) {
            // if (item->callback != NULL) item->callback (item);
        }
    }

    // loop through all of our surrounding items in search for 
    // an event listener to trigger
    else if (activeView == mapView) {
        DoubleList *gos = getObjectsAtPos (playerPos->x, playerPos->y);
        if (gos != NULL) {
            Event *ev = NULL;
            for (ListElement *e = LIST_START (gos); e != NULL; e = e ->next) {
                ev = (Event *) getComponent ((GameObject *) e->data, EVENT);
                // trigger just the first event we find
                if (ev != NULL) {
                    ev->callback (e->data);
                    break;
                }
            }

            if (LIST_SIZE (gos) > 0) dlist_clean (gos);
            else free (gos);
        }
    }
            
}

void hanldeGameEvent (UIScreen *activeScreen, SDL_Event event) {

    playerPos = player->pos;

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_w:
                if (activeView == mapView) move (playerPos->x, playerPos->y - 1);
                else if (activeView == lootView) moveInLoot (lootYIdx - 1);
                else if (activeView == inventoryView) moveInInventory (inventoryXIdx, inventoryYIdx - 1);
                else if (activeView == characterView) moveInCharacter (characterXIdx, characterYIdx - 1, false);
                break;
            case SDLK_s: 
                if (activeView == mapView) move (playerPos->x, playerPos->y + 1);
                else if (activeView == lootView) moveInLoot (lootYIdx + 1);
                else if (activeView == inventoryView) moveInInventory (inventoryXIdx, inventoryYIdx + 1);
                else if (activeView == characterView) moveInCharacter (characterXIdx, characterYIdx + 1, false);
                break;
            case SDLK_a: 
                if (activeView == mapView) move (playerPos->x - 1, playerPos->y);
                else if (activeView == inventoryView) moveInInventory (inventoryXIdx - 1, inventoryYIdx);
                else if (activeView == characterView) moveInCharacter (characterXIdx - 1, characterYIdx, false);
                break;
            case SDLK_d:
                if (activeView == mapView) move (playerPos->x + 1, playerPos->y);
                else if (activeView == inventoryView) moveInInventory (inventoryXIdx + 1, inventoryYIdx);
                else if (activeView == characterView) moveInCharacter (characterXIdx + 1, characterYIdx, true);
                break;

            case SDLK_e: triggerEvent (); break;
            case SDLK_g:
                if (activeView == lootView) getLootItem (lootYIdx);
                else if (activeView == mapView) getItem (); 
                break;
            case SDLK_c: 
                if (activeView == lootView) collectGold (); 
                else toggleCharacter ();
                break;

            // drop item
            case SDLK_SPACE:
                if (activeView == inventoryView) {
                    Item *item = getInvSelectedItem ();
                    if (item != NULL) dropItem (item);
                } 
                break;

            case SDLK_i: toggleInventory (); break;

            case SDLK_p: togglePauseMenu (); break;

            // switch between the open windows
            case SDLK_TAB: swicthView (); break;

            // toggle tooltip
            case SDLK_LSHIFT: 
                if (activeView == lootView) toggleTooltip (0);
                else if (activeView == inventoryView) toggleTooltip (1);
                else if (activeView == characterView) toggleTooltip (2);
                break;

            case SDLK_ESCAPE: closeUIMenu (); break;

            // FIXME:
            // quit to main menu
            // case SDLK_q: if (activeView == scoreScreen) returnToMainMenu (); break;

            // FIXME: this is only for testing!!
            case SDLK_k: gameOver (); break;

            default: break;
        }
    }

}

#pragma endregion

/*** POST GAME ***/

#pragma region POST GAME

void handlePostGameEvent (UIScreen *activeScreen, SDL_Event event) {

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_c: if (postGameScene->activeView == deathScreen) showScore (); break;
            case SDLK_r: if (postGameScene->activeView == scoreScreen) retry (); break;
            case SDLK_l: 
                if (postGameScene->activeView == scoreScreen) toggleLeaderBoards (); 
                break;

            // FIXME:
            // case SDLK_q: returnToMainMenu (); break;
        }
    }

}

#pragma endregion