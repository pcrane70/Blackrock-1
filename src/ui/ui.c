// /*** This handles the functions for rendering ui stuff to the screen ***/

// #include <string.h>

// #include "blackrock.h"

// #include "ui/ui.h"

// #include "utils/myUtils.h"

// char *tileset = "./resources/terminal-art.png";  

// /*** SCENE MANAGER ***/

// UIScreen *activeScene = NULL;

// // don't forget to manually clean the previous screen
// void setActiveScene (UIScreen *newScreen) { activeScene = newScreen; }

// // FIXME:
// void destroyUIScreen (UIScreen *screen) {

//     /* if (screen != NULL) {
//         if (screen->views != NULL) {
//             if (LIST_SIZE (screen->views) > 0) {
//                 screen->activeView = NULL;

//                 // destroy screen views
//                 while (LIST_SIZE (screen->views) > 0) 
//                     ui_destroyView ((UIView *) removeElement (screen->views, NULL));

//             }

//             destroyList (screen->views);
//         }

//         free (screen);
//     } */

// }

// CleanUI destroyCurrentScreen;

// /*** UI ***/

// UIView *ui_newView (UIRect pixelRect, u32 colCount, u32 rowCount, 
//     char *fontFile, asciiChar firstCharInAtlas, u32 bgColor, bool colorize,
//      UIRenderFunction renderFunc) {

//     UIView *view = (UIView *) malloc (sizeof (UIView));
//     UIRect *rect = (UIRect *) malloc (sizeof (Rect));

//     memcpy (rect, &pixelRect, sizeof (Rect));

//     Console *console = initConsole (rect->w, rect->h, rowCount, colCount, bgColor, colorize);

//     i32 cellWidthPixels = pixelRect.w / colCount;
//     i32 cellHeightPixels = pixelRect.h / rowCount;

//     setConsoleBitmapFont (console, fontFile, firstCharInAtlas, cellWidthPixels, cellHeightPixels);

//     view->console = console;
//     view->pixelRect = rect;
//     view->render = renderFunc;

//     return view;

// }

// void ui_destroyView (void *data) {

//     if (data) {
//         UIView *view = (UIView *) data;
//         if (view->pixelRect) free (view->pixelRect);
//         destroyConsole (view->console);
//         free (view);
//     }

// }

// void ui_drawRect (Console *con, UIRect *rect, u32 color, i32 borderWidth, u32 borderColor) {

//     char c;
//     for (u32 y = rect->y; y < rect->y + rect->h; y++) {
//         for (i32 x = rect->x; x < rect->x + rect->w; x++) {
//             c = ' ';
//             if (borderWidth > 0) {
//                 // sides
//                 if ((x == rect->x) || (x == rect->x + rect->w - 1))
//                     c = (borderWidth == 1) ? 179 : 186;

//                 // top
//                 if (y == rect->y) {
//                     // top left corner
//                     if (x == rect->x) c = (borderWidth == 1) ? 218 : 201;

//                     // top right corner
//                     else if (x == rect->x + rect->w - 1)
//                         c = (borderWidth == 1) ? 191 : 187;

//                     // top border
//                     else c = (borderWidth == 1) ? 196 : 205;
//                 }

//                 // bottom 
//                 if (y == rect->y + rect->h - 1) {
//                     // bottom left corner
//                     if (x == rect->x) c = (borderWidth == 1) ? 192 : 200;

//                     // bottom right corner
//                     else if (x == rect->x + rect->w - 1)
//                         c = (borderWidth == 1) ? 217 : 188;

//                     else c = (borderWidth == 1) ? 196 : 205;
//                 }
//             }

//             putCharAt (con, c, x, y, borderColor, color);
//         }
//     }

// }

// /*** IMAGE DRAWING ***/

// // void ui_drawImageAt (Console *console, BitmapImage *image, i32 cellX, i32 cellY) {

// //     u32 dstX = cellX * console->cellWidth;
// //     for (u32 srcY = 0; srcY < image->height; srcY++) {
// //         u32 dstY = (cellY * console->cellHeight) + srcY;
// //         memcpy (&console->pixels[(dstY * console->width) + dstX],
// //             &image->pixels[srcY * image->width], image->width * sizeof (u32));
// //     }

// // }

// /*** UI ELEMENTS ***/

// TextBox *ui_textBox_create (u8 x, u8 y, u8 w, u8 h, u32 bgcolor, 
//     const char *text, bool password, u32 textColor) {

//     TextBox *new_textBox = (TextBox *) malloc (sizeof (TextBox));
//     if (new_textBox) {
//         new_textBox->bgrect = (UIRect *) malloc (sizeof (UIRect));
//         new_textBox->bgrect->x = x;
//         new_textBox->bgrect->y = y;
//         new_textBox->bgrect->w = w;
//         new_textBox->bgrect->h = h;

//         new_textBox->bgcolor = bgcolor;
//         new_textBox->textColor = textColor;

//         new_textBox->text = (char *) calloc (64, sizeof (char));

//         new_textBox->ispassword = password;
//         if (password) {
//             new_textBox->pswd = (char *) calloc (64, sizeof (char));

