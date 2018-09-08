#include <stdbool.h>

#include "blackrock.h"

#include "game.h"
#include "player.h"
#include "item.h"

#include "utils/list.h"

#include "ui/ui.h"
#include "ui/gameUI.h"

extern bool inGame;

/*** MAIN MENU ***/

void hanldeMenuEvent (UIScreen *activeScreen, SDL_Event event) {

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_p: 
                initGame ();
                setActiveScene (gameScene ());
                inGame = true;
                break;

            default: break;
        }

    }

}

/*** GAME ***/

void resolveCombat (Position newPos) {

    // check what is blocking the movement
    List *blockers = getObjectsAtPos (newPos.x, newPos.y);

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

    cleanUpList (blockers);

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

void moveInLoot (u8 newY) { if (newY >= 0 && (newY < LIST_SIZE (lootRects))) lootYIdx = newY; }

void moveInCharacter (u8 newX, u8 newY, bool moveRight) {

    if (newX >= 0 && newX < 2) characterXIdx = newX;
    if (newY >= 0 && newY < 6) characterYIdx = newY;

}

bool isInUI (void) {

    if (lootView != NULL || inventoryView != NULL || characterView != NULL) return true;
    else return false;

}

void closeUIMenu (void) {

    // if (inventoryView != NULL) toggleInventory (false);

    // if (lootView != NULL) toggleLootWindow ();

    // if (characterView != NULL) hideCharacter ();

}

/*** NAVIGATION BETWEEN MENUS ***/

// when the game inits, the active view is the map
UIView *activeView = NULL;

// As of 02/09/2018 -- 01:25 -- we can only switch views between the inventory and character
void swicthView (void) {

    // switch the views in the list
    if ((characterView != NULL) && (inventoryView != NULL)) {
        void *prev = removeElement (activeScene->views, (LIST_END (activeScene->views))->prev);
        void *last = removeElement (activeScene->views, (LIST_END (activeScene->views)));

        insertAfter (activeScene->views, LIST_END (activeScene->views), last);
        insertAfter (activeScene->views, LIST_END (activeScene->views), prev);

        activeView = (UIView *) (LIST_END (activeScene->views))->data;
    }

}

/*** GAME EVENTS ***/

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

            case SDLK_e: {
                if (activeView == inventoryView) {
                    Item *item = getInvSelectedItem ();
                    if (item != NULL) item->callback (item);
                } 

                // loop through all of our surrounding items in search for 
                // an event listener to trigger
                if (activeView == mapView) {
                    List *gos = getObjectsAtPos (playerPos->x, playerPos->y);
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

                        if (LIST_SIZE (gos) > 0) cleanUpList (gos);
                        else free (gos);
                    }
                }
            } break;
            case SDLK_g:
                if (activeView == lootView) getLootItem (lootYIdx);
                else if (activeView == mapView) getItem (); 
                break;

            case SDLK_c: toggleCharacter (); break;

            // FIXME:
            // drop item
            // case SDLK_SPACE:
            //     if (isInUI () && inventoryView != NULL) {
            //         Item *item = getInvSelectedItem ();
            //         if (item != NULL) dropItem (item);
            //     } 
            //     break;

            case SDLK_i: toggleInventory (); break;

            case SDLK_p: togglePauseMenu (); break;

            // switch between the open windows
            case SDLK_TAB: swicthView (); break;

            case SDLK_ESCAPE: closeUIMenu (); break;

            default: break;
        }
    }

}