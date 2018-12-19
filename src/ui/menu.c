#include "blackrock.h"

#include "input.h"
#include "resources.h"

#include "game.h"   // for black credentials

#include "ui/ui.h"
#include "ui/menu.h"
#include "ui/console.h"

#include "utils/dlist.h"
#include "utils/myUtils.h"

MenuView activeMenuView;
UIScreen *menuScreen = NULL;

extern bool typing;
extern TextBox **selected_textBox;

/*** LAUNCH IMAGE ***/

#pragma region LAUNCH IMAGE

#define BG_WIDTH    80
#define BG_HEIGHT   45

char *launchImg = "./resources/blackrock-small.png";  

UIView *launchView = NULL;
BitmapImage *bgImage = NULL;

void renderLaunch (Console *console) {

    if (!bgImage) bgImage = loadImageFromFile (launchImg);

    ui_drawImageAt (console, bgImage, 0, 0);

}

void toggleLaunch (void *args) {

    if (!launchView) {
        if (typing) input_stop_typing ();

        UIRect bgRect = { 0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT) };
        launchView = ui_newView (bgRect, BG_WIDTH, BG_HEIGHT, tileset, 0, BLACK, true, renderLaunch);
        dlist_insert_after (menuScreen->views, LIST_START (menuScreen->views), launchView);

        // menuScreen->activeView = launchView;
        activeMenuView = LAUNCH_VIEW;
    }

    else {
        if (launchView) {
            ListElement *launch = dlist_get_ListElement (activeScene->views, launchView);
            ui_destroyView ((UIView *) dlist_remove_element (activeScene->views, launch));
            launchView = NULL;
        }
    }

}

#pragma endregion

#pragma region LOGIN

UIView *loginView = NULL;

TextBox **loginTextBoxes = NULL;
u8 login_textboxes_idx = 0;

Button *submitButton = NULL;

char *login_error_text = NULL;

// FIXME: add an option to play offline!!

TextBox **initLoginTextBoxes (void) {

    loginTextBoxes = (TextBox **) calloc (4, sizeof (TextBox *));
    if (loginTextBoxes) {
        loginTextBoxes[0] = ui_textBox_create (22, 14, 36, 3, WHITE, NULL, false, BLACK);
        loginTextBoxes[1] = ui_textBox_create (22, 18, 36, 3, WHITE, NULL, true, BLACK);
        loginTextBoxes[2] = ui_textBox_create (22, 29, 36, 3, WHITE, NULL, false, BLACK);
        loginTextBoxes[3] = ui_textBox_create (22, 33, 36, 3, WHITE, NULL, true, BLACK);
    }

    return loginTextBoxes;

}

void renderLogin (Console *console) {

    putStringAtCenter (console, "Welcome to Blackrock!", 3, WHITE, NO_COLOR);

    if (login_error_text) 
        putStringAtCenter (console, login_error_text, 6, FULL_RED, NO_COLOR);

    putStringAtCenter (console, "Login:", 10, WHITE, NO_COLOR);
    putStringAt (console, "Username:", 12, 15, WHITE, NO_COLOR);
    ui_textBox_draw (console, loginTextBoxes[0]);
    putStringAt (console, "Password:", 12, 19, WHITE, NO_COLOR);
    ui_textBox_draw (console, loginTextBoxes[1]);

    putStringAtCenter (console, "Create an account:", 25, WHITE, NO_COLOR);
    putStringAt (console, "Username:", 12, 30, WHITE, NO_COLOR);
    ui_textBox_draw (console, loginTextBoxes[2]);
    putStringAt (console, "Password:", 12, 34, WHITE, NO_COLOR);
    ui_textBox_draw (console, loginTextBoxes[3]);

    ui_button_draw (console, submitButton);

}

void multiplayer_submit_credentials (void *data);

void toggleLogin (void) {

    if (!loginView) {
        loginTextBoxes = initLoginTextBoxes ();
        submitButton = ui_button_create (36, 40, 10, 3, WHITE, "Submit", BLACK, 
            multiplayer_submit_credentials);

        UIRect bgRect = { 0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT) };
        loginView = ui_newView (bgRect, BG_WIDTH, BG_HEIGHT, tileset, 0, 0x4B6584FF, true, renderLogin);
        dlist_insert_after (menuScreen->views, LIST_START (menuScreen->views), loginView);

        activeMenuView = LOGIN_VIEW;

        input_start_typing ();
        selected_textBox = &loginTextBoxes[login_textboxes_idx];
        loginTextBoxes[login_textboxes_idx]->bgcolor = SILVER;
    }

}

