#include "blackrock.h"

#ifdef LINUX
#include <pthread.h>
#endif

#include <assert.h>

#include "cengine/renderer.h"
#include "cengine/textures.h"
#include "cengine/animation.h"

#include "game/game.h"
#include "game/camera.h"

#include "game/world.h"
#include "game/entities/player.h"
#include "game/entities/enemy.h"
#include "game/item.h"
#include "game/map/map.h"

#include "ui/gameUI.h"  // for the message log

#include "utils/objectPool.h"

#include "collections/dlist.h"

#include "utils/myUtils.h"
#include "utils/config.h"     // for getting the data

#include "cerver/client.h"
#include "utils/log.h"

#pragma region COMPONENTS 

static Transform *transform_new (u32 objectID) {

    Transform *new_transform = (Transform *) malloc (sizeof (Transform));
    if (new_transform) {
        new_transform->goID = objectID;
        new_transform->position.x = 0;
        new_transform->position.y = 0;
    }

    return new_transform;

}

static void transform_destroy (Transform *transform) { if (transform) free (transform); }

static Graphics *graphics_new (u32 objectID) {

    Graphics *new_graphics = (Graphics *) malloc (sizeof (Graphics));
    if (new_graphics) {
        new_graphics->goID = objectID;
        new_graphics->sprite = NULL;
        new_graphics->spriteSheet = NULL;
        new_graphics->refSprite = false;
        new_graphics->multipleSprites = false;
        new_graphics->x_sprite_offset = 0;
        new_graphics->y_sprite_offset = 0;
        new_graphics->layer = UNSET_LAYER;
        new_graphics->flip = NO_FLIP;
    }

    return new_graphics;

}

static void graphics_destroy (Graphics *graphics) {

    if (graphics) {
        if (graphics->refSprite) {
            graphics->sprite = NULL;
            graphics->spriteSheet = NULL;
        }

        else {
            if (graphics->sprite) sprite_destroy (graphics->sprite);
            if (graphics->spriteSheet) sprite_sheet_destroy (graphics->spriteSheet);
        }

        free (graphics);
    }

}

void graphics_set_sprite (Graphics *graphics, const char *filename) {

    if (graphics && filename) {
        graphics->sprite = sprite_load (filename, main_renderer);
        graphics->spriteSheet = NULL;
        graphics->multipleSprites = false;
    }

}

void graphics_set_sprite_sheet (Graphics *graphics, const char *filename) {

    if (graphics && filename) {
        graphics->sprite = NULL;
        graphics->spriteSheet = sprite_sheet_load (filename, main_renderer);
        graphics->multipleSprites = true;
    }

}

void graphics_ref_sprite (Graphics *graphics, Sprite *sprite) {

    if (graphics && sprite) {
        graphics->sprite = sprite;
        graphics->spriteSheet = NULL;
        graphics->multipleSprites = false;
        graphics->refSprite = true;
    }

}

void graphics_ref_sprite_sheet (Graphics *graphics, SpriteSheet *spriteSheet) {

    if (graphics && spriteSheet) {
        graphics->sprite = NULL;
        graphics->spriteSheet = spriteSheet;
        graphics->multipleSprites = true;
        graphics->refSprite = true;
    }

}

// FIXME: Add colliders logic!

/* static BoxCollider *collider_box_new (u32 objectID) {

    BoxCollider *new_collider = (BoxCollider *) malloc (sizeof (BoxCollider));
    if (new_collider) {
        new_collider->x = new_collider->y = 0;
        new_collider->w = new_collider->h = 0;
    }

    return new_collider;

}

void collider_box_init (u32 x, u32 y, u32 w, u32 h) {}

bool collider_box_collision (const BoxCollider *a, const BoxCollider *b) {

    if (a && b) 
        if (a->x + a->w >= b->x &&
            b->x + b->w >= a->x &&
            a->y + a->h >= b->y &&
            b->y + b->h >= a->y)
                return true;

    return false;

}

static void collider_box_destroy (BoxCollider *box) { if (box) free (box); } */

#pragma endregion

#pragma region GAME OBJECTS

static GameObject **gameObjects;
static u32 max_gos;
static u32 curr_max_objs;
static u32 new_go_id;

static bool game_objects_realloc (void) {

    u32 new_max_gos = curr_max_objs * 2;

    gameObjects = realloc (gameObjects, new_max_gos * sizeof (GameObject *));

    if (gameObjects) {
        max_gos = new_max_gos;
        return true;
    }

    return false;

}

// init our game objects array
static u8 game_objects_init_all (void) {

    gameObjects = (GameObject **) calloc (DEFAULT_MAX_GOS, sizeof (GameObject *));
    if (gameObjects) {
        for (u32 i = 0; i < DEFAULT_MAX_GOS; i++) gameObjects[i] = NULL;

        max_gos = DEFAULT_MAX_GOS;
        curr_max_objs = 0;
        new_go_id = 0;

        return 0;
    }

    return 1;

}

static i32 game_object_get_free_spot (void) {

    for (u32 i = 0; i < curr_max_objs; i++)
        if (gameObjects[i]->id == -1)
            return i;

    return -1;

}

static void game_object_init (GameObject *go, u32 id, const char *name, const char *tag) {

    if (go) {
        go->id = id;

        if (name) {
            go->name = (char *) calloc (strlen (name) + 1, sizeof (char));
            strcpy (go->name, name);
        }

        else go->name = NULL;

        if (tag) {
            go->tag = (char *) calloc (strlen (tag) + 1, sizeof (char));
            strcpy (go->tag, tag);
        }

        else go->tag = NULL;

        for (u8 i = 0; i < COMP_COUNT; i++) go->components[i] = NULL;

        go->children = NULL;

        go->update = NULL;
    }

}

// game object constructor
GameObject *game_object_new (const char *name, const char *tag) {

    GameObject *new_go = NULL;

    // first check if we have a reusable go in the array
    i32 spot = game_object_get_free_spot ();

    if (spot >= 0) {
        new_go = gameObjects[spot];
        game_object_init (new_go, spot, name, tag);
    } 

    else {
        if (new_go_id >= max_gos) game_objects_realloc ();

        new_go = (GameObject *) malloc (sizeof (GameObject));
        if (new_go) {
            game_object_init (new_go, new_go_id, name, tag);
            gameObjects[new_go->id] = new_go;
            new_go_id++;
            curr_max_objs++;
        }
    } 

    return new_go;

}

// this is used to avoid go destruction when destroying go's children
static void game_object_destroy_dummy (void *ptr) {}

static int game_object_comparator (void *one, void *two) {

    if (one && two) {
        GameObject *go_one = (GameObject *) one;
        GameObject *go_two = (GameObject *) two;

        if (go_one->id < go_two->id) return -1;
        else if (go_one->id == go_two->id) return 0;
        else return 1;
    }

}

void game_object_add_child (GameObject *parent, GameObject *child) {

    if (parent && child) {
        if (!parent->children) parent->children = dlist_init (game_object_destroy_dummy, game_object_comparator);
        dlist_insert_after (parent->children, dlist_end (parent->children), child);
    }

}

GameObject *game_object_remove_child (GameObject *parent, GameObject *child) {

    if (parent && child) {
        if (parent->children) { 
            GameObject *go = NULL;
            for (ListElement *le = dlist_start (parent->children); le; le = le->next) {
                go = (GameObject *) le->data;
                if (go->id == child->id) 
                    return (GameObject *) dlist_remove_element (parent->children, le);
            }
        }
    }

    return NULL;

}

GameObject *game_object_get_by_id (u32 id) { if (id <= curr_max_objs) return gameObjects[id]; }

