#include <stdbool.h>

#include "blackrock.h"

#include "game.h"
#include "player.h"
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

    if (inventoryView != NULL) toggleInventory ();

    if (lootView != NULL) toggleLootWindow ();

    if (characterView != NULL) toggleCharacter ();

}

void hanldeGameEvent (UIScreen *activeScreen, SDL_Event event) {

    playerPos = player->pos;

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_w:
                if (!isInUI ()) move (playerPos->x, playerPos->y - 1);
                else if (inventoryView != NULL) moveInInventory (inventoryXIdx, inventoryYIdx - 1);
                else if (lootView != NULL) moveInLoot (lootYIdx - 1);
                else if (characterView != NULL) moveInCharacter (characterXIdx, characterYIdx - 1, false);
                break;
            case SDLK_s: 
                if (!isInUI ()) move (playerPos->x, playerPos->y + 1);
                else if (inventoryView != NULL) moveInInventory (inventoryXIdx, inventoryYIdx + 1);
                else if (lootView != NULL) moveInLoot (lootYIdx + 1);
                else if (characterView != NULL) moveInCharacter (characterXIdx, characterYIdx + 1, false);
                break;
            case SDLK_a: 
                if (!isInUI ()) move (playerPos->x - 1, playerPos->y);
                else if (inventoryView != NULL) moveInInventory (inventoryXIdx - 1, inventoryYIdx);
                else if (characterView != NULL) moveInCharacter (characterXIdx - 1, characterYIdx, false);
                break;
            case SDLK_d:
                if (!isInUI ()) move (playerPos->x + 1, playerPos->y);
                else if (inventoryView != NULL) moveInInventory (inventoryXIdx + 1, inventoryYIdx);
                else if (characterView != NULL) moveInCharacter (characterXIdx + 1, characterYIdx, true);
                break;

            // 21/08/2018 -- 6:51 -- this is used as the interactable button
            case SDLK_e: {
                if (isInUI () && inventoryView != NULL) {
                    Item *item = getInvSelectedItem ();
                    if (item != NULL) item->callback (item);
                } 
                // loop through all of our surrounding items in search for 
                // an event listener to trigger
                else if (!isInUI ()) {
                    List *gos = getObjectsAtPos (playerPos->x, playerPos->y);
                    if (gos != NULL) {
                        for (ListElement *e = LIST_START (gos); e != NULL; e = e ->next) {
                            Event *ev = (Event *) getComponent ((GameObject *) e->data, EVENT);
                            // trigger just the first event we find
                            if (ev != NULL) {
                                ev->callback (e->data);
                                break;
                            }
                        }
                        destroyList (gos);
                    }
                }
            } break;

            case SDLK_g:
                if (inventoryView == NULL) {
                    if (lootView != NULL) getLootItem (lootYIdx);
                    else getItem (); 
                } 
                break;

            // 30/08/2018 -- 07:02 -- This might be temporary
            case SDLK_c: 
                if (isInUI () && (lootView != NULL)) collectGold ();
                else if (!isInUI () && (characterView == NULL)) toggleCharacter ();
                break;

            // drop item
            // case SDLK_SPACE:
            //     if (isInUI () && inventoryView != NULL) {
            //         Item *item = getInvSelectedItem ();
            //         if (item != NULL) dropItem (item);
            //     } 
            //     break;

            // FIXME: how to handle an open loot menu and an open inventory?
            case SDLK_i: toggleInventory (); break;

            case SDLK_p: togglePauseMenu (); break;

            case SDLK_ESCAPE: closeUIMenu (); break;

            default: break;
        }
    }

}