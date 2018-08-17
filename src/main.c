#include <time.h>
#include <stdio.h>

#include "blackrock.h"
#include "game.h"
#include "map.h"    // we are only accesing the walls array with this

#include "ui/console.h"

#include "input.h"


#define FPS_LIMIT   20


/*** SCREEN ***/

// TODO: are we cleanning up the console and the screen??
// do we want that to happen?
void renderScreen (SDL_Renderer *renderer, SDL_Texture *screen, Console *console) {

    unsigned int test = 0;

    clearConsole (console);

    // TODO: the logic for fov goes in here!!

    // render the player
    // TODO: do we want the player to have its own structure??
    Position *playerPos = (Position *) getComponent (player, POSITION);
    Graphics *playerGra = (Graphics *) getComponent (player, GRAPHICS);
    putCharAt (console, playerGra->glyph, playerPos->x, playerPos->y, playerGra->fgColor, playerGra->bgColor);

    // TODO:
    // render the go with graphics
    GameObject *go = NULL;
    Position *p = NULL;
    Graphics *g = NULL;
    for (ListElement *ptr = LIST_START (gameObjects); ptr != NULL; ptr = ptr->next) {
        go = (GameObject *) ptr->data;
        p = (Position *) getComponent (go, POSITION);
        g = (Graphics *) getComponent (go, GRAPHICS);
        putCharAt (console, g->glyph, p->x, p->y, g->fgColor, g->bgColor);
    }

    // FIXME: we don't want to this every frame!!
    for (unsigned int i = 0; i < wallCount; i++) 
        putCharAt (console, walls[i].glyph, walls[i].x, walls[i].y, walls[i].fgColor, walls[i].bgColor);

    SDL_UpdateTexture (screen, NULL, console->pixels, SCREEN_WIDTH * sizeof (u32));
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
    Console *console = initConsole (SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ROWS, NUM_COLS);
    // set up the console font
    setConsoleBitmapFont (console, "./resources/terminal-art.png", 0, 16, 16);

    // FIXME: 12/08/2018 -- 20:31 -- this is only for tetsing purposes
    initGame ();
    fprintf (stdout, "Number of walls: %i\n", wallCount);


    // Main loop
    bool running = true;
    SDL_Event event;
    // TODO: display an fps counter if we give a debug option
    u32 timePerFrame;
    u32 frameStart;
    i32 sleepTime;
    fprintf (stdout, "Starting main loop\n");
    while (running) {
        timePerFrame = 1000 / FPS_LIMIT;
        frameStart = 0;
        
        while (SDL_PollEvent (&event) != 0) {

            frameStart = SDL_GetTicks ();

            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }

            // TODO: later we will want the input to be handle diffrently
            handlePlayerInput (event, player);

        }

        // if we are inside the game (dungeon, etc)...
        updateGame ();

        // TODO: if we are in game, update that, else we are in a menu or something else
        renderScreen (renderer, screen, console);

        // Limit the FPS
        sleepTime = timePerFrame - (SDL_GetTicks () - frameStart);
        if (sleepTime > 0) SDL_Delay (sleepTime);
    }

    cleanUp (window, renderer, console);

    return 0;

}