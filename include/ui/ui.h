#ifndef UI_H_
#define UI_H_

#include <SDL2/SDL.h>

#include "console.h"

#include "list.h"


struct UIScreen;
typedef struct UIScreen UIScreen;

typedef void (*UIRenderFunction)(Console *);
typedef void (*UIEventHandler)(struct UIScreen *, SDL_Event);

typedef struct {

    Console *console;
    Rect *pixelRect;
    UIRenderFunction render;

} UIView;

struct UIScreen {

    List *views;
    UIView *activeView;
    UIEventHandler handle_event;

};


extern UIScreen *activeScene;


#endif