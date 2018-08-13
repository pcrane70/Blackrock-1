#include "blackrock.h"

#include "game.h"

// Basic Input
// Movement with wsad   03/08/2018

// TODO: recalculate the fov every time the player moves

Position *playerPos = NULL;
Position newPos;

void handlePlayerInput (SDL_Event event) {

    playerPos = (Position *) getComponent (player, POSITION);

    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;

        switch (key) {
            case SDLK_w: 
                newPos.x = playerPos->x;
                newPos.y = playerPos->y - 1;
                if (canMove (newPos)) playerPos->y = newPos.y; break;
            case SDLK_s: 
                newPos.x = playerPos->x;
                newPos.y = playerPos->y + 1;
                if (canMove (newPos)) playerPos->y = newPos.y; break;
            case SDLK_a: 
                newPos.x = playerPos->x - 1;
                newPos.y = playerPos->y;
                if (canMove (newPos)) playerPos->x = newPos.x; break;
            case SDLK_d:
                newPos.x = playerPos->x + 1;
                newPos.y = playerPos->y;
                if (canMove (newPos)) playerPos->x = newPos.x; break;
            default: break;
        }
    }

}

