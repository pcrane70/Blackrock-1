#include <stdio.h>
#include <signal.h>

#include "blackrock.h"

#include "settings.h"

#include "game/game.h"

#include "engine/input.h"
#include "engine/renderer.h"
#include "engine/animation.h"

#include "utils/log.h"
#include "utils/myUtils.h"

bool running = false;
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

    // if (multiplayer) multiplayer_stop ();

    game_cleanUp ();
    animations_end ();
    ui_destroy ();
    video_destroy_main ();

    SDL_Quit ();

    return 0;

}

static void run (void) {

    SDL_Event event;

    u32 timePerFrame = 1000 / FPS_LIMIT;
    u32 frameStart = 0;
    i32 sleepTime = 0;

    u32 deltaTicks = 0;

    while (running) {
        frameStart = SDL_GetTicks ();

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
            // printf ("main fps: %i\n", fps);
            deltaTicks = 0;
            fps = 0;
        }
    }

}

/*** MAIN THREAD ***/

float deltaTime = 0;
u32 fps = 0;

int main (void) {

    running = true;

    if (init ()) {
        logMsg (stderr, ERROR, NO_TYPE, "Failed to init blackrock!");
        running = false;
    }
    
    // char *text = (char *) calloc (20, sizeof (char));

    TextBox *static_text = ui_textBox_create_static (100, 100, ui_rgba_color_create (100, 45, 67, 255),
        "this is a static text!", RGBA_WHITE, NULL, false);
    TextBox *volatile_text = ui_textBox_create_volatile (200, 200, ui_rgba_color_create (100, 45, 67, 255),
        "this is a volatile text!", RGBA_WHITE, NULL, false);

    // TODO: modify to the correct game state
    game_state = game_state_new ();
    game_manager = game_manager_new (game_state);

    run ();

    return end ();

}