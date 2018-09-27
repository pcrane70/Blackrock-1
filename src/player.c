#include "blackrock.h"

#include "game.h"
#include "player.h"
#include "item.h"

#include "config.h"

#include "utils/myUtils.h"

extern void die (char *);

Player *player = NULL;
Config *playerConfig = NULL;
Config *classesConfig = NULL;

u8 inventoryItems = 0;

void getPlayerData (void) {

    playerConfig = parseConfigFile ("./data/player.cfg");
    if (playerConfig == NULL) 
        die ("Critical Error! No player config!\n");


    classesConfig = parseConfigFile ("./data/classes.cfg");
    if (classesConfig == NULL) 
        die ("Critical Error! No classes config!\n");

}

char *getPlayerClassName (u8 c) {

    char class[15];

    switch (c) {
        case WARRIOR: strcpy (class, "Warrior"); break;
        case PALADIN: strcpy (class, "Paladin"); break;
        case ROGUE: strcpy (class, "Rogue"); break;
        case PRIEST: strcpy (class, "Priest"); break;
        case DEATH_KNIGHT: strcpy (class, "Death Knight"); break;
        case MAGE: strcpy (class, "Mage"); break;
        default: break;    
    }

    char *retVal = (char *) calloc (strlen (class), sizeof (char));
    strcpy (retVal, class);

    return retVal;

}

u32 getPlayerClassColor (u8 c) {

    u32 retVal;

    switch (c) {
        case WARRIOR: retVal = 0xD63031FF; break;
        case PALADIN: retVal = 0xFD79A8FF; break;
        case ROGUE: retVal = 0xFFC048FF; break;
        case PRIEST: retVal = 0x05C46B; break;
        case DEATH_KNIGHT: retVal = 0x0A3D62FF; break;
        case MAGE: retVal = 0x7158E2FF; break;
        default: break;    
    }

    return retVal;

}

Item ***initPlayerInventory (void) {

    Item ***inventory = (Item ***) calloc (7, sizeof (Item **));

    for (u8 i = 0; i < 7; i++)
        inventory[i] = (Item **) calloc (3, sizeof (Item *));

    for (u8 y = 0; y < 3; y++) 
        for (u8 x = 0; x < 7; x++) 
            inventory[x][y] = NULL;

    return inventory;

}

Player *createPlayer (void) {

    Player *p = (Player *) malloc (sizeof (Player));
    p->pos = (Position *) malloc (sizeof (Position));
    p->physics = (Physics *) malloc (sizeof (Physics));
    p->graphics = (Graphics *) malloc (sizeof (Graphics));
    p->combat = (Combat *) malloc (sizeof (Combat));

    p->inventory = initPlayerInventory ();

    p->weapons = (Item **) calloc (2, sizeof (Item *));
    for (u8 i = 0; i < 2; i++) p->weapons[i] = NULL;

    p->equipment = (Item **) calloc (EQUIPMENT_ELEMENTS, sizeof (Item *));
    for (u8 i = 0; i < EQUIPMENT_ELEMENTS; i++) p->equipment[i] = NULL;

    return p;

}

