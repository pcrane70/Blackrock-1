#include "blackrock.h"

#include "game.h"

#include "list.h"

#include "ui/ui.h"
#include "ui/gameUI.h"

// Basic Input
// Movement with wsad   03/08/2018

extern bool inGame;

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
Position newPos;

// TODO: maybe later we will want to move using the numpad insted to allow diagonal movement
void hanldeGameEvent (UIScreen *activeScreen, SDL_Event event) {

    playerPos = (Position *) getComponent (player, POSITION);

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            // Movement
            case SDLK_w: 
                newPos.x = playerPos->x;
                newPos.y = playerPos->y - 1;
                if (canMove (newPos)) {
                    recalculateFov = true;
                    playerPos->y = newPos.y;
                } 
                else resolveCombat (newPos);
                playerTookTurn = true; 
                break;
            case SDLK_s: 
                newPos.x = playerPos->x;
                newPos.y = playerPos->y + 1;
                if (canMove (newPos)) {
                    recalculateFov = true;
                    playerPos->y = newPos.y;
                } 
                else resolveCombat (newPos);
                playerTookTurn = true;
                break;
            case SDLK_a: 
                newPos.x = playerPos->x - 1;
                newPos.y = playerPos->y;
                if (canMove (newPos)) {
                    recalculateFov = true;
                    playerPos->x = newPos.x;
                } 
                else resolveCombat (newPos);
                playerTookTurn = true; 
                break;
            case SDLK_d:
                newPos.x = playerPos->x + 1;
                newPos.y = playerPos->y;
                if (canMove (newPos)) {
                    recalculateFov = true;
                    playerPos->x = newPos.x;
                } 
                else resolveCombat (newPos);
                playerTookTurn = true;
                break;

            // 21/08/2018 -- 6:51 -- this is used as the interactable button
            case SDLK_e: {
                // loop through all of our surrounding items in search for 
                // an event listener to trigger
                List *gos = getObjectsAtPos (playerPos->x, playerPos->y);
                for (ListElement *e = LIST_START (gos); e != NULL; e = e ->next) {
                    Event *ev = (Event *) getComponent ((GameObject *) e->data, EVENT);
                    // trigger just the first event we find
                    if (ev != NULL) {
                        ev->callback ();
                        break;
                    }
                }
                free (gos);
            } break;

            case SDLK_g: getItem (); break;

            // TODO: drop an item
            // case SDLK_d: break;

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

            default: break;
        }
    }

}