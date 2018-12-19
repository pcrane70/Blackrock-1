#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

#include "ui/ui.h"

extern bool typing;

extern void input_start_typing (void);
extern void input_stop_typing (void);

extern void hanldeMenuEvent (UIScreen *activeScreen, SDL_Event event);
extern void hanldeGameEvent (UIScreen *activeScreen, SDL_Event event);
extern void handlePostGameEvent (UIScreen *activeScreen, SDL_Event event);

#endif