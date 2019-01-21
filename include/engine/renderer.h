#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>

extern SDL_Window *main_window;
extern SDL_Renderer *main_renderer;

typedef struct ScreenSize {

    u32 width, height;

} ScreenSize;

extern ScreenSize maxScreenSize;
extern ScreenSize currentScreenSize;

extern void window_toggle_full_screen (SDL_Window *window);
extern void window_resize (SDL_Window *window, u32 newWidth, u32 newHeight);

extern void video_init_main (const char *title);
extern void video_destroy_main (void);

extern void render (void);

#endif