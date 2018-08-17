/*** This handles the functions for rendering ui stuff to the screen ***/

#include <string.h>     // for memcpy

#include "blackrock.h"

#include "ui/ui.h"


/*** SCENE MANAGER ***/

UIScreen *activeScene = NULL;

void setActiveScene (UIScreen *screen) {

    if (activeScene != NULL) free (activeScene);
    activeScene = screen;

}




// TODO: we need to refactor somethings here!!
UIView *newView (Rect pixelRect, u32 colCount, u32 rowCount, 
    char *fontFile, asciiChar firstCharInAtlas, u32 bgColor, bool colorize,
     UIRenderFunction renderFunc) {

    UIView *view = (UIView *) malloc (sizeof (UIView));
    Rect *rect = (Rect *) malloc (sizeof (Rect));

    memcpy (rect, &pixelRect, sizeof (Rect));

    // TODO: colorize
    Console *console = initConsole (rect->w, rect->h, rowCount, colCount, bgColor, colorize);

    i32 cellWidthPixels = pixelRect.w / colCount;
    i32 cellHeightPixels = pixelRect.h / rowCount;

    setConsoleBitmapFont (console, fontFile, firstCharInAtlas, cellWidthPixels, cellHeightPixels);

    view->console = console;
    view->pixelRect = rect;
    view->render = renderFunc;

    return view;

}