#include <SDL2/SDL.h>

#include "engine/renderer.h"

#include "game.h"
#include "ui.h"

SDL_Window *main_window = NULL;
SDL_Renderer *main_renderer = NULL;

// TODO: i dont want this here!
extern void ui_cursor_draw (void);

// FIXME:
// TODO: render by layers
void render (void) {

    SDL_RenderClear (main_renderer);

    // render current game screen
    if (game_manager->currState->render)
        game_manager->currState->render ();

    // render game UI
    // TextBox *textBox = NULL;
    // for (u32 i = 0; i < curr_max_ui_elements; i++) {
    //     switch (ui_elements[i]->type) {
    //         case UI_TEXTBOX: 
    //             textBox = (TextBox *) ui_elements[i]->element;
    //             if (textBox->isVolatile) ui_textbox_draw (textBox);
    //             else SDL_RenderCopy (main_renderer, textBox->texture, NULL, &textBox->bgrect);

    //             break;
    //         case UI_BUTTON: break;

    //         default: break;
    //     }
    // }

    // // render the cursor on top of everything
    // ui_cursor_draw ();

    SDL_RenderPresent (main_renderer);

}

/*** OLD WAY OF RENDERING ***/

/*** SCREEN ***/

// FIXME: move this to a separate file
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

static void render_init_main (void) {

    main_renderer = SDL_CreateRenderer (main_window, 0, SDL_RENDERER_SOFTWARE | SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor (main_renderer, 0, 0, 0, 255);
    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RenderSetLogicalSize (main_renderer, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);

}

static void window_init_main (const char *title) {

    main_window = SDL_CreateWindow (title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, 0);

}

void video_init_main (const char *title) {

    window_init_main (title);
    render_init_main ();

}

void video_destroy_main (void) {

    SDL_DestroyRenderer (main_renderer);
    SDL_DestroyWindow (main_window);

}