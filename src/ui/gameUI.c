/*** This file handles the in game UI and the input for playing the game ***/

#include <SDL2/SDL.h>

#include "blackrock.h"
#include "game.h"

#include "ui/ui.h"

#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

#define LOG_WIDTH		60
#define LOG_HEIGHT		5

#define INVENTORY_LEFT		20
#define INVENTORY_TOP		7
#define INVENTORY_WIDTH		40
#define INVENTORY_HEIGHT	30


/*** UI ***/

// Inventory

UIView *inventoryView = NULL;

void toggleInventory (UIScreen *screen) {

    if (inventoryView == NULL) {
       Rect inventoryRect = { (16 * INVENTORY_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT) };
        // FIXME: 
       // inventoryView = 
    }

}


/*** INPUT ***/

// Position *playerPos = NULL;
// Position newPos;


// // TODO: maybe later we will want to move using the numpad insted to allow diagonal movement
// void handlePlayerInput (SDL_Event event) {

//     playerPos = (Position *) getComponent (player, POSITION);

//     if (event.type == SDL_KEYDOWN) {
//         SDL_Keycode key = event.key.keysym.sym;

//         switch (key) {
//             // Movement
//             // TODO: how do we want to handle combat logic?
//             case SDLK_w: 
//                 newPos.x = playerPos->x;
//                 newPos.y = playerPos->y - 1;
//                 if (canMove (newPos)) playerPos->y = newPos.y;
//                 playerTookTurn = true; break;
//             case SDLK_s: 
//                 newPos.x = playerPos->x;
//                 newPos.y = playerPos->y + 1;
//                 if (canMove (newPos)) playerPos->y = newPos.y;
//                 playerTookTurn = true; break;
//             case SDLK_a: 
//                 newPos.x = playerPos->x - 1;
//                 newPos.y = playerPos->y;
//                 if (canMove (newPos)) playerPos->x = newPos.x;
//                 playerTookTurn = true; break;
//             case SDLK_d:
//                 newPos.x = playerPos->x + 1;
//                 newPos.y = playerPos->y;
//                 if (canMove (newPos)) playerPos->x = newPos.x;
//                 playerTookTurn = true; break;

//             // TODO: thi is used as a master key to have interaction with various items
//             // case SDLK_e: break;

//             // TODO: pickup an item
//             // case SDLK_g: break;

//             // TODO: drop an item
//             // case SDLK_d: break;

//             // TODO: toggle inventory
//             // case SDLK_i: break;

//             // TODO: equip an item
//             // case SDLK_q: break;

//             // TODO: toggle character equipment
//             // case SDLK_c: break;

//             // TODO: player rests?
//             // case SDLK_z: break;

//             // TODO: toggle pause menu
//             // case SDLK_p: break;

//             //TODO: what other things do we want?

//             default: break;
//         }
//     }

// }