BlackCredentials *getBlackCredentials (void) {

    BlackCredentials *black_credentials = (BlackCredentials *) malloc (sizeof (BlackCredentials));
                        
    if (black_credentials) {
        // check if we have sign in credentials
        u32 signin_user_len = strlen (loginTextBoxes[0]->text);
        u32 signin_pswd_len = strlen (loginTextBoxes[1]->pswd);

        if ((signin_user_len > 0) && (signin_pswd_len > 0)) {
            // we have login credentials
            strcpy (black_credentials->username, loginTextBoxes[0]->text);
            strcpy (black_credentials->password, loginTextBoxes[1]->pswd);

            black_credentials->login = true;
        }

        else {
            // check for sign up credentials
            u32 signup_user_len = strlen (loginTextBoxes[2]->text);
            u32 signup_pswd_len = strlen (loginTextBoxes[3]->pswd);
            if ((signup_user_len > 0) && (signup_pswd_len > 0)) {
                // we have new credentials
                strcpy (black_credentials->username, loginTextBoxes[2]->text);
                strcpy (black_credentials->password, loginTextBoxes[3]->pswd);

                black_credentials->login = false;
            }

            else return NULL;   // no credential provided
        }
    }

    return black_credentials;

}

#pragma endregion

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
    UIView *mainMenu = ui_newView (menu, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, tileset, 0, MAIN_MENU_COLOR, true, renderMainMenu);
    dlist_insert_after (menuScreen->views, LIST_END (menuScreen->views), mainMenu);

    // FIXME:
    // menuScreen->activeView = MAIN_MENU_VIEW; 
    activeMenuView = MAIN_MENU_VIEW;

}

#pragma endregion

/*** MULTIPLAYER ***/

#pragma region MULTIPLAYER

#define MULTI_MENU_COLOR        0x34495EFF
#define MULTI_MENU_WIDTH        60
#define MULTI_MENU_HEIGHT       35
#define MULTI_MENU_TOP          5
#define MULTI_MENU_LEFT         10

UIView *multiMenu = NULL;

#define LOBBY_MENU_COLOR        0x4B6584FF
#define LOBBY_MENU_WIDTH        FULL_SCREEN_WIDTH
#define LOBBY_MENU_HEIGHT       FULL_SCREEN_HEIGHT

UIView *lobbyMenu = NULL;

extern Lobby *current_lobby;

static void renderMultiplayerMenu (Console *console) {

    putStringAtCenter (console, "Multiplayer Menu", 3, WHITE, NO_COLOR);

    putStringAt (console, "[c]reate new game", 10, 10, WHITE, NO_COLOR);
    putStringAt (console, "[j]oin game in progress", 10, 12, WHITE, NO_COLOR);

    putReverseString (console, "kca]b[", 55, 32, WHITE, NO_COLOR);

}

void toggleMultiplayerMenu (void) {

    if (multiMenu == NULL) {
        UIRect menu = { (16 * MULTI_MENU_LEFT), (16 * MULTI_MENU_TOP), (16 * MULTI_MENU_WIDTH), (16 * MULTI_MENU_HEIGHT) };
        multiMenu = ui_newView (menu, MULTI_MENU_WIDTH, MULTI_MENU_HEIGHT, tileset, 0, MULTI_MENU_COLOR, true, renderMultiplayerMenu);
        dlist_insert_after (menuScreen->views, LIST_END (menuScreen->views), multiMenu);

        // menuScreen->activeView = MULTI_MENU_VIEW;
        activeMenuView = MULTI_MENU_VIEW;
    }

    else {
        ListElement *multi = dlist_get_ListElement (activeScene->views, multiMenu);
        ui_destroyView ((UIView *) dlist_remove_element (activeScene->views, multi));
        multiMenu = NULL;

        activeMenuView = MAIN_MENU_VIEW;
    }

}

