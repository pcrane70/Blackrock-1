#include <stdint.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 768

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t i32;
typedef int64_t i64;

/*** SCREEN ***/

void renderScreen (SDL_Renderer *renderer, SDL_Texture *screen) {

    u32 *pixels = (u32 *) calloc (SCREEN_WIDTH * SCREEN_HEIGHT, sizeof (u32));

    SDL_UpdateTexture (screen, NULL, pixels, SCREEN_WIDTH * sizeof (u32));
    SDL_RenderClear (renderer);
    SDL_RenderCopy (renderer, screen, NULL, NULL);
    SDL_RenderPresent (renderer);

}


/*** THREAD ***/

int main (void) {

    // SDL SETUP
    SDL_Init (SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow ("Blackrock Dungeons",
         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer (window, 0, SDL_RENDERER_SOFTWARE);

    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize (renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_Texture *screen = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

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
        }

        renderScreen (renderer, screen);
    }


    // SDL CLEANUP
    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);

    SDL_Quit ();

    return 0;

}