// mark as inactive or reusable the game object
void game_object_destroy (GameObject *go) {

    if (go) {
        go->id = -1;
        go->update = NULL;

        if (go->name) free (go->name);
        if (go->tag) free (go->tag);

        if (go->children) free (go->children);

        // individually destroy each component
        transform_destroy ((Transform *) go->components[TRANSFORM_COMP]);
        graphics_destroy ((Graphics *) go->components[GRAPHICS_COMP]);
        animator_destroy ((Animator *) go->components[ANIMATOR_COMP]);

        player_comp_delete ((Player *) go->components[PLAYER_COMP]);
    }

}

void game_object_destroy_ref (void *data) { game_object_destroy ((GameObject *) data); } 

static void game_object_delete (GameObject *go) {

    if (go) {
        go->update = NULL;

        // individually destroy each component
        transform_destroy ((Transform *) go->components[TRANSFORM_COMP]);
        graphics_destroy ((Graphics *) go->components[GRAPHICS_COMP]);
        animator_destroy ((Animator *) go->components[ANIMATOR_COMP]);

        player_comp_delete ((Player *) go->components[PLAYER_COMP]);

        if (go->name) free (go->name);
        if (go->tag) free (go->tag);

        free (go);
    }

}

void *game_object_add_component (GameObject *go, GameComponent component) {

    void *retval = NULL;

    if (go) {
        switch (component) {
            case TRANSFORM_COMP: 
                retval = go->components[component] = transform_new (go->id); 
                break;
            case GRAPHICS_COMP: 
                retval = go->components[component] = graphics_new (go->id); 
                break;
            case ANIMATOR_COMP: 
                retval = go->components[component] = animator_new (go->id); 
                break;

            case PLAYER_COMP: 
                retval = go->components[component] = player_comp_new (go->id); 
                go->update = player_update;
                break;
            case ENEMY_COMP:
                retval = go->components[component] = enemy_create_comp (go->id);
                go->update = enemy_update;
                break;

            default: break;
        }
    }

    return retval;

}

void *game_object_get_component (GameObject *go, GameComponent component) {

    if (go) return go->components[component];

}

void game_object_remove_component (GameObject *go, GameComponent component) {

    if (go) {
        switch (component) {
            case TRANSFORM_COMP: 
                transform_destroy (go->components[component]); 
                break;
            case GRAPHICS_COMP: 
                graphics_destroy (go->components[component]);
                break;
            case ANIMATOR_COMP: 
                animator_destroy (go->components[component]);
                break;

            case PLAYER_COMP: 
                player_comp_delete (go->components[component]);
                go->update = NULL;
                break;
            case ENEMY_COMP:
                enemy_destroy_comp (go->components[component]);
                go->update = NULL;
                break;

            default: break;
        }
    }

}

#pragma endregion

/*** GAME ***/

#pragma region GAME 

// FIXME: how do we want to manage the score?
// Score
Score *playerScore = NULL;

// TODO:
static u8 load_game_data (void) {

    // connect to items db
    // items_init ();
    // connect to enemies db and load enemy data
    if (!enemies_connect_db ()) 
        return 0;

    return 0;

}

// FIXME:
static void game_update (void);

// TODO: this inits the game to the tavern/village
// TODO: this can be a good place to check if we have a save file of a map and load that from disk

// FIXME: move this from here
static u8 game_init (void) {

    game_objects_init_all ();

    if (!load_game_data ()) {
        // init world
        world = world_create ();

        // init map
        // FIXME: from where do we get this values?
        // single player -> random values
        // multiplayer -> we get it from the server
        world->game_map = map_create (100, 40);
        world->game_map->dungeon = dungeon_generate (world->game_map, 
            world->game_map->width, world->game_map->heigth, 100, .45);

        // spawn enemies
        enemies_spawn_all (world, random_int_in_range (5, 10));

        // spawn items

        // init player(s)
        dlist_insert_after (world->players, dlist_start (world->players), player_init ());

        // spawn players
        GameObject *go = NULL;
        Transform *transform = NULL;
        for (ListElement *le = dlist_start (world->players); le != NULL; le = le->next) {
            go = (GameObject *) le->data;
            transform = (Transform *) game_object_get_component (go, TRANSFORM_COMP);
            Coord spawnPoint = map_get_free_spot (world->game_map);
            // FIXME: fix wolrd scale!!!
            transform->position.x = spawnPoint.x * 64;
            transform->position.y = spawnPoint.y * 64;
        }

        // update camera
        GameObject *main_player = (GameObject *) (dlist_start (world->players)->data );
        transform = (Transform *) game_object_get_component (main_player, TRANSFORM_COMP);
        world->game_camera->center = transform->position;

        // init score

        // FIXME: we are ready to start updating the game
        game_state->update = game_update;
        // game_manager->currState->update = game_update;

        return 0;
    } 

    return 1;

}

// destroy all game data
static u8 game_end (void) {

    // FIXME: destroy world 
    enemy_data_delete_all ();

    enemies_disconnect_db ();

    #ifdef DEV
    logMsg (stdout, DEBUG_MSG, GAME, "Game end!");
    #endif

}

#pragma endregion

// TODO: this shouuld go at the bottom of the file
/*** GAME STATE ***/

#pragma region GAME STATE

GameState *game_state = NULL;

static void game_onEnter (void) { game_init (); }

static void game_onExit (void) { game_end (); }

static void game_update (void) {

    // update every game object
    for (u32 i = 0; i < curr_max_objs; i++) {
        if (gameObjects[i]->id != -1) {
            if (gameObjects[i]->update)
                gameObjects[i]->update (gameObjects[i]);
        }
    }

    // update the camera
    camera_update (world->game_camera);
    
}

// FIXME: we need to implement occlusion culling!
static void game_render (void) {

    Transform *transform = NULL;
    Graphics *graphics = NULL;
    for (u32 i = 0; i < curr_max_objs; i++) {
        transform = (Transform *) game_object_get_component (gameObjects[i], TRANSFORM_COMP);
        graphics = (Graphics *) game_object_get_component (gameObjects[i], GRAPHICS_COMP);
        if (transform && graphics) {
            if (graphics->multipleSprites)
                texture_draw_frame (world->game_camera, graphics->spriteSheet, 
                transform->position.x, transform->position.y, 
                graphics->x_sprite_offset, graphics->y_sprite_offset,
                graphics->flip);
            
            else
                texture_draw (world->game_camera, graphics->sprite, 
                transform->position.x, transform->position.y, 
                graphics->flip);
        }
    }

}

void game_clean_up (void) {

    // clean up game objects
    for (u32 i = 0; i < curr_max_objs; i++) 
        if (gameObjects[i])
            game_object_delete (gameObjects[i]);

    free (gameObjects);

    #ifdef DEV
    logMsg (stdout, SUCCESS, GAME, "Done cleaning up game data!");
    #endif
    
}

GameState *game_state_new (void) {

    GameState *new_game_state = (GameState *) malloc (sizeof (GameState));
    if (new_game_state) {
        new_game_state->state = IN_GAME;

        // new_game_state->update = game_update;
        new_game_state->update = NULL;
        new_game_state->render = game_render;

        new_game_state->onEnter = game_onEnter;
        new_game_state->onExit = game_onExit;
    }

}

#pragma endregion

/*** GAME MANAGER ***/

#pragma region GAME MANAGER

GameManager *game_manager = NULL;

GameState *menu_state = NULL;
GameState *game_over_state =  NULL;

GameManager *game_manager_new (GameState *initState) {

    GameManager *new_game_manager = (GameManager *) malloc (sizeof (GameManager));
    if (new_game_manager) {
        new_game_manager->currState = initState;
        if (new_game_manager->currState->onEnter)
            new_game_manager->currState->onEnter ();
    } 

    return new_game_manager;

}

void game_manager_delete (GameManager *manager) {

    if (manager) {
        if (manager->currState) free (manager->currState);
        free (manager);
    }

}

State game_state_get_current (void) { return game_manager->currState->state; }

void game_state_change_state (GameState *newState) { 
    
    if (game_manager->currState->onExit)
        game_manager->currState->onExit ();

    game_manager->currState = newState; 
    if (game_manager->currState->onEnter)
        game_manager->currState->onEnter ();
    
}

