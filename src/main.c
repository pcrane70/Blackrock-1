#include <time.h>

#include "blackrock.h"
#include "game.h"
#include "map.h"

#include "console.h"

#include "input.h"


#define FPS_LIMIT   20


unsigned int wallCount;

/*** SCREEN ***/

// TODO: are we cleanning up the console and the screen??
// do we want that to happen?
void renderScreen (SDL_Renderer *renderer, SDL_Texture *screen, Console *console) {

    clearConsole (console);

    // TODO: the logic for fov goes in here!!

    // while (ptr != NULL) {
    //     putCharAt (console, ptr->glyph, ptr->x, ptr->y, ptr->fgColor, ptr->bgColor) ;
    //     ptr = ptr->next;
    // }

    // FIXME: we don't want to this every frame!!
    // for (u32 i = 0; i < wallCount; i++) 
    //     putCharAt (console, walls[i].glyph, walls[i].x, walls[i].y, walls[i].fgColor, walls[i].bgColor);

    SDL_UpdateTexture (screen, NULL, console->pixels, SCREEN_WIDTH * sizeof (u32));
    SDL_RenderClear (renderer);
    SDL_RenderCopy (renderer, screen, NULL, NULL);
    SDL_RenderPresent (renderer);

}

/*** SET UP ***/

// SDL SETUP
void initSDL (SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *screen) {

    SDL_Init (SDL_INIT_VIDEO);
    window = SDL_CreateWindow ("Blackrock Dungeons",
         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    renderer = SDL_CreateRenderer (window, 0, SDL_RENDERER_SOFTWARE);

    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize (renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    screen = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

}


/*** CLEAN UP ***/

void cleanUp (SDL_Window *window, SDL_Renderer *renderer) {

    // Cleanup our GameObjects and Pools
    // fprintf (stdout, "Cleanning GameObjects...\n");
    // if (cleanUpGame () == 0) fprintf (stdout, "All GameObjects have been cleared!\n");
    // else fprintf (stderr, "Error cleanning GOs!! Quiting anyway...\n");

    // SDL CLEANUP
    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);

    SDL_Quit ();

}


/*** MAIN THREAD ***/

int main (void) {

    srand ((unsigned) time (NULL));

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *screen = NULL;
    initSDL (window, renderer, screen);

    // Create our console emulator graphics
    Console *console = initConsole (SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ROWS, NUM_COLS);
    // set up the console font
    setConsoleBitmapFont (console, "./resources/terminal-art.png", 0, 16, 16);

    // FIXME: The player is a global variable, 
    // but when we have an init screen, we don't want to initilize him here!!!
    // player = initPlayer ();

    // TODO: at the start of the game we plan to create an initial menu that is in a type of tavern
    // so we need to have the map saved in a file and then loaded here

    // FIXME: for now we are testing our map generation
    // MAP
    // wallCount = initWorld (player);

    // Main loop
    bool running = true;
    SDL_Event event;
    // TODO: display an fps counter if we give a debug option
    u32 timePerFrame;
    u32 frameStart;
    i32 sleepTime;
    while (running) {
        timePerFrame = 1000 / FPS_LIMIT;
        u32 frameStart = 0;
        
        while (SDL_PollEvent (&event) != 0) {

            frameStart = SDL_GetTicks ();

            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }

            // TODO: later we will want the input to be handle diffrently
            handlePlayerInput (event, player);

        }

        // TODO: if we are in game, update that, else we are in a menu or something else
        renderScreen (renderer, screen, console);

        // Limit the FPS
        sleepTime = timePerFrame - (SDL_GetTicks () - frameStart);
        if (sleepTime > 0) SDL_Delay (sleepTime);
    }

    cleanUp (window, renderer);

    return 0;

}