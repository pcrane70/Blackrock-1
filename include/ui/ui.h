#ifndef UI_H_
#define UI_H_

#include <SDL2/SDL.h>

#include "resources.h"

#include "console.h"

#include "utils/dlist.h"

/*** COMMON COLORS ***/

#define NO_COLOR        0x00000000
#define WHITE           0xFFFFFFFF
#define BLACK           0x000000FF

#define FULL_GREEN      0x00FF00FF
#define FULL_RED        0xFF0000FF

#define YELLOW          0xFFD32AFF
#define SAPPHIRE        0x1E3799FF

/*** FULL SCREEN ***/

#define FULL_SCREEN_LEFT		0
#define FULL_SCREEN_TOP		    0
#define FULL_SCREEN_WIDTH		80
#define FULL_SCREEN_HEIGHT	    45

typedef SDL_Rect UIRect;

struct UIScreen;

typedef void (*UIRenderFunction)(Console *);
typedef void (*UIEventHandler)(struct UIScreen *, SDL_Event);

typedef struct UIView {

    Console *console;
    UIRect *pixelRect;
    UIRenderFunction render;

} UIView;

typedef struct UIScreen {

    DoubleList *views;
    UIView *activeView;
    UIEventHandler handleEvent;

} UIScreen;

/*** SCREENS ***/

extern UIScreen *activeScene;
extern void setActiveScene (UIScreen *);
extern void destroyUIScreen (UIScreen *);

typedef void (*CleanUI)(void);
extern CleanUI destroyCurrentScreen;

/*** VIEWS **/

extern UIView *ui_newView (UIRect pixelRect, u32 colCount, u32 rowCount, 
    char *fontFile, asciiChar firstCharInAtlas, u32 bgColor, bool colorize,
     UIRenderFunction renderFunc);

extern void ui_destroyView (void *data);

extern void ui_drawRect (Console *con, UIRect *rect, u32 color, i32 borderWidth, u32 borderColor);

extern void ui_drawImageAt (Console *console, BitmapImage *image, i32 cellX, i32 cellY);


extern char *tileset;


#endif