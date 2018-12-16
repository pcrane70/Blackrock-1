#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "ui/ui.h"

typedef enum MenuView {

    LOGIN_VIEW,
    LAUNCH_VIEW,
    MAIN_MENU_VIEW,
    MULTI_MENU_VIEW,
    LOBBY_MENU_VIEW

} MenuView;

extern MenuView activeMenuView;

extern UIScreen *menuScene (void);
extern void destroyMenuScene (void);

/*** MAIN MENU ***/

extern void createMainMenu (void);

/*** MULTIPLAYER ***/

extern void toggleMultiplayerMenu (void);

extern void toggleLobbyMenu (void);

#endif