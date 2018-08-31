#include "blackrock.h"

#include "game.h"
#include "player.h"
#include "item.h"

/*** PLAYER ***/

case PLAYER: {
    if (getComponent (go, type) != NULL) return;
    Player *newPlayer = (Player *) malloc (sizeof (Player));
    Player *playerData = (Player *) data;
    newPlayer->name = playerData->name;
    newPlayer->cClass = playerData->cClass;
    newPlayer->genre = playerData->genre;
    newPlayer->inventory = initPlayerInventory ();
    newPlayer->weapons = (Item **) calloc (3, sizeof (Item *));
    for (u8 i = 0; i < 3; i++) newPlayer->weapons[i] = NULL;
    newPlayer->equipment = (Item **) calloc (EQUIPMENT_ELEMENTS, sizeof (Item *));
    for (u8 i = 0; i < EQUIPMENT_ELEMENTS; i++) newPlayer->equipment[i] = NULL;
    newPlayer->level = playerData->level;
    newPlayer->maxWeight = playerData->maxWeight;
    newPlayer->money[0] = playerData->money[0];
    newPlayer->money[1] = playerData->money[1];
    newPlayer->money[2] = playerData->money[2];

    go->components[type] = newPlayer;
} break;

// TODO: check for a save file to retrive the information freom there instead
// because the player is an special GO, we want to initialize him differently
Player *initPlayer (void) {

    Player *go = (Player *) malloc (sizeof (Player));

    for (short unsigned int i = 0; i < COMP_COUNT; i++) go->components[i] = NULL;

    // This is just a placeholder until it spawns in the world
    Position pos = { .x = 0, .y = 0, .layer = TOP_LAYER };
    addComponent (go, POSITION, &pos);

    ConfigEntity *playerEntity = getEntityWithId (playerConfig, 1);

    Physics phys = { 0, true, true };
    addComponent (go, PHYSICS, &phys);

    Player p;
    p.name = getEntityValue (playerEntity, "name");
    p.genre = atoi (getEntityValue (playerEntity, "genre"));
    p.level = atoi (getEntityValue (playerEntity, "level"));

    // get class
    CharClass cClass = atoi (getEntityValue (playerEntity, "class"));
    p.cClass = cClass;
    ConfigEntity *classEntity = getEntityWithId (classesConfig, p.cClass);
    p.color = xtoi (getEntityValue (classEntity, "color"));

    // money
    p.money[0] = atoi (getEntityValue (playerEntity, "gold"));
    p.money[1] = atoi (getEntityValue (playerEntity, "silver"));
    p.money[2] = atoi (getEntityValue (playerEntity, "copper"));

    p.maxWeight = atoi (getEntityValue (playerEntity, "maxWeight")) + atoi (getEntityValue (classEntity, "weightMod"));
    addComponent (go, PLAYER, &p);

    // As of 18/08/2018 -- 23-21 -- the color of the glyph is based on the class
    asciiChar glyph = atoi (getEntityValue (playerEntity, "glyph"));
    Graphics g = { 0, glyph, p.color, 0x000000FF, false, false, NULL };
    addComponent (go, GRAPHICS, &g);

    // TODO: modify the combat component based on the class
    // we need to have a file where we can read the stats we have saved
    // also we need to take into account that every class has different stats
    Combat c;
    c.baseStats.power = atoi (getEntityValue (playerEntity, "power"));
    c.baseStats.powerRegen = atoi (getEntityValue (playerEntity, "powerRegen"));
    c.baseStats.strength = atoi (getEntityValue (playerEntity, "strength"));
    c.attack.baseDps = atoi (getEntityValue (playerEntity, "baseDps"));
    c.attack.hitchance = atoi (getEntityValue (playerEntity, "hitchance"));
    c.attack.attackSpeed = atoi (getEntityValue (playerEntity, "attack_speed"));
    c.attack.spellPower = atoi (getEntityValue (playerEntity, "spellPower"));
    c.attack.criticalStrike = atoi (getEntityValue (playerEntity, "critical"));

    c.defense.armor = atoi (getEntityValue (playerEntity, "armor"));
    c.defense.block = atoi (getEntityValue (playerEntity, "block"));
    c.defense.dodge = atoi (getEntityValue (playerEntity, "dodge"));
    c.defense.parry = atoi (getEntityValue (playerEntity, "block"));

    c.baseStats.maxHealth = (atoi (getEntityValue (playerEntity, "baseHP"))) + c.defense.armor;
    c.baseStats.health = c.baseStats.maxHealth;

    addComponent (go, COMBAT, &c);

    return go;

}

char *getPlayerClassName (void) {

    switch (playerComp->cClass) {
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