#define PLAYER_RECT_WIDTH       20
#define PLAYER_RECT_HEIGTH      20

#define MAX_PLAYER_RECTS        4

#define PLAYER_SELECTED_COLOR      0x847967FF

typedef struct {

    u8 xIdx, yIdx;
    UIRect *bgRect;
    UIRect *imgRect;
    // FIXME: add player here!

} PlayerRect;

PlayerRect **player_rects = NULL;

u8 playersXIdx = 0;

// TODO: image rects
PlayerRect *createPlayerRect (u8 x, u8 y) {

    PlayerRect *new_rect = (PlayerRect *) malloc (sizeof (PlayerRect));

    if (new_rect) {
        new_rect->bgRect = (UIRect *) malloc (sizeof (UIRect));
        new_rect->bgRect->w = PLAYER_RECT_WIDTH;
        new_rect->bgRect->h = PLAYER_RECT_HEIGTH;

        // new_rect->imgRect = (UIRect *) malloc (sizeof (UIRect));
        // new_rect->imgRect->w = PLAYER_RECT_WIDTH;
        // new_rect->imgRect->h = PLAYER_RECT_HEIGTH;
        new_rect->imgRect = NULL;

        // FIXME:
        new_rect->xIdx = x;
        new_rect->yIdx = y;
        // new->bgRect->x = x + 3 + (INVENTORY_CELL_WIDTH * x);
        // new->bgRect->y = y + 5 + (INVENTORY_CELL_HEIGHT * y);
        // new->imgRect->x = x + 3 + (INVENTORY_CELL_WIDTH * x);
        // new->imgRect->y = y + 5 + (INVENTORY_CELL_HEIGHT * y);
    }

    return new_rect;

}

PlayerRect **initPlayerRects (void) {

    PlayerRect **playerRects = (PlayerRect **) calloc (MAX_PLAYER_RECTS, sizeof (PlayerRect *));

    for (u8 i = 0; i < MAX_PLAYER_RECTS; i++) playerRects[i] = createPlayerRect (i, 0);

    return playerRects;

}

// TODO:
// update the lobby player rects based on the new lobby data
void updatePlayerRects (void) {}

void destroyPlayerRects (void) {

    if (player_rects) {
        for (u8 i = 0; i < MAX_PLAYER_RECTS; i++) {
            if (player_rects[i]) {
                if (player_rects[i]->bgRect) free (player_rects[i]->bgRect);
                // if (player_rects[i]->imgRect) free (playerRects[i]->imgRect);
                // TODO: player
                free (player_rects[i]);
            }
        }

        free (player_rects);
    }

}

void renderPlayerRects (Console *console) {

    PlayerRect *player_rect = NULL;

    for (u8 i = 0; i < MAX_PLAYER_RECTS; i++) {
        player_rect = player_rects[i];

        // draw highlighted rect
        if (playersXIdx == player_rect->xIdx) {
            ui_drawRect (console, player_rect->bgRect, PLAYER_SELECTED_COLOR, 0, NO_COLOR);
            // FIXME: player
            // if () {

            // }
        }

        // draw every other rect with a player in it

        // draw empty rects
    }

}

// TODO: draw here the item image
// void renderInventoryItems (Console *console) {

//              // draw highlighted rect
//             // if (inventoryXIdx == invRect->xIdx && inventoryYIdx == invRect->yIdx) {
//             //     ui_drawRect (console, invRect->bgRect, INVENTORY_SELECTED, 0, NO_COLOR);
//             //     // drawRect (console, invRect->imgRect, INVENTORY_SELECTED, 0, 0x00000000);
//             //     if (invRect->item != NULL) {
//             //         // drawImageAt (console, apple, invRect->imgRect->x, invRect->imgRect->y);
//             //         Graphics *g = (Graphics *) getGameComponent (invRect->item, GRAPHICS);
//             //         if (g != NULL) 
//             //             putStringAt (console, g->name, 5, 22, getItemColor (invRect->item->rarity), NO_COLOR);

//             //         u8 quantity = ZERO_ITEMS + invRect->item->quantity;
//             //         // putCharAt (console, quantity, invRect->imgRect->x, invRect->imgRect->y, 0xFFFFFFFF, 0x00000000);
//             //         putCharAt (console, quantity, invRect->bgRect->x, invRect->bgRect->y, WHITE, NO_COLOR);
//             //     }
//             // }

