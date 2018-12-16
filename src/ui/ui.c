/*** This handles the functions for rendering ui stuff to the screen ***/

#include <string.h>

#include "blackrock.h"

#include "resources.h"

#include "ui/ui.h"

char *tileset = "./resources/terminal-art.png";  

/*** SCENE MANAGER ***/

UIScreen *activeScene = NULL;

// don't forget to manually clean the previous screen
void setActiveScene (UIScreen *newScreen) { activeScene = newScreen; }

// FIXME:
void destroyUIScreen (UIScreen *screen) {

    /* if (screen != NULL) {
        if (screen->views != NULL) {
            if (LIST_SIZE (screen->views) > 0) {
                screen->activeView = NULL;

                // destroy screen views
                while (LIST_SIZE (screen->views) > 0) 
                    ui_destroyView ((UIView *) removeElement (screen->views, NULL));

            }

            destroyList (screen->views);
        }

        free (screen);
    } */

}

CleanUI destroyCurrentScreen;

/*** UI ***/

UIView *ui_newView (UIRect pixelRect, u32 colCount, u32 rowCount, 
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

void ui_destroyView (void *data) {

    if (data) {
        UIView *view = (UIView *) data;
        if (view->pixelRect) free (view->pixelRect);
        destroyConsole (view->console);
        free (view);
    }

}

void ui_drawRect (Console *con, UIRect *rect, u32 color, i32 borderWidth, u32 borderColor) {

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

/*** IMAGE DRAWING ***/

void ui_drawImageAt (Console *console, BitmapImage *image, i32 cellX, i32 cellY) {

    u32 dstX = cellX * console->cellWidth;
    for (u32 srcY = 0; srcY < image->height; srcY++) {
        u32 dstY = (cellY * console->cellHeight) + srcY;
        memcpy (&console->pixels[(dstY * console->width) + dstX],
            &image->pixels[srcY * image->width], image->width * sizeof (u32));
    }

}

/*** UI ELEMENTS ***/

TextBox *ui_textBox_create (u8 x, u8 y, u8 w, u8 h, u32 bgcolor, const char *text) {

    TextBox *new_textBox = (TextBox *) malloc (sizeof (TextBox));
    if (new_textBox) {
        new_textBox->bgrect = (UIRect *) malloc (sizeof (UIRect));
        new_textBox->bgrect->x = x;
        new_textBox->bgrect->y = y;
        new_textBox->bgrect->w = w;
        new_textBox->bgrect->h = h;

        new_textBox->bgcolor = bgcolor;

        new_textBox->text = (char *) calloc (64, sizeof (char));
        if (text)
            strcpy (new_textBox->text, text);
    }

    return new_textBox;

}

void ui_textBox_destroy (TextBox *textbox) {

    if (textbox) {
        if (textbox->bgrect) free (textbox->bgrect);
        if (textbox->text) free (textbox->text);

        free (textbox);
    }

}

void ui_textBox_draw (Console *console, TextBox *textbox) {

    if (console && textbox) {
        ui_drawRect (console, textbox->bgrect, textbox->bgcolor, 0, NO_COLOR);

        if (textbox->text)
            putStringAt (console, textbox->text, textbox->bgrect->x + 1, textbox->bgrect->y + 1, 
                BLACK, NO_COLOR);
    }
        
}