#pragma endregion


/*** WORLD STATE ***/

// FOV
// u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
bool recalculateFov = false;
// extern void calculateFov (u32 xPos, u32 yPos, u32 [MAP_WIDTH][MAP_HEIGHT]);

/*** INITIALIZATION **/

#pragma region INIT GAME


void initWorld (void) {

    /* pthread_t playerThread;

    if (pthread_create (&playerThread, NULL, playerLogic, NULL) != THREAD_OK) 
        die ("Error creating player thread!\n");

    // score struct, probably we will need a more complex system later
    playerScore = (Score *) malloc (sizeof (Score));
    void resetScore (void);
    resetScore ();

    // TODO: we will want to load the in game menu (tavern)

    // 20/08/2018 -- 22:51 -- as we don't have the tavern an the main menu ready, we will go staright into a new
    // game inside the dungeon
    void enterDungeon (void);
    enterDungeon ();

    if (pthread_join (playerThread, NULL) != THREAD_OK) die ("Error joinning player thread!\n");

    // we can now place the player and we are done 
    Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    main_player->pos->x = (u8) playerSpawnPos.x;
    main_player->pos->y = (u8) playerSpawnPos.y;

    calculateFov (main_player->pos->x, main_player->pos->y, fovMap); */
     
}

#pragma endregion

/*** Game Object Management **/

#pragma region GAME OBJECTS

// TESTING 19/08/2018 -- 19:04
// FIXME: HOW CAN WE MANAGE THE WALLS!!!!???
DoubleList *getObjectsAtPos (u32 x, u32 y) {

    /* Position *pos = NULL;
    DoubleList *retVal = dlist_init (free);
    for (ListElement *e = dlist_start (gameObjects); e != NULL; e = e->next) {
        pos = (Position *) getComponent ((GameObject *) e->data, POSITION);
        if (pos != NULL)
            if (pos->x == x && pos->y == y) dlist_insert_after (retVal, NULL, e->data);
    }

    return retVal; */

}

#pragma endregion

/*** MOVEMENT ***/

#pragma region MOVEMENT

// bool isWall (u32 x, u32 y) { return (currentLevel->mapCells[x][y]); }

bool canMove (Position pos, bool isPlayer) {

    /* bool move = true;

    // first check the if we are inside the map bounds
    if ((pos.x >= 0) && (pos.x < MAP_WIDTH) && (pos.y >= 0) && (pos.y < MAP_HEIGHT)) {
        // check for level elements (like walls)
        if (isWall (pos.x, pos.y)) move = false;

        // check for any other entity, like monsters
        GameObject *go = NULL;
        Position *p = NULL;
        for (ListElement *e = dlist_start (gameObjects); e != NULL; e = e->next) {
            go = (GameObject *) e->data;
            p = (Position *) getComponent (go, POSITION);
            if (p->x == pos.x && p->y == pos.y) {
                if (((Physics *) getComponent (go, PHYSICS))->blocksMovement) move = false;
                break;
            }
        }

        // check for player
        if (!isPlayer) {
            if (pos.x == main_player->pos->x && pos.y == main_player->pos->y)
                move = false;

        }
    }   

    else move = false;

    return move; */

}

#define UNSET   999

static i32 **dmap = NULL;

i32 **initTargetMap (void) {

    /* i32 **tmap = NULL;

    tmap = (i32 **) calloc (MAP_WIDTH, sizeof (i32 *));
    for (u8 i = 0; i < MAP_WIDTH; i++)
        tmap[i] = (i32 *) calloc (MAP_HEIGHT, sizeof (i32));

    return tmap; */ 

}

void cleanTargetMap (void) {

    /* if (dmap != NULL) {
        for (u8 i = 0; i < MAP_WIDTH; i++)
            free (dmap[i]);

        free (dmap);
    }  */

}

// 13/08/2018 -- simple representation of a Dijkstra's map, maybe later we will want a more complex map 
// or implement a more advance system
// void generateTargetMap (i32 targetX, i32 targetY) {

//     if (dmap == NULL) dmap = initTargetMap ();

//     // reset the target map
//     for (u8 x = 0; x < MAP_WIDTH; x++)
//         for (u8 y = 0; y < MAP_HEIGHT; y++)
//             dmap[x][y] = UNSET;

//     dmap[targetX][targetY] = 0;

//     bool changesMade = true;
//     while (changesMade) {
//         changesMade = false;

//         for (u8 x = 0; x < MAP_WIDTH; x++) {
//             for (u8 y = 0; y < MAP_HEIGHT; y++) {
//                 i32 currCellValue = dmap[x][y];
//                 // check cells around and update them if necessary
//                 if (currCellValue != UNSET) {
//                     if ((!isWall (x + 1, y)) && (dmap[x + 1][y] > currCellValue + 1)) {
//                         dmap[x + 1][y] = currCellValue + 1;
//                         changesMade = true;
//                     }

//                     if ((!isWall (x - 1, y)) && (dmap[x - 1][y] > currCellValue + 1)) {
//                         dmap[x - 1][y] = currCellValue + 1;
//                         changesMade = true;
//                     }

//                     if ((!isWall (x, y - 1)) && (dmap[x][y - 1] > currCellValue + 1)) {
//                         dmap[x][y - 1] = currCellValue + 1;
//                         changesMade = true;
//                     }

//                     if ((!isWall (x, y + 1)) && (dmap[x][y + 1] > currCellValue + 1)) {
//                         dmap[x][y + 1] = currCellValue + 1;
//                         changesMade = true;
//                     }
//                 }
//             }
//         }
//     }

// }

// 13/08/2018 -- 22:27 -- I don't like neither of these!
Position *getPos (i32 id) {

    // for (ListElement *e = dlist_start (positions); e != NULL; e = e->next) {
    //     Position *pos = (Position *) LIST_DATA (e);
    //     if (pos->objectId == id) return pos;
    // }

    // return NULL;

}

// simple movement update for our entities based on the Dijkstra's map
// TODO: maybe we can create a more advanced system based on a state machine??
// TODO: this is a good place for multi-threading... I am so excited ofr that!!!
// TODO: maybe a more efficient way is to only update the movement of the
// enemies that are in a close range?

// void fight (Combat *att, Combat *def, bool isPlayer);

void updateMovement (void) {

    /* for (ListElement *e = dlist_start (movement); e != NULL; e = e->next) {
        Movement *mv = (Movement *) LIST_DATA (e);

        // determine if we are going to move this tick
        mv->ticksUntilNextMov -= 1;
        if (mv->ticksUntilNextMov <= 0) {
            Position *pos = getPos (mv->objectId);
            Position newPos = { .objectId = pos->objectId, .x = pos->x, .y = pos->y, .layer = pos->layer };

            // a monster should only move towards the player if it has seen him
            // and should chase him if we has been in the monster fov in the last 5 turns   
            bool chase = false;
            // player is visible
            if (fovMap[pos->x][pos->y] > 0) {
                chase = true;
                mv->chasingPlayer = true;
                mv->turnsSincePlayerSeen = 0;
            }

            // player is not visible, but see if we can still chase him...
            else {
                chase = mv->chasingPlayer;
                mv->turnsSincePlayerSeen += 1;
                if (mv->turnsSincePlayerSeen > 5) mv->chasingPlayer = false;
            }

            i32 speedCounter = mv->speed;
            while (speedCounter > 0) {
                // determine if we are in combat range with the player
                if ((fovMap[pos->x][pos->y] > 0) && (dmap[pos->x][pos->y] == 1)) {
                    // fight the player!
                    GameObject *enemy = searchGameObjectById (mv->objectId);
                    if (enemy != NULL) 
                        fight ((Combat *) getComponent (enemy, COMBAT), main_player->combat, false);
                }
                
                else {
                    if (chase) {
                        Position moves[4];
                        unsigned int moveCounter = 0;
                        i32 currTargetValue = dmap[pos->x][pos->y];
                        if (dmap[pos->x - 1][pos->y] < currTargetValue) {
                            Position np = newPos;
                            np.x -= 1;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        if (dmap[pos->x][pos->y - 1] < currTargetValue) {
                            Position np = newPos;
                            np.y--;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        if (dmap[pos->x + 1][pos->y] < currTargetValue) {
                            Position np = newPos;
                            np.x += 1;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        if (dmap[pos->x][pos->y + 1] < currTargetValue) {
                            Position np = newPos;
                            np.y += 1;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        // pick a random potential pos
                        if (moveCounter > 0) {
                            u32 moveIdx = (u32) randomInt (0, moveCounter);
                            newPos = moves[moveIdx];
                        }

                    }

                    // if we are not chasing the player, we are in patrol state, so move randomly
                    else {
                        u32 dir = (u32) randomInt (0, 4);
                        switch (dir) {
                            case 0: newPos.x -= 1; break;
                            case 1: newPos.y -= 1; break;
                            case 2: newPos.x += 1; break;
                            case 3: newPos.y += 1; break;
                            default: break;
                        }
                    }

                    // check if we can move to the new pos
                    if (canMove (newPos, false)) {
                        // Updating the entity position in an odd way
                        pos->x = newPos.x;
                        pos->y = newPos.y;
                        mv->ticksUntilNextMov = mv->frecuency;
                    }

                    else mv->ticksUntilNextMov += 1;
                }

                speedCounter -= 1;
            }

        }

    } */

}

