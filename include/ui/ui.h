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
    UIEventHandler handleEvent;

};


extern UIScreen *activeScene;

extern UIView *newView (Rect pixelRect, u32 colCount, u32 rowCount, 
    char *fontFile, asciiChar firstCharInAtlas, u32 bgColor, bool colorize,
     UIRenderFunction renderFunc);

extern void destroyView (UIView *view);

extern void drawRect (Console *con, Rect *rect, u32 color, i32 borderWidth, u32 borderColor);


#endif