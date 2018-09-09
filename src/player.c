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

char *getPlayerClassName (void) {

    switch (player->cClass) {
        case WARRIOR: return "Warrior"; break;
        case PALADIN: return "Paladin"; break;
        case ROGUE: return "Rogue"; break;
        case PRIEST: return "Priest"; break;
        case DEATH_KNIGHT: return "Death Knight"; break;
        case MAGE: return "Mage"; break;
        default: return NULL; break;    
    }

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

// TODO: check for a save file to retrive the information freom there instead
// because the player is an special GO, we want to initialize him differently
Player *initPlayer (void) {

    getPlayerData ();

    Player *p = (Player *) malloc (sizeof (Player));

    p->pos = (Position *) malloc (sizeof (Position));
    p->pos->objectId = 0;
    p->pos->layer = TOP_LAYER;

    p->physics = (Physics *) malloc (sizeof (Physics));
    p->physics->objectId = 0;
    p->physics->blocksMovement = true;
    p->physics->blocksSight = true;

    ConfigEntity *playerEntity = getEntityWithId (playerConfig, 1);

    p->inventory = initPlayerInventory ();
    p->weapons = (Item **) calloc (2, sizeof (Item *));
    for (u8 i = 0; i < 2; i++) p->weapons[i] = NULL;
    p->equipment = (Item **) calloc (EQUIPMENT_ELEMENTS, sizeof (Item *));
    for (u8 i = 0; i < EQUIPMENT_ELEMENTS; i++) p->equipment[i] = NULL;

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
    p->graphics = (Graphics *) malloc (sizeof (Graphics));
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
    p->combat = (Combat *) malloc (sizeof (Combat));
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

    // FIXME: depending on the class, we have a different starting weapon
    // u16 startingWeapon = atoi (getEntityValue (playerEntity, "startingWeapon"));
    // Item *weapon = createWeapon (startingWeapon);
    // if (weapon != NULL) 
    //     p->weapons[((Weapon *) getItemComponent (weapon, WEAPON))->slot] = weapon;

    // else fprintf (stderr, "Problems creating player weapon.");

    // we don't need to have this two in memory
    clearConfig (playerConfig);
    clearConfig (classesConfig);

    return p;

}

// FIXME: problems cleanning up weapons and equipment
void destroyPlayer (void) {

    free (player->name);

    // clean up inventory
    for (u8 y = 0; y < 3; y++) 
        for (u8 x = 0; x < 7; x++) 
            if (player->inventory[x][y] != NULL)
                player->inventory[x][y] = destroyItem (player->inventory[x][y]);

    free (player->inventory);

    // clean up weapons
    for (u8 i = 0; i < 2; i++)
        if (player->weapons[i] != NULL) 
            player->weapons[i] = destroyItem (player->weapons[i]);

    // // clean up equipment
    for (u8 i = 0; i < EQUIPMENT_ELEMENTS; i++) 
        if (player->equipment[i] != NULL)
            player->equipment[i] = destroyItem (player->equipment[i]);
        
    free (player->pos);
    free (player->graphics);
    free (player->physics);
    free (player->combat);

    free (player);

}