//             // // draw every other rect with an item on it
//             // else if (invRect->item != NULL) {
//             //     ui_drawRect (console, invRect->bgRect, INVENTORY_CELL_COLOR, 0, NO_COLOR);

//             //     u8 quantity = ZERO_ITEMS + invRect->item->quantity;
//             //     putCharAt (console, quantity, invRect->bgRect->x, invRect->bgRect->y, WHITE, NO_COLOR);
//             // }

//             // // draw the empty rects
//             // else ui_drawRect (console, invRect->bgRect, INVENTORY_CELL_COLOR, 0, NO_COLOR);


// } 

static void renderLobbyMenu (Console *console) {

    putStringAtCenter (console, "Lobby Menu", 3, WHITE, NO_COLOR);

    if (current_lobby) {
        char *str = createString ("Max players: %i", current_lobby->settings.maxPlayers);
        putStringAtCenter (console, str, 10, WHITE, NO_COLOR);
    }

}

void toggleLobbyMenu (void) {

    if (!lobbyMenu) {
        UIRect lobby_menu = { 0, 0, (16 * LOBBY_MENU_WIDTH), (16 * LOBBY_MENU_HEIGHT) };
        lobbyMenu = ui_newView (lobby_menu, LOBBY_MENU_WIDTH, LOBBY_MENU_HEIGHT, tileset, 0, LOBBY_MENU_COLOR, true, renderLobbyMenu);
        dlist_insert_after (menuScreen->views, LIST_END (menuScreen->views), lobbyMenu);

        // menuScreen->activeView = MULTI_MENU_VIEW;
        activeMenuView = LOBBY_MENU_VIEW;

        if (!player_rects) {
            player_rects = initPlayerRects ();
            updatePlayerRects ();
        }

        else updatePlayerRects ();
    }

}

#pragma endregion

/*** CHARACTER MENU ***/

#pragma region CHARACTER

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
    ui_drawRect (console, &rect, CHAR_CREATION_COLOR, 0, 0xFFFFFFFF);

    putStringAtCenter (console, "Character Creation", 2, CHAR_CREATION_TEXT, 0x00000000);

}


// FIXME:
void toggleCharacterMenu (void) {

    if (characterMenu == NULL) {
        toggleLaunch (NULL);

        UIRect charMenu = { (16 * CHAR_CREATION_LEFT), (16 * CHAR_CREATION_TOP), (16 * CHAR_CREATION_WIDTH), (16 * CHAR_CREATION_HEIGHT) };
        characterMenu = ui_newView (charMenu, CHAR_CREATION_WIDTH, CHAR_CREATION_HEIGHT, tileset, 0, NO_COLOR, true, renderCharacterMenu);
        dlist_insert_after (activeScene->views, LIST_END (activeScene->views), characterMenu);

        menuScreen->activeView = characterMenu;
    }

    else {
        if (characterMenu != NULL) {
            toggleLaunch (NULL);

            ListElement *charMenu = dlist_get_ListElement (activeScene->views, characterMenu);
            ui_destroyView ((UIView *) dlist_remove_element (activeScene->views, charMenu));
            characterMenu = NULL;
        }
    }

}

#pragma endregion

/*** MENU SCENE ***/

#pragma region MENU

void destroyMenuScene (void) {

    if (menuScreen) {
        if (login_error_text) free (login_error_text);

        if (loginTextBoxes) 
            for (u8 i = 0; i < 4; i++)
                ui_textBox_destroy (loginTextBoxes[i]);

        ui_button_destroy (submitButton);

        destroyImage (bgImage);

        destroyPlayerRects ();

        dlist_destroy (menuScreen->views);
        free (menuScreen);

        fprintf (stdout, "Done cleaning up menu.\n");
    }

}

UIScreen *menuScene (void) {

    menuScreen = (UIScreen *) malloc (sizeof (UIScreen));
    menuScreen->views = dlist_init (ui_destroyView);
    menuScreen->handleEvent = hanldeMenuEvent;

    // toggleLaunch ();
    toggleLogin ();

    destroyCurrentScreen = destroyMenuScene;

    return menuScreen;

}

#pragma endregion