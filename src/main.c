#include <time.h>
#include <stdio.h>
#include <pthread.h>

#include "blackrock.h"
#include "game.h"

#include "input.h"

#include "ui/console.h"
#include "ui/ui.h"
#include "ui/gameUI.h"

#include "utils/log.h"

#define FPS_LIMIT   30

bool running = false;
bool inGame = false;
bool wasInGame = false;

TextBox **selected_textBox = NULL;

/*** MISC ***/

void die (const char *error) {

    perror (error);
    running = false;

};

void pthread_create_detachable (void *(*work) (void *), void *args) {

    pthread_attr_t attr;
    pthread_t request_Thread;

    int rc = pthread_attr_init (&attr);
    rc = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

   if (pthread_create (&request_Thread, &attr, work, args)
        != THREAD_OK)
        logMsg (stderr, ERROR, NO_TYPE, "Failed o create request thread!");

}

/*** SCREEN ***/

// TODO: are we cleanning up the console and the screen??
// do we want that to happen?
void renderScreen (SDL_Renderer *renderer, SDL_Texture *screen, UIScreen *scene) {

    // render the views from back to front for the current screen
    UIView *v = NULL;
    for (ListElement *e = LIST_START (scene->views); e != NULL; e = e->next) {
        v = (UIView *) LIST_DATA (e);
        clearConsole (v->console);
        v->render (v->console);
        SDL_UpdateTexture (screen, v->pixelRect, v->console->pixels, v->pixelRect->w * sizeof (u32));
    }

    SDL_RenderClear (renderer);
    SDL_RenderCopy (renderer, screen, NULL, NULL);
    SDL_RenderPresent (renderer);

}

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

/*** SET UP ***/

void setUpSDL (SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **screen) {

    SDL_Init (SDL_INIT_VIDEO);
    *window = SDL_CreateWindow ("Blackrock Dungeons",
         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    *renderer = SDL_CreateRenderer (*window, 0, SDL_RENDERER_SOFTWARE | SDL_RENDERER_ACCELERATED);

    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize (*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    *screen = SDL_CreateTexture (*renderer, SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

}

/*** MAIN THREAD ***/

pthread_t gameThread;

int main (void) {

    srand ((unsigned) time (NULL));

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *screen = NULL;
    setUpSDL (&window, &renderer, &screen);

    SDL_Event event;
    UIScreen *screenForInput;

    extern UIScreen *menuScene (void);
    setActiveScene (menuScene ());

    u32 timePerFrame = 1000 / FPS_LIMIT;
    u32 frameStart;
    i32 sleepTime;

    char *text = (char *) calloc (20, sizeof (char));

    running = true;
    while (running) {
        frameStart = SDL_GetTicks ();
        
        while (SDL_PollEvent (&event) != 0) {
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
        renderScreen (renderer, screen, activeScene);

        // limit the FPS
        sleepTime = timePerFrame - (SDL_GetTicks () - frameStart);
        if (sleepTime > 0) SDL_Delay (sleepTime);
    }

    if (wasInGame)
        if (pthread_join (gameThread, NULL) != THREAD_OK)
            fprintf (stderr, "Failed to join game thread!\n");

    if (multiplayer) multiplayer_stop ();

    cleanUp (window, renderer);

    return 0;

}