// TODO: check for a save file to retrive the information freom there instead
// because the player is an special GO, we want to initialize him differently
void initPlayer (Player *p) {

    getPlayerData ();

    p->pos->objectId = 0;
    p->pos->layer = TOP_LAYER;

    p->physics->objectId = 0;
    p->physics->blocksMovement = true;
    p->physics->blocksSight = true;

    ConfigEntity *playerEntity = getEntityWithId (playerConfig, 1);

    if (p->name != NULL) free (p->name);
    p->name = getEntityValue (playerEntity, "name");
    p->genre = atoi (getEntityValue (playerEntity, "genre"));
    p->level = atoi (getEntityValue (playerEntity, "level"));

    // get class
    CharClass cClass = atoi (getEntityValue (playerEntity, "class"));
    p->cClass = cClass;
    ConfigEntity *classEntity = getEntityWithId (classesConfig, p->cClass);
    p->color = xtoi (getEntityValue (classEntity, "color"));

    // money
    p->money[0] = atoi (getEntityValue (playerEntity, "gold"));
    p->money[1] = atoi (getEntityValue (playerEntity, "silver"));
    p->money[2] = atoi (getEntityValue (playerEntity, "copper"));

    // As of 18/08/2018 -- 23-21 -- the color of the glyph is based on the class
    p->graphics->objectId = 0;
    p->graphics->bgColor = 0x000000FF;
    p->graphics->fgColor = p->color;
    p->graphics->hasBeenSeen = false;
    p->graphics->visibleOutsideFov = false;
    p->graphics->glyph = atoi (getEntityValue (playerEntity, "glyph"));
    p->graphics->name = NULL;

    // TODO: modify the combat component based on the class
    // we need to have a file where we can read the stats we have saved
    // also we need to take into account that every class has different stats
    p->combat->baseStats.power = atoi (getEntityValue (playerEntity, "power"));
    p->combat->baseStats.powerRegen = atoi (getEntityValue (playerEntity, "powerRegen"));
    p->combat->baseStats.strength = atoi (getEntityValue (playerEntity, "strength"));
    p->combat->attack.baseDps = atoi (getEntityValue (playerEntity, "baseDps"));
    p->combat->attack.hitchance = atoi (getEntityValue (playerEntity, "hitchance"));
    p->combat->attack.attackSpeed = atoi (getEntityValue (playerEntity, "attack_speed"));
    p->combat->attack.spellPower = atoi (getEntityValue (playerEntity, "spellPower"));
    p->combat->attack.criticalStrike = atoi (getEntityValue (playerEntity, "critical"));

    p->combat->defense.armor = atoi (getEntityValue (playerEntity, "armor"));
    p->combat->defense.block = atoi (getEntityValue (playerEntity, "block"));
    p->combat->defense.dodge = atoi (getEntityValue (playerEntity, "dodge"));
    p->combat->defense.parry = atoi (getEntityValue (playerEntity, "block"));

    p->combat->baseStats.maxHealth = (atoi (getEntityValue (playerEntity, "baseHP"))) + p->combat->defense.armor;
    p->combat->baseStats.health = p->combat->baseStats.maxHealth;

    // TODO: depending on the class, we have a different starting weapon
    u16 startingWeapon = atoi (getEntityValue (playerEntity, "startingWeapon"));
    Item *weapon = createWeapon (startingWeapon);
    if (weapon != NULL) {
        Weapon *w = (Weapon *) getItemComponent (weapon, WEAPON);
        if (w != NULL) fprintf (stdout, "Found the weapon component!\n");
        p->weapons[w->slot] = weapon;
        w->isEquipped = true;
        Graphics *g = (Graphics *) getGameComponent (weapon, GRAPHICS);
        fprintf (stdout, "Done creating: %s\n", g->name);
    }

    else fprintf (stderr, "Problems creating player weapon.");

    // we don't need to have this two in memory
    clearConfig (playerConfig);
    clearConfig (classesConfig);

    fprintf (stdout, "Init player done!\n");

}

void resetPlayer (Player *p) {

    if (p != NULL) {
        // reset inventory
        for (u8 y = 0; y < 3; y++) 
            for (u8 x = 0; x < 7; x++) 
                p->inventory[x][y] = NULL;

        inventoryItems = 0;

        // reset weapons
        for (u8 i = 0; i < 2; i++) p->weapons[i] = NULL;

        // reset equipment
        for (u8 i = 0; i < EQUIPMENT_ELEMENTS; i++) p->equipment[i] = NULL;

    }

}

void destroyPlayer (void) {

    free (player->name);

    // clean up inventory
    for (u8 y = 0; y < 3; y++) 
        for (u8 x = 0; x < 7; x++) 
            player->inventory[x][y] = NULL;

    free (player->inventory);

    // clean up weapons
    for (u8 i = 0; i < 2; i++) player->weapons[i] = NULL;

    free (player->weapons);

    // clean up equipment
    for (u8 i = 0; i < EQUIPMENT_ELEMENTS; i++) player->equipment[i] = NULL;

    free (player->equipment);
        
    free (player->pos);
    free (player->graphics);
    free (player->physics);
    free (player->combat);

    free (player);

}