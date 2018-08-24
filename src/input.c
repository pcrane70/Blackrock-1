#include <stdbool.h>

#include "blackrock.h"

#include "game.h"
#include "item.h"

#include "utils/list.h"

#include "ui/ui.h"
#include "ui/gameUI.h"

// Basic Input
// Movement with wsad   03/08/2018

extern bool inGame;

/*** MAIN MENU ***/

void hanldeMenuEvent (UIScreen *activeScreen, SDL_Event event) {

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_n: 
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
    if (blockers == NULL || LIST_SIZE (blockers) <= 0) return;
    for (ListElement *e = LIST_START (blockers); e != NULL; e= e->next) {
        Combat *c = (Combat *) getComponent ((GameObject *) e->data, COMBAT);
        if (c != NULL) {
            fight (player, (GameObject *) e->data);
            break;
        }
    }

    free (blockers);

}

Position *playerPos = NULL;

void move (u8 newX, u8 newY) {

    Position newPos = { .x = newX, .y = newY };
    if (canMove (newPos)) {
        recalculateFov = true;
        playerPos->x = newX;
        playerPos->y = newY;
    } 
    else resolveCombat (newPos);
    playerTookTurn = true; 

}

extern UIView *lootView;
extern UIView *inventoryView;

bool isInUI (void) {

    if (lootView != NULL || inventoryView != NULL) return true;
    else return false;

}

void closeUIMenu (void) {

    if (inventoryView != NULL) {
        toggleInventory ();
        return;
    } 

    if (lootView != NULL) toggleLootWindow ();

}

void hanldeGameEvent (UIScreen *activeScreen, SDL_Event event) {

    playerPos = (Position *) getComponent (player, POSITION);

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_w:
                if (!isInUI ()) move (playerPos->x, playerPos->y - 1);
                break;
            case SDLK_s: 
                if (!isInUI ()) move (playerPos->x, playerPos->y + 1);
                break;
            case SDLK_a: 
                if (!isInUI ()) move (playerPos->x - 1, playerPos->y);
                break;
            case SDLK_d:
                if (!isInUI ()) move (playerPos->x + 1, playerPos->y);
                break;

            // 21/08/2018 -- 6:51 -- this is used as the interactable button
            case SDLK_e: {
                if (isInUI ()) return;
                // loop through all of our surrounding items in search for 
                // an event listener to trigger
                List *gos = getObjectsAtPos (playerPos->x, playerPos->y);
                if (gos != NULL) {
                    for (ListElement *e = LIST_START (gos); e != NULL; e = e ->next) {
                        Event *ev = (Event *) getComponent ((GameObject *) e->data, EVENT);
                        // trigger just the first event we find
                        if (ev != NULL) {
                            ev->callback ();
                            break;
                        }
                    }
                    free (gos);
                }
            } break;

            case SDLK_g:
                if (inventoryView == NULL) {
                    if (lootView != NULL) getLootItem ();
                    else getItem (); 
                } 
                break;

            // TODO: drop an item
            // case SDLK_d: break;

            // FIXME: how to handle an open loot menu and an open inventory?
            case SDLK_i: toggleInventory (); break;

            // TODO: equip an item
            // case SDLK_q: break;

            // TODO: toggle character equipment
            // case SDLK_c: break;

            // TODO: player rests?
            // case SDLK_z: break;

            // TODO: toggle pause menu
            // case SDLK_p: break;

            //TODO: what other things do we want?

            case SDLK_ESCAPE: closeUIMenu (); break;

            default: break;
        }
    }

}