#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "ui/ui.h"

typedef enum MenuView {

    LAUNCH_VIEW = 0,
    MAIN_MENU_VIEW,
    MULTI_MENU_VIEW,

} MenuView;

extern MenuView activeMenuView;

extern UIScreen *menuScene (void);
extern void destroyMenuScene (void);

/*** MAIN MENU ***/

extern void createMainMenu (void);

/*** MULTIPLAYER ***/

extern void toggleMultiplayerMenu (void);

#endif