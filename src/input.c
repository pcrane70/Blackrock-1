#include "blackrock.h"

#include "game.h"

#include "console.h"

// Basic Input
// Movement with wsad   03/08/2018

// TODO: recalculate the fov every time the player moves

void handlePlayerInput (SDL_Event event, GameObject *player) {

    // FIXME: 04/08/2018
    // Position *playerPos = (Position *) getComponent (player, POSITION);

    // FIXME: how can we handle collisions??
    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;
        switch (key) {
            case SDLK_w: 
                if (player->y > 0) player->y -= 1; break;
            case SDLK_s: 
                if (player->y < NUM_ROWS - 1) player->y += 1; break;
            case SDLK_a: 
                if (player->x > 0) player->x -= 1; break;
            case SDLK_d:
                if (player->x < NUM_COLS - 1) player->x += 1; break;
            default: break;
        }
    }

}