#pragma endregion

/*** LOOT - ITEMS ***/

#pragma region LOOT

// FIXME:
DoubleList *generateLootItems (u32 *dropItems, u32 count) {

    // FIXME: take into account that we have differente tables for weapons
    // and armour and for normal items
    // FIXME: 07/09/2018 -- 09:16 -- we can only create normal items
    // connect to the items db to get items probability

    // get each items probability to calculate drops
    // double itemsProbs[count];

    // sqlite3_stmt *res;
    // char *sql = "SELECT * FROM Items WHERE Id = ?";
    // for (u32 i = 0; i < count; i++) {
    //     if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK)
    //         sqlite3_bind_int (res, 1, dropItems[i]);

    //     // FIXME: better error handling
    //     // else {
    //     //     fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
    //     //     return 1;
    //     // } 
    // }

    // sqlite3_finalize (res);

    // generate a random number of loot items
    // FIXME: this will be based depending of the enemy
    // u8 itemsNum = (u8) randomInt (0, 2);

    // if (itemsNum == 0) return NULL;
    // else {
    //     fprintf (stdout, "Creating loot items...\n");

    //     DoubleList *lootItems = dlist_init (free);

    //     // generate random loot drops based on items probability
    //     // FIXME: 07/09/2018 -- 09:44 this is just for testing
    //     // we are only slectig the first items in dropItems and ignoring probs
    //     // FIXME: fix the drop items index
    //     Item *item = NULL;
    //     u8 type;
    //     for (u8 i = 0; i < itemsNum; i++) {
    //         type = dropItems[i] / 1000;
    //         switch (type) {
    //             case 1: item = createItem (dropItems[i]); break;
    //             case 2: item = createWeapon (dropItems[i]); break;
    //             case 3: item = createArmour (dropItems[i]); break;
    //             default: break;
    //         }

    //         if (item) dlist_insert_after (lootItems, dlist_end (lootItems), item);
    //     }

    //     return lootItems;
    // }

}

// FIXME:
// generate random loot based on the enemy
void *createLoot (void *arg) {

    GameObject *go = (GameObject *) arg;

    // Monster *mon = searchMonById (go->dbId);

    // if (mon != NULL) {
        fprintf (stdout, "Creating new loot...\n");

        // Loot newLoot;

        // FIXME: create a better system
        // generate random money directly
        // newLoot.money[0] = randomInt (mon->loot.minGold, mon->loot.maxGold);
        // newLoot.money[1] = randomInt (0, 99);
        // newLoot.money[2] = randomInt (0, 99);

        // newLoot.lootItems = generateLootItems (mon->loot.drops, mon->loot.dropCount);

        // // add the loot struct to the go as a component
        // //addComponent (go, LOOT, &newLoot);

        // fprintf (stdout, "New loot created!\n");
    // }

}

Loot *currentLoot = NULL;

bool emptyLoot (Loot *loot) {

    bool noItems, noMoney;

    if (loot->lootItems != NULL) {
      if (dlist_size (loot->lootItems) <= 0) noItems = true;  
    } 
    else noItems = true;

    if (loot->money[0] == 0 && loot->money[1] == 0 && loot->money[2] == 0) noMoney = true;

    if (noItems && noMoney) return true;
    else return false;

}

void displayLoot (void *goData) {

    /* GameObject *go = NULL;
    if (goData != NULL) go = searchGameObjectById (((GameObject *)(goData))->id);

    if (go != NULL) {
        Loot *loot = (Loot *) getComponent (go, LOOT);
        if (loot != NULL) {
            if (!emptyLoot (loot)) {
                currentLoot = loot;
                toggleLootWindow ();
            }

            else {
                currentLoot = NULL;
                logMessage ("There is no loot here.", WARNING_COLOR);
            } 
            
        }   

        else {
            currentLoot = NULL;
            logMessage ("There is no loot here.", WARNING_COLOR);
        } 
    } */
    
}

void collectGold (void) {

    /* if (currentLoot != NULL) {
        if (currentLoot->money[0] > 0 || currentLoot->money[1] > 0 || currentLoot->money[2] > 0) {
            char *str = createString ("You collected: %ig %is %ic",
                currentLoot->money[0], currentLoot->money[1], currentLoot->money[2]);
            
            if ((main_player->money[2] + currentLoot->money[2]) >= 100) {
                main_player->money[1] += 1;
                main_player->money[2] = 0;
                currentLoot->money[2] = 0;
            }

            else {
                main_player->money[2] += currentLoot->money[2];
                currentLoot->money[2] = 0;
            } 

            if ((main_player->money[1] + currentLoot->money[1]) >= 100) {
                main_player->money[0] += 1;
                main_player->money[1] = 0;
                currentLoot->money[1] = 0;
            }

            else {
                main_player->money[1] += currentLoot->money[1];
                currentLoot->money[1] = 0;
            }

            main_player->money[0] += currentLoot->money[0];
            currentLoot->money[0] = 0;

            logMessage (str, DEFAULT_COLOR);
            free (str);

        }

        else logMessage ("There is not gold here to collect!", WARNING_COLOR);

    } */

}

void destroyLoot (void) {

    // clean up active loot objects
    /* if (loot != NULL) {
        if (dlist_size (loot) > 0) {
            Loot *lr = NULL;
            for (ListElement *e = dlist_start (loot); e != NULL; e = e->next) {
                lr = (Loot *) e->data;
                if (lr->lootItems != NULL) {
                    if (dlist_size (lr->lootItems) > 0) {
                        for (ListElement *le = dlist_start (lr->lootItems); le != NULL; le = le->next) 
                            dlist_remove_element (lr->lootItems, le);
                        
                    }

                    free (lr->lootItems);
                }   
            }
        }

        dlist_destroy (loot);
    }

    // clean up loot pool
    if (lootPool != NULL) {
        if (POOL_SIZE (lootPool) > 0) {
            Loot *lr = NULL;
            for (PoolMember *p = POOL_TOP (lootPool); p != NULL; p = p->next) {
                lr = (Loot *) p->data;
                if (lr->lootItems != NULL) {
                    if (dlist_size (lr->lootItems) > 0) {
                        for (ListElement *le = dlist_start (lr->lootItems); le != NULL; le = le->next) 
                            dlist_remove_element (lr->lootItems, le);
                    }

                    free (lr->lootItems);
                }
            } 
        }
        pool_clear (lootPool);
    } */

}

#pragma endregion

