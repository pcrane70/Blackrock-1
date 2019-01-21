#include <stdio.h>
#include <signal.h>

#include "blackrock.h"

#include "game/game.h"

#include "engine/input.h"
#include "engine/renderer.h"
#include "engine/animation.h"

#include "utils/log.h"

bool running = false;
bool inGame = false;
bool wasInGame = false;

/*** MISC ***/

void quit (int dummy) {

    running = false;
    inGame = false;

}

void die (const char *error) {

    logMsg (stderr, ERROR, NO_TYPE, error);
    quit (1);

};

/*** CLEAN UP ***/

extern void cleanUpMenuScene (void);

void cleanUp (SDL_Window *window, SDL_Renderer *renderer) {

    if (wasInGame) cleanUpGame ();

    // clean the UI
    destroyCurrentScreen ();
    
    // SDL CLEANUP
    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);

    SDL_Quit ();

}

/*** MAIN THREAD ***/

float deltaTime = 0;
u32 fps = 0;

int main (void) {

    // register to the quit signal
    signal (SIGINT, quit);

    SDL_Init (SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO); 
    video_init_main ("Blackrock Dungeons");

    SDL_Event event;

    u32 timePerFrame = 1000 / FPS_LIMIT;
    u32 frameStart = 0;
    i32 sleepTime = 0;

    u32 deltaTicks = 0;

    char *text = (char *) calloc (20, sizeof (char));

    running = true;
    animations_init ();

    // TODO: modify to the correct game state
    game_state = game_state_new ();
    game_manager = game_manager_new (game_state);

    while (running) {
        frameStart = SDL_GetTicks ();

        input_handle (event);

        if (input_is_key_down (SDL_SCANCODE_T))
            window_resize (main_window, 1920, 1080);
        
            // window_toggle_full_screen (main_window);

        // TODO: create a separate thread
        if (game_manager->currState->update)
            game_manager->currState->update ();

        // rendering is done in the main thread
        render ();

        // limit the FPS
        sleepTime = timePerFrame - (SDL_GetTicks () - frameStart);
        if (sleepTime > 0) SDL_Delay (sleepTime);

        // count fps
        deltaTime = SDL_GetTicks () - frameStart;
        deltaTicks += deltaTime;
        fps++;
        if (deltaTicks >= 1000) {
            printf ("main fps: %i\n", fps);
            deltaTicks = 0;
            fps = 0;
        }
    }

    // if (multiplayer) multiplayer_stop ();

    // FIXME: I dont want these here!
    // FIXME: cleanup
    game_cleanUp ();
    animations_end ();
    // ui_destroy ();
    video_destroy_main ();
    SDL_Quit ();

    return 0;

}