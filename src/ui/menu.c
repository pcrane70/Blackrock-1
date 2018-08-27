/*** Code for the main menu of the game ***/

// TODO: later we will want to add a menu to create your profile, etc
// TODO: also add a menu to create your character, select races and classes
// TODO: add a settings menu

#include "blackrock.h"

#include "resources.h"

#include "utils/list.h"

#include "ui/ui.h"
#include "ui/console.h"

#include "input.h"

#define BG_WIDTH    80
#define BG_HEIGHT   45

#define MENU_LEFT   50
#define MENU_TOP    28
#define MENU_WIDTH  24
#define MENU_HEIGHT 10 

char *launchImg = "./resources/blackrock-small.png";  

/*** LAUNCH IMAGE ***/

static void renderBg (Console *console) {

    static BitmapImage *bgImage = NULL;

    if (bgImage == NULL) 
        bgImage = loadImageFromFile (launchImg);

    drawImageAt (console, bgImage, 0, 0);

}

/*** MENUS ***/

// Old menu
static void renderMainMenu (Console *console) {

    UIRect rect = { 0, 0, MENU_WIDTH, MENU_HEIGHT };
    drawRect (console, &rect, 0x363247FF, 0, 0xFFFFFFFF);

    putStringAt (console, "Start a (N)ew game", 2, 3, 0xBCA285FF, 0X00000000);

}

UIScreen *menuScene (void) {

    List *menuViews = initList (NULL);

    // UIRect menuRect = { (16 * MENU_LEFT), (16 * MENU_TOP), (16 * MENU_WIDTH), (16 * MENU_HEIGHT) };
    // UIView *menuView = newView (menuRect, MENU_WIDTH, MENU_HEIGHT, tileset, 0, 0x000000FF, true, renderMainMenu);
    // insertAfter (menuViews, NULL, menuView);

    UIRect bgRect = { 0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT) };
    UIView *bgView = newView (bgRect, BG_WIDTH, BG_HEIGHT, tileset, 0, 0x000000FF, true, renderBg);
    insertAfter (menuViews, NULL, bgView);

    // TODO: add more menus here!

    UIScreen *menuScreen = (UIScreen *) malloc (sizeof (UIScreen));
    menuScreen->views = menuViews;
    menuScreen->activeView = bgView;
    menuScreen->handleEvent = hanldeMenuEvent;

    return menuScreen;

}

