#include "blackrock.h"

#include "input.h"
#include "resources.h"

#include "ui/ui.h"
#include "ui/menu.h"
#include "ui/console.h"

#include "utils/list.h"

MenuView activeMenuView;
UIScreen *menuScreen = NULL;

/*** LAUNCH IMAGE ***/

#pragma region LAUNCH IMAGE

#define BG_WIDTH    80
#define BG_HEIGHT   45

char *launchImg = "./resources/blackrock-small.png";  

UIView *launchView = NULL;
BitmapImage *bgImage = NULL;

static void renderLaunch (Console *console) {

    if (!bgImage) bgImage = loadImageFromFile (launchImg);

    drawImageAt (console, bgImage, 0, 0);

}

// FIXME: where are we destroying the image?
void toggleLaunch (void) {

    if (launchView == NULL) {
        UIRect bgRect = { 0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT) };
        launchView = newView (bgRect, BG_WIDTH, BG_HEIGHT, tileset, 0, NO_COLOR, true, renderLaunch);
        insertAfter (menuScreen->views, NULL, launchView);

        // menuScreen->activeView = launchView;
        activeMenuView = LAUNCH_VIEW;
    }

    else {
        if (launchView != NULL) {
            // if (bgImage != NULL) destroyImage (bgImage);

            ListElement *launch = getListElement (activeScene->views, launchView);
            destroyView ((UIView *) removeElement (activeScene->views, launch));
            launchView = NULL;
        }
    }

}

#pragma endregion

#pragma region MENUS

/*** MAIN MENU ***/

#define MAIN_MENU_COLOR         0x4B6584FF

static void renderMainMenu (Console *console) {

    putStringAtCenter (console, "Main Menu", 3, WHITE, NO_COLOR);

    putStringAt (console, "[s]olo play", 10, 10, WHITE, NO_COLOR);
    putStringAt (console, "[m]ultiplayer", 10, 12, WHITE, NO_COLOR);

}

// FIXME:
void createMainMenu (void) {

    UIRect menu = { (16 * FULL_SCREEN_LEFT), (16 * FULL_SCREEN_TOP), (16 * FULL_SCREEN_WIDTH), (16 * FULL_SCREEN_HEIGHT) };
    UIView *mainMenu = newView (menu, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, tileset, 0, MAIN_MENU_COLOR, true, renderMainMenu);
    insertAfter (menuScreen->views, LIST_END (menuScreen->views), mainMenu);

    // FIXME:
    // menuScreen->activeView = MAIN_MENU_VIEW; 
    activeMenuView = MAIN_MENU_VIEW;

}

/*** MULTIPLAYER ***/

#define MULTI_MENU_COLOR        0x34495EFF
#define MULTI_MENU_WIDTH        60
#define MULTI_MENU_HEIGHT       35
#define MULTI_MENU_TOP          5
#define MULTI_MENU_LEFT         10

UIView *multiMenu = NULL;

static void renderMultiplayerMenu (Console *console) {

    putStringAtCenter (console, "Multiplayer Menu", 3, WHITE, NO_COLOR);

    putStringAt (console, "[c]reate new game", 10, 10, WHITE, NO_COLOR);
    putStringAt (console, "[j]oin game in progress", 10, 12, WHITE, NO_COLOR);

    putReverseString (console, "kca]b[", 55, 32, WHITE, NO_COLOR);

}

void toggleMultiplayerMenu (void) {

    if (multiMenu == NULL) {
        UIRect menu = { (16 * MULTI_MENU_LEFT), (16 * MULTI_MENU_TOP), (16 * MULTI_MENU_WIDTH), (16 * MULTI_MENU_HEIGHT) };
        multiMenu = newView (menu, MULTI_MENU_WIDTH, MULTI_MENU_HEIGHT, tileset, 0, MULTI_MENU_COLOR, true, renderMultiplayerMenu);
        insertAfter (menuScreen->views, LIST_END (menuScreen->views), multiMenu);

        // menuScreen->activeView = MULTI_MENU_VIEW;
        activeMenuView = MULTI_MENU_VIEW;
    }

    else {
        ListElement *multi = getListElement (activeScene->views, multiMenu);
        destroyView ((UIView *) removeElement (activeScene->views, multi));
        multiMenu = NULL;

        activeMenuView = MAIN_MENU_VIEW;
    }

}

/*** CHARACTER MENU ***/

#define CHAR_CREATION_LEFT		0
#define CHAR_CREATION_TOP		0
#define CHAR_CREATION_WIDTH		80
#define CHAR_CREATION_HEIGHT	45

#define CHAR_CREATION_COLOR     0x4B6584FF

#define CHAR_CREATION_TEXT      0xEEEEEEFF

UIView *characterMenu = NULL;

// FIXME:
// 09/09/2018 -- this is only temporary, later we will want to have a better character and profile menu
static void renderCharacterMenu (Console *console) {

    UIRect rect = { 0, 0, CHAR_CREATION_WIDTH, CHAR_CREATION_HEIGHT };
    drawRect (console, &rect, CHAR_CREATION_COLOR, 0, 0xFFFFFFFF);

    putStringAtCenter (console, "Character Creation", 2, CHAR_CREATION_TEXT, 0x00000000);

}

void toggleCharacterMenu (void) {

    if (characterMenu == NULL) {
        toggleLaunch ();

        UIRect charMenu = { (16 * CHAR_CREATION_LEFT), (16 * CHAR_CREATION_TOP), (16 * CHAR_CREATION_WIDTH), (16 * CHAR_CREATION_HEIGHT) };
        characterMenu = newView (charMenu, CHAR_CREATION_WIDTH, CHAR_CREATION_HEIGHT, tileset, 0, 0x000000FF, true, renderCharacterMenu);
        insertAfter(activeScene->views, LIST_END (activeScene->views), characterMenu);

        menuScreen->activeView = characterMenu;
    }

    else {
        if (characterMenu != NULL) {
            toggleLaunch ();

            ListElement *charMenu = getListElement (activeScene->views, characterMenu);
            destroyView ((UIView *) removeElement (activeScene->views, charMenu));
            characterMenu = NULL;
        }
    }

}

#pragma endregion


/*** MENU SCENE ***/

#pragma region MENU SCENE

void destroyMenuScene (void) {

    if (menuScreen) {
        destroyImage (bgImage);

        UIView *view = NULL;
        while (LIST_SIZE (menuScreen->views) > 0) {
            view = (UIView *) removeElement (menuScreen->views, LIST_END (menuScreen->views));
            if (view) destroyView (view);
        }

        destroyList (menuScreen->views);
        free (menuScreen);

        fprintf (stdout, "Done cleaning up menu.\n");
    }

}

// FIXME: destroy list function
UIScreen *menuScene (void) {

    menuScreen = (UIScreen *) malloc (sizeof (UIScreen));
    menuScreen->views = initList (free);
    menuScreen->handleEvent = hanldeMenuEvent;

    toggleLaunch ();

    destroyCurrentScreen = destroyMenuScene;

    return menuScreen;

}

#pragma endregion