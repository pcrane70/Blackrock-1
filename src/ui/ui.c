/*** This handles the functions for rendering ui stuff to the screen ***/

#include <string.h>     // for memcpy

#include "blackrock.h"

#include "ui/ui.h"

// TODO: maybe in the future we acn add more graphics, but for now we are sticking with
// only ascii chars
// In linux we have to take the path from the makefile 
char *tileset = "./resources/terminal-art.png";  


/*** SCENE MANAGER ***/

UIScreen *activeScene = NULL;

void setActiveScene (UIScreen *screen) {

    if (activeScene != NULL) free (activeScene);
    activeScene = screen;

}

/*** UI ***/

UIView *newView (UIRect pixelRect, u32 colCount, u32 rowCount, 
    char *fontFile, asciiChar firstCharInAtlas, u32 bgColor, bool colorize,
     UIRenderFunction renderFunc) {

    UIView *view = (UIView *) malloc (sizeof (UIView));
    UIRect *rect = (UIRect *) malloc (sizeof (Rect));

    memcpy (rect, &pixelRect, sizeof (Rect));

    Console *console = initConsole (rect->w, rect->h, rowCount, colCount, bgColor, colorize);

    i32 cellWidthPixels = pixelRect.w / colCount;
    i32 cellHeightPixels = pixelRect.h / rowCount;

    setConsoleBitmapFont (console, fontFile, firstCharInAtlas, cellWidthPixels, cellHeightPixels);

    view->console = console;
    view->pixelRect = rect;
    view->render = renderFunc;

    return view;

}

void destroyView (UIView *view) {

    if (view != NULL) {
        printf ("Destroying view");
        free (view->pixelRect);
        destroyConsole (view->console);
        // free (view);
    }

    else printf ("Inventory view is NULL!");

}

void drawRect (Console *con, UIRect *rect, u32 color, i32 borderWidth, u32 borderColor) {

    char c;
    for (u32 y = rect->y; y < rect->y + rect->h; y++) {
        for (i32 x = rect->x; x < rect->x + rect->w; x++) {
            c = ' ';
            if (borderWidth > 0) {
                // sides
                if ((x == rect->x) || (x == rect->x + rect->w - 1))
                    c = (borderWidth == 1) ? 179 : 186;

                // top
                if (y == rect->y) {
                    // top left corner
                    if (x == rect->x) c = (borderWidth == 1) ? 218 : 201;

                    // top right corner
                    else if (x == rect->x + rect->w - 1)
                        c = (borderWidth == 1) ? 191 : 187;

                    // top border
                    else c = (borderWidth == 1) ? 196 : 205;
                }

                // bottom 
                if (y == rect->y + rect->h - 1) {
                    // bottom left corner
                    if (x == rect->x) c = (borderWidth == 1) ? 192 : 200;

                    // bottom right corner
                    else if (x == rect->x + rect->w - 1)
                        c = (borderWidth == 1) ? 217 : 188;

                    else c = (borderWidth == 1) ? 196 : 205;
                }
            }

            putCharAt (con, c, x, y, borderColor, color);
        }
    }

}