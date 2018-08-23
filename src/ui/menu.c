/*** Code for the main menu of the game ***/

// TODO: later we will want to add a menu to create your profile, etc
// TODO: also add a menu to create your character, select races and classes
// TODO: add a settings menu

#include "blackrock.h"

#include "utils/list.h"

#include "ui/ui.h"
#include "ui/console.h"

#include "input.h"

#define MENU_LEFT   50
#define MENU_TOP    28
#define MENU_WIDTH  24
#define MENU_HEIGHT 10  

/*** MENUS ***/

static void renderMainMenu (Console *console) {

    UIRect rect = { 0, 0, MENU_WIDTH, MENU_HEIGHT };
    drawRect (console, &rect, 0x363247FF, 0, 0xFFFFFFFF);

    putStringAt (console, "Start a (N)ew game", 2, 3, 0xBCA285FF, 0X00000000);

}


UIScreen *menuScene (void) {

    List *menuViews = initList (NULL);

    UIRect menuRect = { (16 * MENU_LEFT), (16 * MENU_TOP), (16 * MENU_WIDTH), (16 * MENU_HEIGHT) };
    UIView *menuView = newView (menuRect, MENU_WIDTH, MENU_HEIGHT, tileset, 0, 0x000000FF, true, renderMainMenu);
    insertAfter (menuViews, NULL, menuView);

    // TODO: add more menus here!

    UIScreen *menuScreen = (UIScreen *) malloc (sizeof (UIScreen));
    menuScreen->views = menuViews;
    menuScreen->activeView = menuView;
    menuScreen->handleEvent = hanldeMenuEvent;

    return menuScreen;

}