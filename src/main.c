#include <time.h>
#include <stdio.h>

#include "blackrock.h"
#include "game.h"
#include "map.h"    // we are only accesing the walls array with this

#include "ui/console.h"

#include "ui/ui.h"
#include "ui/gameUI.h"

#include "input.h"


#define FPS_LIMIT   20

bool running;
void die (void) { running = false; };


/*** SCREEN ***/

// TODO: are we cleanning up the console and the screen??
// do we want that to happen?
void renderScreen (SDL_Renderer *renderer, SDL_Texture *screen, UIScreen *scene) {

    unsigned int test = 0;

    // render the views from back to front for the current screen
    for (ListElement *e = LIST_START (scene->views); e != NULL; e = e->next) {
        UIView *v = (UIView *) LIST_DATA (e);
        clearConsole (v->console);
        v->render (v->console);
        SDL_UpdateTexture (screen, v->pixelRect, v->console->pixels, v->pixelRect->w * sizeof (u32));
    }

    SDL_RenderClear (renderer);
    SDL_RenderCopy (renderer, screen, NULL, NULL);
    SDL_RenderPresent (renderer);

}


/*** CLEAN UP ***/

void cleanUp (SDL_Window *window, SDL_Renderer *renderer, Console *console) {

    // Cleanup our GameObjects and Pools
    fprintf (stdout, "Cleanning GameObjects...\n");
    
    // cleanup data structures
    cleanUpGame ();

    // clean up single structs
    destroyConsole (console);
    // for (short unsigned int i = 0; i < MAP_WIDTH; i++)
    //     free (currentLevel->mapCells[i]);

    // free (currentLevel->mapCells);
    
    free (currentLevel);
    free (player);

    // SDL CLEANUP
    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);

    SDL_Quit ();

}

/*** SET UP ***/

void setUpSDL (SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **screen) {

    SDL_Init (SDL_INIT_VIDEO);
    *window = SDL_CreateWindow ("Blackrock Dungeons",
         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    *renderer = SDL_CreateRenderer (*window, 0, SDL_RENDERER_SOFTWARE);

    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize (*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    *screen = SDL_CreateTexture (*renderer, SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

}


/*** MAIN THREAD ***/

int main (void) {

    srand ((unsigned) time (NULL));

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *screen = NULL;
    setUpSDL (&window, &renderer, &screen);

    // Create our console emulator graphics
    Console *console = initConsole (SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ROWS, NUM_COLS, 0x000000FF, true);
    // set up the console font
    setConsoleBitmapFont (console, "./resources/terminal-art.png", 0, 16, 16);

    // FIXME: 12/08/2018 -- 20:31 -- this is only for tetsing purposes
    initGame ();
    fprintf (stdout, "Number of walls: %i\n", wallCount);


    // Main loop
    running = true;
    bool inGame = true;     // are we in the dungeon?
    SDL_Event event;
    // TODO: display an fps counter if we give a debug option
    u32 timePerFrame;
    u32 frameStart;
    i32 sleepTime;
    UIScreen *screenForInput;
    fprintf (stdout, "Starting main loop\n");

    // FIXME: manually setting the active screen to be the in game scree
    setActiveScene (gameScene ());

    while (running) {
        timePerFrame = 1000 / FPS_LIMIT;
        frameStart = 0;
        
        while (SDL_PollEvent (&event) != 0) {

            frameStart = SDL_GetTicks ();

            if (event.type == SDL_QUIT) running = false;

            // TODO: later we will want the input to be handle diffrently
            // handlePlayerInput (event, player);

            // TODO: how can we have a more eficient event handler?
            // handle the event in the correct screen
            screenForInput = activeScene;
            screenForInput->handleEvent (screenForInput, event);
        }

        // if we are inside the game (dungeon, etc)...
        if (inGame) updateGame ();

        // render the correct screen
        renderScreen (renderer, screen, activeScene);

        // Limit the FPS
        sleepTime = timePerFrame - (SDL_GetTicks () - frameStart);
        if (sleepTime > 0) SDL_Delay (sleepTime);
    }

    cleanUp (window, renderer, console);

    return 0;

}