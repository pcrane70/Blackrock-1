#include <stdio.h>
#include <signal.h>

#include "myos.h"

#include "blackrock.h"

#include "settings.h"

#include "game/game.h"

#include "cengine/input.h"
#include "cengine/renderer.h"
#include "cengine/animation.h"
#include "cengine/thread.h"

#include "utils/log.h"
#include "utils/myUtils.h"

bool running = true;
bool inGame = false;
bool wasInGame = false;

/*** MISC ***/

void quit (int code) {

    running = false;
    inGame = false;

}

void die (const char *error) {

    logMsg (stderr, ERROR, NO_TYPE, error);
    quit (1);

};

static int init (void) {

    int errors = 0;

    // register to some signals
    signal (SIGINT, quit);
    signal (SIGSEGV, quit);

    main_settings = settings_load ();

    if (!SDL_Init (SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        errors = video_init_main ("Blackrock Dungeons");

        errors = animations_init ();
        errors = ui_init ();
    }

    else {
        logMsg (stderr, ERROR, NO_TYPE, "Unable to initialize SDL!");
        #ifdef BLACK_DEBUG
        logMsg (stderr, ERROR, NO_TYPE, createString ("%s", SDL_GetError ()));
        #endif
        errors = 1;
    }

    return errors;

}

static int end (void) {

    game_manager->currState->onExit ();
    game_manager_delete (game_manager);

    // if (multiplayer) multiplayer_stop ();

    // TODO: do we want to call this when we exit the game state instead?
    game_clean_up ();
    
    animations_end ();
    ui_destroy ();
    video_destroy_main ();

    SDL_Quit ();

    return 0;

}

static pthread_t update_thread;

void *update (void *args) {

    thread_set_name ("update");

    u32 timePerFrame = 1000 / FPS_LIMIT;
    u32 frameStart = 0;
    i32 sleepTime = 0;

    float deltaTime = 0;
    u32 deltaTicks = 0;
    u32 fps = 0;

    while (running) {
        frameStart = SDL_GetTicks ();

        if (game_manager->currState->update)
            game_manager->currState->update ();

        // limit the FPS
        sleepTime = timePerFrame - (SDL_GetTicks () - frameStart);
        if (sleepTime > 0) SDL_Delay (sleepTime);

        // count fps
        deltaTime = SDL_GetTicks () - frameStart;
        deltaTicks += deltaTime;
        fps++;
        if (deltaTicks >= 1000) {
            // printf ("update fps: %i\n", fps);
            deltaTicks = 0;
            fps = 0;
        }
    }
    
}

static void run (void) {

    SDL_Event event;

    u32 timePerFrame = 1000 / FPS_LIMIT;
    u32 frameStart = 0;
    i32 sleepTime = 0;

    float deltaTime = 0;
    u32 deltaTicks = 0;
    u32 fps = 0;

    while (running) {
        frameStart = SDL_GetTicks ();

        input_handle (event);

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

}

/*** MAIN THREAD ***/

int main (void) {

    running = !init () ? true : false;

    // char *text = (char *) calloc (20, sizeof (char));

    TextBox *static_text = ui_textBox_create_static (100, 100, ui_rgba_color_create (100, 45, 67, 255),
        "this is a static text!", RGBA_WHITE, NULL, false);
    TextBox *volatile_text = ui_textBox_create_volatile (200, 200, ui_rgba_color_create (100, 45, 67, 255),
        "this is a volatile text!", RGBA_WHITE, NULL, false);

    // TODO: modify to the correct game state
    game_state = game_state_new ();
    game_manager = game_manager_new (game_state);

    if (pthread_create (&update_thread, NULL, update, NULL)) {
        logMsg (stderr, ERROR, NO_TYPE, "Failed to create update thread!");
        running = false;
    }

    run ();

    return end ();

}