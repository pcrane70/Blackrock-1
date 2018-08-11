#include <time.h>

#include "blackrock.h"
#include "game.h"
#include "map.h"

#include "console.h"

#include "input.h"

// FIXME: 
GameObject *player = NULL;
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
    for (u32 i = 0; i < wallCount; i++) 
        putCharAt (console, walls[i].glyph, walls[i].x, walls[i].y, walls[i].fgColor, walls[i].bgColor);

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
    // TODO: this is only for testing the GO linked list
    GameObject playerData = { .glyph = '@', .fgColor = 0xFFFFFFFF, .bgColor = 0x000000FF };
    player = createGOList (player, &playerData);

    // TODO: at the start of the game we plan to create an initial menu that is in a type of tavern
    // so we need to have the map saved in a file and then loaded here

    // FIXME: for now we are testing our map generation
    // MAP
    wallCount = initWorld (player);

    // Main loop
    // TODO: maybe we want to refactor this
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent (&event) != 0) {
            if (event.type == SDL_QUIT) {
                done = true;
                break;
            }

            handlePlayerInput (event, player);

        }

        renderScreen (renderer, screen, console);
    }

    // Cleanup our GameObjects
    fprintf (stdout, "Cleanning GameObjects...\n");
    // FIXME:
    player = cleanGameObjects (player);
    if (player == NULL) fprintf (stdout, "All GameObjects have been cleared!\n");

    // SDL CLEANUP
    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);

    SDL_Quit ();

    return 0;

}