/*** COMBAT ***/

#pragma region COMBAT

// FIXME: fix probability
// char *calculateDefense (Combat *def, bool isPlayer) {

    /* GameObject *defender = NULL;
    if (isPlayer) defender = searchGameObjectById (def->objectId);

    u8 chance = (u8) randomInt (0, 3);
    char *msg = NULL;
    switch (chance) {
        case 0: break;
        case 1: {
            if (def->defense.dodge > 0) {
                u8 roll = (u8) randomInt (1, 100);
                if (roll <= def->defense.dodge) {
                    if (!isPlayer) logMessage ("You dodge the attack.", STOPPED_COLOR);
                    else {
                        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
                        if (g != NULL) 
                            msg = createString ("The %s dodges your attack.", g->name);
                    }
                } 
            }
        } break;
        case 2: {
            if (def->defense.parry > 0) {
                u8 roll = (u8) randomInt (1, 100);
                if (roll <= def->defense.parry) {
                    if (!isPlayer) logMessage ("You parry the attack.", STOPPED_COLOR);
                    else {
                        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
                        if (g != NULL) 
                            msg = createString ("The %s parries your attack.", g->name);
                    }  
                }
            }
        } break;
        case 3: {
            if (def->defense.block > 0) {
                u8 roll = (u8) randomInt (1, 100);
                if (roll <= def->defense.block) {
                    // FIXME: only block if you have a shield equipped
                    if (!isPlayer) logMessage ("You block the attack.", STOPPED_COLOR);
                    else {
                        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
                        if (g != NULL) 
                            msg = createString ("The %s blocks your attack.", g->name);
                    }
                }  
            }
        } break;
        default: break;
    }

    return msg; */

// }

// FIXME:
// u32 getPlayerDmg (Combat *att) {

    /* u32 damage;

    Item *mainWeapon = main_player->weapons[MAIN_HAND];
    if (mainWeapon != NULL) {
        Weapon *main = (Weapon *) getItemComponent (mainWeapon, WEAPON);
        if (main == NULL) {
            fprintf (stderr, "No weapon component!\n");
            return 0;
        } 

        // check if we are wielding 2 weapons
        Item *offWeapon = NULL;
        if (main->twoHanded == false) {
            if (main_player->weapons[OFF_HAND] != NULL) offWeapon = main_player->weapons[OFF_HAND];

            // FIXME: take into acount if it is a shield or whatever
            // FIXME: how do we calculate the damage?

            // FIXME: this is just for testing
            // if we only have one weapon
            if (offWeapon == NULL) damage = main->dps;
        }  

        // FIXME: this is only for testing
        // FIXME: also create a more dynamic damage
        // fighting with a two handed weapon
        else damage = main->dps;
    }

    // FIXME: add a more dynamic damage
    // we are fighting we our bare hands!
    else {
        if (att->attack.baseDps == 1) damage = randomInt (1, 2);
        else 
            damage = (u32) randomInt (att->attack.baseDps - (att->attack.baseDps / 2), att->attack.baseDps);
    }

    // FIXME: how do we want to handle strength
    damage += att->baseStats.strength;

    return damage; */

// }

// u32 calculateDamage (Combat *att, Combat *def, bool isPlayer) {

    /* GameObject *attacker = NULL;
    GameObject *defender = NULL;
    if (isPlayer) defender = searchGameObjectById (def->objectId);
    else attacker = searchGameObjectById (att->objectId);

    u32 damage;

    if (isPlayer) damage = getPlayerDmg (att);
    // if the attacker is a mob, just get the the base dps + strength
    else {
        // damage = att->attack.baseDps;
        damage = (u32) randomInt (att->attack.baseDps - (att->attack.baseDps / 2), att->attack.baseDps);
        damage += att->baseStats.strength;
    }

    // take a roll to decide if we can hit a critical
    bool critical = false;
    if (randomInt (1, 100) <= att->attack.criticalStrike) {
        critical = true;
        damage *= 2;
    } 

    char *str = NULL;

    if (isPlayer) {
        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
        str = createString ("You hit the %s for %i damage.", g->name, damage);
        if (critical) logMessage (str, CRITICAL_COLOR);
        else logMessage (str, HIT_COLOR);
    }

    else {
        Graphics *g = (Graphics *) getComponent (attacker, GRAPHICS);
        str = createString ("The %s hits you for %i damage!", g->name, damage);
        logMessage (str, DAMAGE_COLOR);
    }

    free (str);

    return damage; */

// }

void updatePlayerScore (GameObject *);

void checkForKill (GameObject *defender, bool isPlayer) {

    // check for monster kill
    /* if (isPlayer) {
        if (((Combat *) getComponent (defender, COMBAT))->baseStats.health <= 0) {
            // we want the player to be able to walk over the corpse
            ((Position *) getComponent (defender, POSITION))->layer = MID_LAYER; 
            Physics *phys = (Physics *) getComponent (defender, PHYSICS);
            phys->blocksMovement = false;
            phys->blocksSight = false;

            removeComponent (defender, MOVEMENT);

            pthread_t lootThread;

            if (pthread_create (&lootThread, NULL, createLoot, defender) != THREAD_OK)
                fprintf (stderr, "Error creating loot thread!\n");

            Event e = { 0, displayLoot };
            addComponent (defender, EVENT, &e);

            // TODO: maybe add a better visial feedback
            Graphics *gra = (Graphics *) getComponent (defender, GRAPHICS);
            gra->glyph = '%';
            gra->fgColor = 0x990000FF;
            char *str = createString ("You killed the %s.", gra->name);
            logMessage (str, KILL_COLOR);
            free (str);

            updatePlayerScore (defender);

            if (pthread_join (lootThread, NULL) != THREAD_OK)
                fprintf (stderr, "Error joinning loot thread!\n");

        }
    }

    // check for player death
    else {
        if (main_player->combat->baseStats.health <= 0) {
            logMessage ("You have died!!", KILL_COLOR);
            // TODO: player death animation?
            void gameOver (void);
            gameOver ();
        }
    } */

}

// FIXME:
// TODO: 16/08/2018 -- 18:59 -- we can only handle melee weapons
// void fight (Combat *att, Combat *def, bool isPlayer) {

    /* GameObject *attacker = NULL;
    GameObject *defender = NULL;
    if (isPlayer) defender = searchGameObjectById (def->objectId); 
    else attacker = searchGameObjectById (att->objectId);

    // FIXME: check for attack speed

    // check for the attack hit chance
    u32 hitRoll = (u32) randomInt (1, 100);
    if (hitRoll <= att->attack.hitchance) {
        // chance of blocking, parry or dodge
        char *msg = calculateDefense (def, isPlayer);
        if (msg == NULL) {
            // health = maxhealth = basehealth + armor
            def->baseStats.health -= calculateDamage (att, def, isPlayer);

            checkForKill (defender, isPlayer);
        }

        else {
            if (msg != NULL) {
                logMessage (msg, STOPPED_COLOR);
                free (msg);
            } 
        }

    }

    // The attcker missed the target
    else {
        if (isPlayer) logMessage ("Your attack misses.", MISS_COLOR);

        else {
            Graphics *g = (Graphics *) getComponent (attacker, GRAPHICS);
            char *str = createString ("The %s misses you.", g->name);
            logMessage (str, MISS_COLOR);
            free (str);
        }
    } */

// }

#pragma endregion

/*** LEVEL MANAGER ***/

#pragma region LEVEL MANAGER

void updateLeaderBoards (void);

// TODO: wrap all the things up before we return to the main menu
void returnToMenu (void) {

    // update the leaderboards files
    updateLeaderBoards ();

}

