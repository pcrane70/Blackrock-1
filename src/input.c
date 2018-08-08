#include "blackrock.h"

#include "game.h"

#include "console.h"

// Basic Input
// Movement with wsad   03/08/2018

// TODO: recalculate the fov every time the player moves

void handlePlayerInput (SDL_Event event, GameObject *player) {

    // FIXME: 04/08/2018
    Position *playerPos = (Position *) getComponent (player, POSITION);

    // FIXME: how can we handle collisions??
    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;
        switch (key) {
            case SDLK_w: 
                if (playerPos->y > 0) playerPos->y -= 1; break;
            case SDLK_s: 
                if (playerPos->y < NUM_ROWS - 1) playerPos->y += 1; break;
            case SDLK_a: 
                if (playerPos->x > 0) playerPos->x -= 1; break;
            case SDLK_d:
                if (playerPos->x < NUM_COLS - 1) playerPos->x += 1; break;
            default: break;
        }
    }

}