//             if (text) {
//                 strcpy (new_textBox->pswd, text);
//                 u32 len = strlen (text);
//                 for (u8 i = 0; i < len; i++) new_textBox->text[i] = '*';
//             }
//         }

//         else {
//             if (text) strcpy (new_textBox->text, text);
//             new_textBox->pswd = NULL;
//         } 

//         new_textBox->borderWidth = 0;
//         new_textBox->borderColor = NO_COLOR;
//     }

//     return new_textBox;

// }

// void ui_textBox_destroy (TextBox *textbox) {

//     if (textbox) {
//         if (textbox->bgrect) free (textbox->bgrect);
//         if (textbox->text) free (textbox->text);
//         if (textbox->pswd) free (textbox->pswd);

//         free (textbox);
//     }

// }

// void ui_textBox_setBorders (TextBox *textbox, u8 borderWidth, u32 borderColor) {

//     if (textbox) {
//         textbox->borderWidth = borderWidth;
//         textbox->borderColor = borderColor;
//     }

// }

// void ui_textBox_update_text (TextBox *textbox, const char *text) {

//     if (textbox) {
//         if (textbox->ispassword) {
//             if (textbox->pswd) strcat (textbox->pswd, text);
//             if (textbox->text) {
//                 u32 len = strlen (textbox->pswd);
//                 for (u8 i = 0; i < len; i++) textbox->text[i] = '*';
//             }
//         }
        
//         else if (textbox->text) strcat (textbox->text, text);
//     }

// }

// void ui_textbox_delete_text (TextBox *textbox) {

//     if (textbox) {
//         u32 len = strlen (textbox->text);
//         if (len > 0) {
//             if (len == 1) {
//                 free (textbox->text);
//                 textbox->text = (char *) calloc (64, sizeof (char));

//                 if (textbox->ispassword) {
//                     free (textbox->pswd);
//                     textbox->pswd = (char *) calloc (64, sizeof (char));
//                 }
//             }

//             else {
//                 char *temp = (char *) calloc (64, sizeof (char));

//                 if (textbox->ispassword) {
//                     for (u8 i = 0; i < len - 1; i++) temp[i] = textbox->pswd[i];
//                     free (textbox->text);
//                     free (textbox->pswd);
//                     textbox->pswd = createString ("%s", temp);
//                     u32 newlen = strlen (textbox->pswd);
//                     textbox->text = (char *) calloc (64, sizeof (char));
//                     for (u8 i = 0; i < newlen; i++) textbox->text[i] = '*';
//                 }

//                 else {
//                     for (u8 i = 0; i < len - 1; i++) temp[i] = textbox->text[i];
//                     free (textbox->text);
//                     textbox->text = createString ("%s", temp);
//                 }

//                 free (temp);
//             }
//         }
//     }

// }

// void ui_textBox_draw (Console *console, TextBox *textbox) {

//     if (console && textbox) {
//         ui_drawRect (console, textbox->bgrect, textbox->bgcolor,
//             textbox->borderWidth, textbox->borderColor);

//         if (textbox->text)
//             putStringAt (console, textbox->text, textbox->bgrect->x + 1, textbox->bgrect->y + 1, 
//                 textbox->textColor, NO_COLOR);
//     }
        
// }

// Button *ui_button_create (u8 x, u8 y, u8 w, u8 h, u32 bgcolor,
//     const char *text, u32 textColor, EventListener event) {

//     Button *new_button = (Button *) malloc (sizeof (Button));
//     if (new_button) {
//         new_button->bgrect = (UIRect *) malloc (sizeof (UIRect));
//         new_button->bgrect->x = x;
//         new_button->bgrect->y = y;
//         new_button->bgrect->w = w;
//         new_button->bgrect->h = h;

//         new_button->bgcolor = bgcolor;
//         new_button->textColor = textColor;

//         new_button->text = (char *) calloc (64, sizeof (char));
//         if (text) strcpy (new_button->text, text);

//         new_button->borderWidth = 0;
//         new_button->borderColor = NO_COLOR;

//         new_button->event = event;
//     }

//     return new_button;

// }

// void ui_button_destroy (Button *button) {

//     if (button) {
//         if (button->bgrect) free (button->bgrect);
//         if (button->text) free (button->text);

//         free (button);
//     }

// }

// void ui_button_setBorders (Button *button, u8 borderWidth, u32 borderColor) {

//     if (button) {
//         button->borderWidth = borderWidth;
//         button->borderColor = borderColor;
//     }

// }

// // TODO: center the text at y pos
// void ui_button_draw (Console *console, Button *button) {

//     if (console && button) {
//         ui_drawRect (console, button->bgrect, button->bgcolor,
//             button->borderWidth, button->borderColor);

//         if (button->text) {
//             u32 stringlen = strlen (button->text);
//             u32 x = (button->bgrect->w / 2) - (stringlen / 2);
//             x += button->bgrect->x;

//             for (u8 i = 0; i < stringlen; i++)
//                 putCharAt (console, (asciiChar) button->text[i], x + i, button->bgrect->y + 1, 
//                     button->textColor, NO_COLOR);
//         }
//     }

// }