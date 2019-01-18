#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "blackrock.h"

#include "game/game.h"
#include "game/entity.h"
#include "game/enemy.h"

#include "engine/animation.h"

#include "utils/llist.h"
#include "utils/log.h"
#include "utils/myUtils.h"

const char *enemiesDbPath = "enemies.db";
sqlite3 *enemiesDb;

#pragma region Enemy Data

LList *enemyData = NULL;

#ifdef DEV
void createEnemiesDb (void) {

    char *err_msg = 0;

    // create monsters table
    char *sql = "DROP TABLE IF EXISTS Monsters;"
                "CREATE TABLE Monsters(Id INT, Name TEXT, Probability DOUBLE)";

    // create combat stats table
    sql = "DROP TABLE IF EXISTS Combat;"
          "CREATE TABLE Combat(Id INT, Health INT, Armour INT, Dps INT, Strength INT, Hitchance INT, Speed INT, Critical INT, Dodge INT, Parry INT, Block INT)";

    // create movement stats table
    sql = "DROP TABLE IF EXISTS Movement;"
          "CREATE TABLE Movement(Id INT, Speed INT, Frequency INT)";

    // create graphics table
    sql = "DROP TABLE IF EXISTS Graphics;"
          "CREATE TABLE Graphics(Id INT, Glyph INT, Color TEXT)";

    if (sqlite3_exec (enemiesDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf (stderr, "Error! Failed to create GRAPHICS table!\n");
        fprintf (stderr, "SQL error: %s\n", err_msg);
        sqlite3_free (err_msg);
    }

    else fprintf (stdout, "Table created!\n");

    // 06/09/2018 -- first approach to a drop system
    char *sql = "DROP TABLE IF EXISTS Drops;"
                "CREATE TABLE Drops(Id INT, Min INT, Max INT, Items TEXT)";

    if (sqlite3_exec (enemiesDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf (stderr, "Error! Failed to create Drops table!\n");
        fprintf (stderr, "SQL error: %s\n", err_msg);
        sqlite3_free (err_msg);
    }

}
#endif

static u8 enemy_loot_load (u32 monId, EnemyLoot *loot) {

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Drops WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    loot->minGold = (u32) sqlite3_column_int (res, 1);
    loot->maxGold = (u32) sqlite3_column_int (res, 2);

    // get the possible drop items
    const char *c = sqlite3_column_text (res, 3);
    if (c != NULL) {
        char *itemsTxt = (char *) calloc (strlen (c) + 1, sizeof (char));
        strcpy (itemsTxt, c);

        // parse the string by commas
        char **tokens = splitString (itemsTxt, ',');
        if (tokens) {
            // count how many elements will be extracted
            u32 count = 0;
            while (*itemsTxt++) if (',' == *itemsTxt) count++;

            count += 1;     // take into account the last item after the last comma

            loot->dropCount = count;
            loot->drops = (u32 *) calloc (count, sizeof (u32));
            u32 idx = 0;
            for (u32 i = 0; *(tokens + i); i++) {
                loot->drops[idx] = atoi (*(tokens + i));
                idx++;
                free (*(tokens + i));
            }

            free (tokens);
        }

        // no drop items in the db or an error somewhere retrieving the data
        // so only hanlde the loot with the money
        else {
            loot->drops = NULL;
            loot->dropCount = 0;
        } 
    }
    
    else {
        loot->drops = NULL;
        loot->dropCount = 0;
    } 

    sqlite3_finalize (res);

    return 0;

}

// Getting the enemies data from the db into memory
static int enemy_data_load_all (void *data, int argc, char **argv, char **azColName) {

    EnemyData *edata = (EnemyData *) malloc (sizeof (EnemyData));

    edata->dbId = atoi (argv[0]);
    char temp[20];
    strcpy (temp, argv[1]);
    edata->name = (char *) calloc (strlen (temp) + 1, sizeof (char));
    strcpy (edata->name, temp);
    edata->probability = atof (argv[2]);

    if (enemy_loot_load (edata->dbId, &edata->loot)) {
        #ifdef DEV
        logMsg (stderr, ERROR, GAME, "Failed getting enemy loot. Enemy id: %i", edata->id);
        #endif
    }

    llist_insert_next (enemyData, llist_end (enemyData), edata);

    return 0;

}

static void enemy_data_delete (void *data) {

    if (data) {
        EnemyData *edata = (EnemyData *) data;
        if (edata->name) free (edata->name);
        if (edata->loot.drops) free (edata->loot.drops);

        free (edata);
    }

}

void enemy_data_delete_all (void) {

    if (enemyData) llist_destroy (enemyData);

}

u8 enemies_connect_db (void) {

    // connect to the enemies db
    if (sqlite3_open (enemiesDbPath, &enemiesDb) != SQLITE_OK) {
        #ifdef DEV
        logMsg (stderr, ERROR, GAME, "Problems connectin to enemies db!");
        logMsg (stderr, ERROR, GAME, createString ("%s", sqlite3_errmsg (enemiesDb)));
        #elif PRODUCTION
        logMsg (stderr, ERROR, NO_TYPE, "Failed to load game data!");
        #endif
        return 1;
    } 

    else {
        #ifdef DEV
        logMsg (stdout, DEBUG_MSG, GAME, "Connected to enemies db.");
        #endif

        // load basic enemy data into memory
        enemyData = llist_init (enemy_data_delete);

        char *err = 0;
        char *sql = "SELECT * FROM Monsters";
            
        if (sqlite3_exec (enemiesDb, sql, enemy_data_load_all, NULL, &err) != SQLITE_OK) {
            #ifdef DEV
            logMsg (stderr, ERROR, GAME, "Problems loading enemy data from db!");
            logMsg (stderr, ERROR, GAME, createString ("%s", err));
            #elif PRODUCTION
            logMsg (stderr, ERROR, NO_TYPE, "Failed to load game data!");
            #endif
            return 1;
        } 
        
        else {
            #ifdef DEV
            logMsg (stdout, DEBUG_MSG, GAME, "Done loading basic enemy data from db.");
            #endif
            return 0;
        }
    }

}

void enemies_disconnect_db (void) {

    sqlite3_close (enemiesDb);
    enemy_data_delete_all ();

}

EnemyData *enemy_data_get_by_id (u32 id) {

    EnemyData *edata = NULL;
    for (ListNode *n = llist_start (enemyData); n != NULL; n = n->next) {
        edata = (EnemyData *) n->data;
        if (edata->dbId == id) return edata;
    }

    return NULL;

}

#pragma endregion

#pragma region Enemy Component

Enemy *enemy_create_comp (u32 goID) {

    Enemy *enemy = (Enemy *) malloc (sizeof (Enemy));
    if (enemy) {
        enemy->goID = goID;
        enemy->entity = entity_new ();
    }

    return enemy;

}

void enemy_destroy_comp (Enemy *enemy) {

    if (enemy) {
        entity_destroy (enemy->entity);
        free (enemy);
    }

}

// TODO: 
void enemy_update (void *data) {}

#pragma endregion

#pragma region Enemy

static void enemy_add_transform (GameObject *enemy_go) {

    if (enemy_go) game_object_add_component (enemy_go, TRANSFORM_COMP);

}

// FIXME:
static void enemy_add_graphics (GameObject *enemy_go, u32 dbID) {

    if (enemy_go) {
        Graphics *graphics = game_object_add_component (enemy_go, GRAPHICS_COMP);
    }

}

// FIXME:
static void enemy_add_animator (GameObject *enemy_go, u32 dbID) {

    if (enemy_go) {
        Animator *animator = game_object_add_component (enemy_go, ANIMATOR_COMP);
    }

}

// FIXME:
static void enemy_get_stats (GameObject *enemy_go, u32 dbID) {

    if (enemy_go) {
        Enemy *enemy = game_object_add_component (enemy_go, ENEMY_COMP);
        enemy->dbID = dbID;

        // get enemy stats from enemy db
    }

}

GameObject *enemy_create (u32 dbID) {

    GameObject *enemy_go = game_object_new (NULL, "enemy");
    if (enemy_go) {
        enemy_add_transform (enemy_go);
        enemy_add_graphics (enemy_go, dbID);
        enemy_add_animator (enemy_go, dbID);
        
        enemy_get_stats (enemy_go, dbID);
    }

    return enemy_go;

}

#pragma endregion

// u8 addGraphicsToMon (u32 monId, Monster *monData, GameObject *mon) {

    // get the db data
    /* sqlite3_stmt *res;
    char *sql = "SELECT * FROM Graphics WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    asciiChar glyph = (asciiChar) sqlite3_column_int (res, 1);
    char *name = (char *) calloc (strlen (monData->name) + 1, sizeof (char));
    strcpy (name, monData->name);
    const char *c = sqlite3_column_text (res, 2);
    char *colour = (char *) calloc (strlen (c) + 1, sizeof (char));
    strcpy (colour, c);
    u32 color = (u32) xtoi (colour);
    Graphics g = { 0, glyph, color, 0x000000FF, false, false, name };
    addComponent (mon, GRAPHICS, &g);

    free (name);
    free (colour);
    sqlite3_finalize (res);

    return 0; */

// }

// u8 addMovementToMon (u32 monId, Monster *monData, GameObject *mon) {

    // get the db data
    /* sqlite3_stmt *res;
    char *sql = "SELECT * FROM Movement WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    u32 speed = (u32) sqlite3_column_int (res, 1);
    u32 frecuency = (u32) sqlite3_column_int (res, 2);
    Movement mv = { .speed = speed, .frecuency = frecuency, .ticksUntilNextMov = 1, .chasingPlayer = false, .turnsSincePlayerSeen = 0 };
    addComponent (mon, MOVEMENT, &mv);

    sqlite3_finalize (res);

    return 0; */

// }

// u8 addCombatToMon (u32 monId, Monster *monData, GameObject *mon) {

    // get the db data
    /* sqlite3_stmt *res;
    char *sql = "SELECT * FROM Combat WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    Combat c;

    c.baseStats.strength = (u32) sqlite3_column_int (res, 4);
    c.attack.baseDps = (u32) sqlite3_column_int (res, 3);
    c.attack.hitchance = (u32) sqlite3_column_int (res, 5);
    c.attack.attackSpeed = (u32) sqlite3_column_int (res, 6);
    c.attack.spellPower = 0;
    c.attack.criticalStrike = (u32) sqlite3_column_int (res, 7);
    c.defense.armor = (u32) sqlite3_column_int (res, 2);
    c.defense.dodge = (u32) sqlite3_column_int (res, 8);
    c.defense.parry = (u32) sqlite3_column_int (res, 9);
    c.defense.block = (u32) sqlite3_column_int (res, 10);

    c.baseStats.maxHealth = ((u32) sqlite3_column_int (res, 1)) + c.defense.armor;
    c.baseStats.health = c.baseStats.maxHealth;

    addComponent (mon, COMBAT, &c);

    sqlite3_finalize (res);

    return 0;  */

// }

// // NEW way for creating the monsters using the enemies db and the in game memory enemy list
// GameObject *createMonster (u32 monId) {

    // get the memory data
    /* Monster *monData = searchMonById (monId);
    if (monData == NULL) {
        fprintf (stderr, "Error! No monster found with the provided id!\n");
        return NULL;
    }

    GameObject *mon = createGO ();
    mon->dbId = monId;

    // This is just a placeholder until it spawns in the world
    Position pos = { .x = 0, .y = 0, .layer = MID_LAYER };
    addComponent (mon, POSITION, &pos);

    // graphics
    if (addGraphicsToMon (monId, monData, mon) != 0) {
        fprintf (stderr, "Error adding graphics component!\n");
        return NULL;
    } 

    Physics phys = { 0, true, false };
    addComponent (mon, PHYSICS, &phys);

    // movement
    if (addMovementToMon (monId, monData, mon) != 0) {
        fprintf (stderr, "Error adding movement component!\n");
        return NULL;
    } 

    // combat
    if (addCombatToMon (monId, monData, mon) != 0) {
        fprintf (stderr, "Error adding combat component!\n");
        return NULL;
    } 

    return mon; */

// }

// // 22/08/2018 -- 7:11 -- this is our first random monster generation function
// // TODO: maybe later also take into account the current level
// // FIXME: create a better system
// u32 getMonsterId (void) {

    // number of monster per type
    /* u8 weak = 2;
    u8 medium = 2;
    u8 strong = 3;
    u8 veryStrong = 2;

    u32 choice;

    switch (randomInt (1, 4)) {
        case 1:
            switch (randomInt (1, weak)) {
               case 1: choice = 101; break; 
               case 2: choice = 102; break; 
            }
            break;
        case 2:
            switch (randomInt (1, medium)) {
                case 1: choice = 201; break;
                case 2: choice = 202; break;
            }
            break;
        case 3:
            switch (randomInt (1, strong)) {
                case 1: choice = 301; break;
                case 2: choice = 302; break;
                case 3: choice = 303; break;
            }
            break;
        case 4:
            switch (randomInt (1, veryStrong)) {
                case 1: choice = 401; break;
                case 2: choice = 402; break;
            }
            break;
        default: break;
    }

    return choice; */

// }

// void cleanUpEnemies (void) {

//     dlist_destroy (enemyData);
//     

// }