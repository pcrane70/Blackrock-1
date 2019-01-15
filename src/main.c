#include <stdio.h>

#include "blackrock.h"

#include "engine/renderer.h"

#include "game/game.h"

#include "input.h"

#include "ui/ui.h"
#include "ui/gameUI.h"

#include "utils/log.h"

bool running = false;
bool inGame = false;
bool wasInGame = false;

TextBox **selected_textBox = NULL;

/*** MISC ***/

void die (const char *error) {

    perror (error);
    running = false;

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

// pthread_t gameThread;

int main (void) {

    SDL_Init (SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    video_init_main ("Blackrock Dungeons");

    SDL_Event event;

    // TODO: modify to the correct game state
    game_state = game_state_new ();
    game_manager = game_manager_new (game_state);

    // FIXME: init new UI
    UIScreen *screenForInput;
    extern UIScreen *menuScene (void);
    setActiveScene (menuScene ());

    u32 timePerFrame = 1000 / FPS_LIMIT;
    u32 frameStart = 0;
    i32 sleepTime = 0;

    float deltaTime = 0;
    u32 deltaTicks = 0;
    u32 fps = 0;

    char *text = (char *) calloc (20, sizeof (char));

    running = true;
    while (running) {
        frameStart = SDL_GetTicks ();
        
        /* while (SDL_PollEvent (&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
                inGame = false;
            } 

            else if (typing && event.type == SDL_TEXTINPUT) 
                ui_textBox_update_text (*selected_textBox, event.text.text);

            else {
                // handle the event in the correct screen
                screenForInput = activeScene;
                screenForInput->handleEvent (screenForInput, event);
            }
        }

        // render the correct screen
        // renderScreen (renderer, screen, activeScene); */

        /*** NEW LOOP ***/

        input_handle (event);

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
            printf ("fps: %i\n", fps);
            deltaTicks = 0;
            fps = 0;
        }
    }

    // if (wasInGame)
    //     if (pthread_join (gameThread, NULL) != THREAD_OK)
    //         logMsg (stderr, ERROR, NO_TYPE, "Failed to join game thread!");

    // if (multiplayer) multiplayer_stop ();

    // FIXME: I dont want these here!
    // FIXME: cleanup
    // game_cleanUp ();
    // ui_destroy ();
    video_destroy_main ();
    SDL_Quit ();

    return 0;

}