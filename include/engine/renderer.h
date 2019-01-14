#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>

extern SDL_Window *main_window;
extern SDL_Renderer *main_renderer;

extern void video_init_main (const char *title);
extern void video_destroy_main (void);

extern void render (void);

#endif