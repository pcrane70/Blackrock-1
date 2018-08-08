#include <time.h>

#include "blackrock.h"
#include "game.h"
#include "map.h"

#include "console.h"

#include "input.h"

// FIXME: 
// global GameObject *player = NULL;

/*** SCREEN ***/

// TODO: are we cleanning up the console and the screen??
// do we want that to happen?
void renderScreen (SDL_Renderer *renderer, SDL_Texture *screen, Console *console) {

    clearConsole (console);

    // test
    // Position *playerPos = (Position *) getComponent (player, POSITION);
    // putCharAt (console, '@', playerPos->x, playerPos->y, 0xFFFFFFFF, 0x000000FF);

    // TODO: is this the most efficient way of doing it?
    // TODO: the logic for fov goes in here!!
    for (u32 i = 1; i < MAX_GO; i++) {
        if (graphicComps[i].objectId > 0) {
            Position *p = (Position *) getComponent (&gameObjects[i], POSITION);
            putCharAt (console, graphicComps[i].glyph, p->x, p->y, graphicComps[i].fgColor, graphicComps[i].bgColor);
        }
    }

    SDL_UpdateTexture (screen, NULL, console->pixels, SCREEN_WIDTH * sizeof (u32));
    SDL_RenderClear (renderer);
    SDL_RenderCopy (renderer, screen, NULL, NULL);
    SDL_RenderPresent (renderer);

}


/*** THREAD ***/

int main (void) {

    srand ((unsigned) time (NULL));

    // SDL SETUP
    SDL_Init (SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow ("Blackrock Dungeons",
         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer (window, 0, SDL_RENDERER_SOFTWARE);

    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize (renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_Texture *screen = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Create our console emulator graphics
    Console *console = initConsole (SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ROWS, NUM_COLS);

    // set up the console font
    setConsoleBitmapFont (console, "./resources/terminal-art.png", 0, 16, 16);

    // FIXME: better player init
    // TODO: how do we want to manage our player?
    GameObject *player = createGameObject ();
    // the position component is handled by the map generator
    // but this will be differnt for the start menu
    // Position pos = { player->id, 25, 25 };
    // addComponentToGO (player, POSITION, &pos);
    Graphics playerGraphics = { player->id, '@', 0xFFFFFFFF, 0x000000FF };
    addComponentToGO (player, GRAPHICS, &playerGraphics);
    Physics physics = { player->id, true, true };
    addComponentToGO (player, PHYSICS, &physics);

    // TODO: at the start of the game we plan to create an initial menu that is in a type of tavern
    // so we need to have the map saved in a file and then loaded here

    // FIXME: for now we are testing our map generation
    // MAP
    initWorld (player);

    // Main loop
    // TODO: maybe we want to refactor this
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent (&event) != 0) {
            if (event.type == SDL_QUIT) {
                done = true;

                // FIXME: VERY IMPORTNAT
                // we don't want to just quit the game...
                // we ned to have a set of process for freeing the memory of the map, and all the gameObjects
                // and their components
                // also we need to check what to save to a file automatticaly 
                // we don't want just a force quit like this!!!

                break;
            }

            handlePlayerInput (event, player);

        }

        renderScreen (renderer, screen, console);
    }


    // SDL CLEANUP
    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);

    SDL_Quit ();

    return 0;

}