// FIXME:
// This is called every time we generate a new level to start fresh, only from data from the pool
void clearOldLevel (void) {

    /* newId = 0; // reset the go id

    void *data = NULL;

    if (gameObjects != NULL) {
        if (dlist_size (gameObjects) > 0) {
            // send all of our objects and components to ther pools
            for (ListElement *e = dlist_start (gameObjects); e != NULL; e = e->next) {
                data = dlist_remove_element (gameObjects, e);
                if (data != NULL) destroyGO ((GameObject *) data);
            }
        }

    }

    if (items != NULL) {
        if (dlist_size (items) > 0) {
            // send the items to their pool
            for (ListElement *e = dlist_start (items); e != NULL; e = e->next) {
                data = dlist_remove_element (items, e);
                if (data != NULL) destroyItem ((Item *) data);
            }
        }
    } */
 
}

// game over logic
void gameOver (void) {  

    // setActiveScene (postGameScreen ());

    // destroyGameUI ();
    // resetGameUI ();

}

extern bool inGame;

// we will have the game update every time the player moves...
void *updateGame (void *data) {

    /* u32 timePerFrame = 1000 / FPS_LIMIT;
    u32 frameStart;
    i32 sleepTime;

    while (inGame) {
        frameStart = SDL_GetTicks ();

        if (playerTookTurn) {
            generateTargetMap (main_player->pos->x, main_player->pos->y);
            updateMovement ();

            playerTookTurn = false;
        }

        // recalculate the fov
        if (recalculateFov) {
            calculateFov (main_player->pos->x, main_player->pos->y, fovMap);
            recalculateFov = false;
        }

        // limit the FPS
        sleepTime = timePerFrame - (SDL_GetTicks () - frameStart);
        if (sleepTime > 0) SDL_Delay (sleepTime);
    } */

}

extern bool inGame;
extern bool wasInGame;
// extern pthread_t gameThread;

void startGame (void) {

    // cleanUpMenuScene ();
    // activeScene = NULL;

    // initGame ();

    // setActiveScene (gameScene ());

    // inGame = true;
    // wasInGame = true; 

    // if (pthread_create (&gameThread, NULL, updateGame, NULL) != THREAD_OK)
    //     fprintf (stderr, "Error creating game thread!\n");

}

// gets you into a nw level
void useStairs (void *goData) {

    /* currentLevel->levelNum += 1;

    void generateLevel (void);
    generateLevel ();

    calculateFov (main_player->pos->x, main_player->pos->y, fovMap);

    // TODO: what is our win condition?

    char *msg = createString ("You are now on level %i", currentLevel->levelNum);
    logMessage (msg, 0xFFFFFFFF);
    free (msg); */

}

void placeStairs (Point spawn) {

    /* GameObject *stairs = createGO ();
    Position p = { 0, spawn.x, spawn.y, MID_LAYER };
    addComponent (stairs, POSITION, &p);
    Graphics g = { 0, '<', 0xFFD700FF, 0x00000000, false, true, "Stairs" };
    addComponent (stairs, GRAPHICS, &g);
    Physics phys = { 0, false, false };
    addComponent (stairs, PHYSICS, &phys);
    Event e = { 0, useStairs };
    addComponent (stairs, EVENT, &e); */

}

// FIXME: 
void generateLevel (void) {

    // make sure we have cleaned the previous level data
    /* clearOldLevel ();

    // this is used to render the walls to the screen... but maybe it is not a perfect system
    initMap (currentLevel->mapCells);

    // TODO: create other map elements such as stairs
    // As of 20/08/2018 -- 23:18 -- we can only move through the dungeon using the stair cases
    // but we can only move forward, we can not return to the previous level
    // Point stairsPoint = getFreeSpot (currentLevel->mapCells);
    placeStairs (getFreeSpot (currentLevel->mapCells));

    fprintf (stdout, "Creating monsters...\n");

    // FIXME: how do we handle how many monsters to add
    u8 monNum = 15;
    u8 count = 0;
    for (u8 i = 0; i < monNum; i++) {
        // generate a random monster
        GameObject *monster = createMonster (getMonsterId ());
        if (monster != NULL) {
            // spawn in a random position
            Point monsterSpawnPos = getFreeSpot (currentLevel->mapCells);
            Position *monsterPos = (Position *) getComponent (monster, POSITION);
            monsterPos->x = (u8) monsterSpawnPos.x;
            monsterPos->y = (u8) monsterSpawnPos.y;
            // TODO: mark the spawnPos as filled
            count++;
        }
    }

    fprintf (stdout, "%i / %i monsters created successfully\n", count, monNum); */

}

void enterDungeon (void) {

    
    // // generate a random world froms scratch
    // // TODO: maybe later we want to specify some parameters based on difficulty?
    // // or based on the type of terrain that we want to generate.. we don't want to have the same algorithms
    // // to generate rooms and for generating caves or open fields

    // // after we have allocated the new level structure, we can start generating the first level
    // generateLevel ();
    
    // // TODO: add different texts here!!
    // // logMessage ("You have entered the dungeon!", 0xFFFFFFFF);

    // fprintf (stdout, "You have entered the dungeon!\n");

}

// TODO: add a loading window??
// we reload the game scene as the same character
void retry (void) {

    // if (!messageLog) messageLog = dlist_init (free);

    // pthread_t playerThread;

    // if (pthread_create (&playerThread, NULL, playerLogic, NULL) != THREAD_OK) 
    //     die ("Error creating player thread!\n");

    // void resetScore (void);
    // resetScore ();

    // currentLevel->levelNum = 0;

    // // create a new level
    // enterDungeon ();

    // if (pthread_join (playerThread, NULL) != THREAD_OK) die ("Error joinning player thread!\n");

    // // we can now place the player and we are done 
    // /* Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    // main_player->pos->x = (u8) playerSpawnPos.x;
    // main_player->pos->y = (u8) playerSpawnPos.y; 

    // calculateFov (main_player->pos->x, main_player->pos->y, fovMap); */

    // fprintf (stdout, "Setting active scene...\n");
    // setActiveScene (gameScene ());
    // fprintf (stdout, "Destroying post gam screen...\n");
    // destroyPostGameScreen ();

}

#pragma endregion

/*** SCORE ***/

#pragma region SCORE

// LBEntry *playerLBEntry = NULL;

// // FIXME: modify to use player profiles
// // we need to modify this when we add multiplayer i guess...
// LBEntry *getPlayerLBEntry (void) {

//     /* LBEntry *entry = (LBEntry *) malloc (sizeof (LBEntry));

//     // entry->playerName = (char *) calloc (strlen (player->name) + 1, sizeof (char));
//     // strcpy (entry->playerName, player->name);
//     // entry->completeName = createString ("%s the %s", player->name, getPlayerClassName (player->cClass));

//     entry->nameColor = player_get_class_color (main_player->cClass);

//     entry->level = createString ("%i", currentLevel->levelNum);

//     entry->kills = createString ("%i", playerScore->killCount);

//     entry->score = playerScore->score;
//     // 26/09/2018 -- we are reversing the string for a better display in the UI
//     entry->reverseScore = reverseString (createString ("%i", playerScore->score));

//     return entry; */

// }

// void updatePlayerScore (GameObject *monster) {

//     playerScore->score += monster->dbId;

//     playerScore->killCount++;

// }

// void resetScore (void) {

//     playerScore->killCount = 0;
//     playerScore->score = 0;

// }

// // TODO: later we will want to handle the logic of connecting to the server an retrieving data of
// // the global leaderboards
// void showScore (void) {

//     // clean up the level 
//     clearOldLevel ();

//     // get player score ready for display
//     playerLBEntry = getPlayerLBEntry ();

//     // render score image with the current score struct
//     toggleScoreScreen ();

//     // delete death screen UI and image
//     deleteDeathScreen ();

// }

#pragma endregion

/*** MULTIPLAYER ***/

#pragma region MULTIPLAYER

#include "cerver/client.h"

#include "ui/menu.h"

bool multiplayer = false;

char *black_server_ip = "127.0.0.1";
u16 black_port = 9001;

Client *player_client = NULL;

Connection *main_connection = NULL;

Lobby *current_lobby = NULL;

extern char *login_error_text;

// void multiplayer_handle_failed_auth (void *);
void multiplayer_send_black_credentials (void *);

void multiplayer_error_packet_handler (void *);
void multiplayer_packet_handler (void *);

// u8 multiplayer_start (BlackCredentials *black_credentials) {

//     player_client = client_create ();

//     if (player_client) {
//         // client_register_to_error_type (player_client, multiplayer_handle_failed_auth, NULL,
//         //     ERR_FAILED_AUTH);

//         client_set_app_error_packet_handler (player_client, multiplayer_error_packet_handler);
//         client_set_app_packet_handler (player_client, multiplayer_packet_handler);

//         main_connection = client_connection_new (black_port, true);
//         if (main_connection) {
//             BlackAuthData *authdata = (BlackAuthData *) malloc (sizeof (BlackAuthData));
//             authdata->connection = main_connection;
//             authdata->credentials = black_credentials;

//             connection_register_to_success_auth (main_connection, toggleLaunch, NULL);

//             client_connect_to_server (player_client, main_connection, 
//                 black_server_ip, black_port, GAME_SERVER,
//                 multiplayer_send_black_credentials, authdata);
//         }

//         return 0;
//     }

//     return 1;

// }

// // FIXME:
// u8 multiplayer_stop (void) {

//     // client_disconnectFromServer (player_client, main_connection);
//     client_teardown (player_client);

//     return 0;

// }

// void multiplayer_handle_failed_auth (const char *msg) {

//     if (login_error_text) free (login_error_text);
//     login_error_text = (char *) calloc (64, sizeof (char));
//     strcpy (login_error_text, msg);    

// }

// void multiplayer_send_black_credentials (void *data) {

//     Connection *connection = NULL;
//     BlackCredentials *credentials = NULL;

//     if (data) {
//         BlackAuthData *authdata = (BlackAuthData *) data;
//         connection = authdata->connection;
//         credentials = authdata->credentials;

//         if (connection && credentials) {
//             size_t packet_size = sizeof (PacketHeader) + sizeof (RequestData) + sizeof (BlackCredentials);
//             void *req = client_generatePacket (AUTHENTICATION, packet_size);
//             if (req) {
//                 char *end = req;
//                 RequestData *reqdata = (RequestData *) (end += sizeof (PacketHeader));
//                 reqdata->type = CLIENT_AUTH_DATA;

//                 BlackCredentials *blackcr = (BlackCredentials *) (end += sizeof (RequestData));
//                 strcpy (blackcr->password, credentials->password);
//                 strcpy (blackcr->username, credentials->username);
//                 blackcr->login = credentials->login;

//                 if (client_sendPacket (connection, req, packet_size) < 0) 
//                     logMsg (stderr, ERROR, PACKET, "Failed to send black credentials packet!");

//                 // else {
//                 //     #ifdef BLACK_DEBUG
//                 //         logMsg (stdout, SUCCESS, PACKET, "Sent authentication packet to server.");
//                 //     #endif
//                 // }
//                 free (req);
//             }
//         }
//     }

// }

// void multiplayer_submit_credentials (void *data) {

//     if (data) {
//         BlackCredentials *credentials = (BlackCredentials *) data;

//         if (!player_client && !main_connection) {
//             #ifdef BLACK_DEBUG
//                 logMsg (stdout, DEBUG_MSG, NO_TYPE, "Starting multiplayer...");
//             #endif
//             if (multiplayer_start (credentials)) 
//                 logMsg (stderr, ERROR, CLIENT, "Failed to start multiplayer!");
//         }

//         // just send the new credentials to the server
//         else {
//             BlackAuthData *authdata = (BlackAuthData *) malloc (sizeof (BlackAuthData));
//             authdata->connection = main_connection;
//             authdata->credentials = credentials;

//             connection_remove_auth_data (main_connection);
//             connection_set_auth_data (main_connection, authdata);

//             multiplayer_send_black_credentials (main_connection->authData);
//         } 
//     }

//     else {
//         logMsg (stderr, ERROR, NO_TYPE, "No credentials provided!");
//         if (login_error_text) free (login_error_text);
//         login_error_text = (char *) calloc (64, sizeof (char));
//         strcpy (login_error_text, "Error - No credentials provided!");
//     }

// }

// void multiplayer_error_packet_handler (void *data) {
    
//     if (data) {
//         PacketInfo *pack_info = (PacketInfo *) data;
//         char *end = pack_info->packetData;
//         BlackError *error = (BlackError *) (end + sizeof (PacketHeader));

//         logMsg (stderr, ERROR, NO_TYPE, createString ("Black error - %s", error->msg));

//         switch (error->errorType) {
//             case BLACK_ERROR_SERVER: break;

//             case BLACK_ERROR_WRONG_CREDENTIALS: 
//                 multiplayer_handle_failed_auth ("Error - Wrong credentials"); 
//                 break;
//             case BLACK_ERROR_USERNAME_TAKEN: 
//                 multiplayer_handle_failed_auth ("Error - Username already taken!");
//                 break;

//             default:
//                 #ifdef BLACK_DEBUG
//                 logMsg (stdout, WARNING, NO_TYPE, "Unknown black error type!");
//                 #endif
//         }
//     }

// }

// // FIXME: player profile
// void multiplayer_packet_handler (void *data) {

//     if (data) {
//         /* PacketInfo *pack_info = (PacketInfo *) data;
//         char *end = pack_info->packetData;
//         BlackPacketData *black_data = (BlackPacketData *) (end += sizeof (PacketHeader));
//         switch (black_data->blackPacketType) {
//             case PLAYER_PROFILE: {
//                 SPlayerProfile *s_profile = (SPlayerProfile *) (end += sizeof (BlackPacketData));
//                 player_profile_get_from_server (s_profile); 
//             } break;

//             default: 
//             #ifdef BLACK_DEBUG
//             logMsg (stdout, WARNING, GAME, "Unknwon black packet type.");
//             #endif
//             break;
//         } */
//     }

// }

// // TODO: we need to pass the game type
// // called from the main menu to request the server to create a new lobby
// void multiplayer_createLobby (void *data) {

//     Lobby *new_lobby = (Lobby *) client_game_createLobby (player_client, main_connection, ARCADE);
//     if (new_lobby != NULL) {
//         #ifdef CLIENT_DEBUG   
//             logMsg (stdout, SUCCESS, NO_TYPE, "Got a new lobby from server!");
//         #endif
//         current_lobby = new_lobby;

//         #ifdef CLIENT_DEBUG
//             logMsg (stdout, DEBUG_MSG, NO_TYPE, "New lobby values: ");
//             printf ("Game type: %i\n", new_lobby->settings.gameType);
//             printf ("Player timeout: %i\n", new_lobby->settings.playerTimeout);
//             printf ("FPS: %i\n", new_lobby->settings.fps);
//             printf ("Max players: %i\n", new_lobby->settings.maxPlayers);
//             printf ("Min players: %i\n", new_lobby->settings.minPlayers);
//         #endif

//         toggleLobbyMenu ();
//     }
        
//     else {
//         #ifdef CLIENT_DEBUG
//             logMsg (stderr, ERROR, NO_TYPE, "Failed to get a new lobby from server!");
//         #endif 
        
//         // FIXME: give feedback to the player
//     }

// }

// // TODO: we need to pass the game type
// // called from the main menu ro request the server to search a lobby for use
// void multiplayer_joinLobby (void *data) {

//     Lobby *new_lobby = (Lobby *) client_game_joinLobby (player_client, main_connection, ARCADE);
//     if (new_lobby != NULL) {
//         #ifdef CLIENT_DEBUG   
//             logMsg (stdout, SUCCESS, NO_TYPE, "Got a new lobby from server!");
//         #endif
//         current_lobby = new_lobby;

//         // FIXME: move to the lobby screen!
//     }
        
//     else {
//         #ifdef CLIENT_DEBUG
//             logMsg (stderr, ERROR, NO_TYPE, "Failed to get a new lobby from server!");
//         #endif 
        
//         // FIXME: give feedback to the player
//     }

// }

// // TODO:
// void multiplayer_leaveLobby (void) {

//     // TODO: check connection, check that we are on a game and handle logic

// }

#pragma endregion

/*** LEADERBOARDS ***/

// FIXME: 17/11/2018 -- we need to update our logic to better work with the server!!

#pragma region LEADERBOARDS

// this are from the makefile
const char localLBFilePath[64] = "./data/localLB.cfg";
const char globalLBFilePath[64] = "./data/globalLB.cfg";

// FIXME: USE THIS!!!
bool changedLocalLB = false;

bool updateGlobalLb = false;

Config *localLBConfig = NULL;
Config *globalLBConfig = NULL;

DoubleList *localLBData = NULL;
DoubleList *globalLBData = NULL;

// FIXME:
// get the config data into a list
// we expect the data to be already sorted!!
DoubleList *getLBData (Config *config) {

    /* DoubleList *lbData = dlist_init (free);

    ConfigEntity *entity = NULL;
    char *class = NULL;
    u8 c;

    for (ListElement *e = dlist_start (localLBConfig->entities); e != NULL; e = e->next) {
        entity = (ConfigEntity *) e->data;
        LBEntry *lbEntry = (LBEntry *) malloc (sizeof (LBEntry));
 
        lbEntry->playerName = getEntityValue (entity, "name");
        c = atoi (getEntityValue (entity, "class"));
        class = player_get_class_name (c);

        if ((lbEntry->playerName != NULL) && (class != NULL)) 
            lbEntry->completeName = createString ("%s the %s", lbEntry->playerName, class);
        else if (lbEntry->playerName != NULL) {
            lbEntry->completeName = (char *) calloc (strlen (lbEntry->playerName) + 1, sizeof (char));
            strcpy (lbEntry->completeName, lbEntry->playerName);
        }
        // if some how the data got corrupted...
        else {
            lbEntry->playerName = (char *) calloc (10, sizeof (char));
            strcpy (lbEntry->playerName, "Anonymous");
            lbEntry->completeName = (char *) calloc (10, sizeof (char));
            strcpy (lbEntry->completeName, "Anonymous");
        }

        lbEntry->nameColor = player_get_class_color (c);

        lbEntry->level = getEntityValue (entity, "level");

        lbEntry->kills = getEntityValue (entity, "kills");

        // this is used for the sort
        char *score = getEntityValue (entity, "score");
        lbEntry->score = atoi (score);
        // 26/09/2018 -- we are reversing the string for a better display in the UI
        lbEntry->reverseScore = reverseString (score);

        dlist_insert_after (lbData, dlist_end (lbData), lbEntry);
        
        free (score);
    }

    return lbData; */

}

DoubleList *getLocalLBData (void) {

    DoubleList *lbData = NULL;

    // check if we have a .conf file
    // localLBConfig = parseConfigFile ("./data/localLB.cfg");
    // if (localLBConfig != NULL) {
    //     lbData = getLBData (localLBConfig);

    //     // insert the current player score at the end no matter what
    //     // dlist_insert_after (lbData, NULL, playerLBEntry);

    //     // then sort the list
    //     // lbData->start = mergeSort (dlist_start (lbData));
    // } 

    // else {
    //     // FIXME: we don't have a local leaderboard, so create one
    // }

    // return the list for display
    return lbData;

}

// FIXME:
// FIXME: check the date of the file
// FIXME: server connection
DoubleList *getGlobalLBData (void) {

    // DoubleList *globalData = NULL;

    // // check if we have already a .conf file
    // globalLBConfig = parseConfigFile ("./data/globalLB.cfg");
    // if (globalLBConfig != NULL) {
    //     globalData = getLBData (globalLBConfig);

    //     // insert the player score no matter what
    //     dlist_insert_after (globalData, NULL, playerLBEntry);

    //     // then sort the list
    //     // globalData->start = mergeSort (dlist_start (globalData));
    // } 

    // we don't have a global lb file, so connect to the server
    /* else {
        if (!connectedToServer) {
            // if (initConnection ()) {
            //     fprintf (stderr, "Failed to retrieve global LB!\n");
            //     // FIXME: give feedback to the player
            // }
        }

        // we are connected, so request the file
        if (makeRequest (REQ_GLOBAL_LB) != 0) fprintf (stderr, "Failed request!\n");
        else {
             // FIXME: check that we have got a valid file

            globalLBConfig = parseConfigFile ("./data/globalLB.cfg");
            if (globalLBConfig != NULL) {
                globalData = getLBData (globalLBConfig);

                // insert the player score no matter what
                dlist_insert_after (globalData, NULL, playerLBEntry);

                // then sort the list
                // globalData->start = mergeSort (dlist_start (globalData));
            } 
        } 

        closeConnection ();
    } */

    // return globalData;

}

// FIXME:
Config *createNewLBCfg (DoubleList *lbData) {

    // make sure that we are only wrtitting the 10 best scores...
    // don't forget that the data is sorted like < 

    // Config *cfg = (Config *) malloc (sizeof (Config));
    // cfg->entities = dlist_init (free);

    // u8 count = 0;
    // ListElement *e = dlist_end (lbData);
    // LBEntry *entry = NULL;
    // while (count < 10 && e != NULL) {
    //     entry = (LBEntry *) e->data;

    //     ConfigEntity *newEntity = (ConfigEntity *) malloc (sizeof (ConfigEntity));
    //     newEntity->name = createString ("%s", "ENTRY");
    //     setEntityValue (newEntity, "name", entry->playerName);
    //     setEntityValue (newEntity, "class", createString ("%i", entry->charClass));
    //     setEntityValue (newEntity, "level", entry->level);
    //     setEntityValue (newEntity, "kills", entry->kills);
    //     setEntityValue (newEntity, "score", createString ("%i", entry->score));

    //     dlist_insert_after (cfg->entities, dlist_start (cfg->entities), newEntity);

    //     e = e->prev;
    //     count++;
    // }

    // return cfg;

}

// FIXME: server connection
// TODO: maybe make an async call to the server to upload the file
void updateLBFile (const char *filename, Config *cfg, bool globalLB) {   

    // write out the cfg file
    // writeConfigFile (filename, cfg);

    /* if (globalLB) {
        if (!connectedToServer) {
            if (initConnection ()) {
                fprintf (stderr, "Failed to retrieve global LB!\n");
                // FIXME: give feedback to the player
            }
        }

        // we are connected, so post the file
        if (makeRequest (POST_GLOBAL_LB) != 0) fprintf (stderr, "Failed post request!\n");

        closeConnection ();
    }*/

}

void updateLeaderBoards (void) {

    // update local
    updateLBFile (localLBFilePath, createNewLBCfg (localLBData), false);

    if (updateGlobalLb)
        updateLBFile (globalLBFilePath, createNewLBCfg (globalLBData), true);

}

void deleteLBEntry (LBEntry *entry) {

    if (entry != NULL) {
        if (entry->playerName) free (entry->playerName);
        if (entry->completeName) free (entry->completeName);
        if (entry->level) free (entry->level);
        if (entry->kills) free (entry->kills);
        if (entry->reverseScore) free (entry->reverseScore);

        free (entry);
    }

}

void destroyLeaderBoard (DoubleList *lb) {

    while (dlist_size (lb) > 0) 
        deleteLBEntry ((LBEntry *) dlist_remove_element (lb, dlist_end (lb)));
        
    free (lb);

}

void cleanLeaderBoardData (void) {

    // delete config data
    // if (localLBConfig) clearConfig (localLBConfig);
    // if (globalLBConfig) clearConfig (globalLBConfig);

    // // delete parsed data
    // if (localLBData) destroyLeaderBoard (localLBData);
    // if (globalLBData) destroyLeaderBoard (globalLBData);

}

#pragma endregion