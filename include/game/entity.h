#ifndef ENTITY_H
#define ENTITY_H

#include "blackrock.h"

typedef enum Genre {

    MALE = 1,
    FEMALE = 2,
    OTHER = 3

} Genre;

extern char *entity_get_genre_name (Genre);

typedef enum CharRace {

    HUMAN = 1,

} CharRace;

extern char *entity_get_race_name (CharRace);

typedef enum CharClass {

    WARRIOR = 1,
    PALADIN,
    ROGUE,
    PRIEST,
    DEATH_KNIGHT,
    MAGE

} CharClass;

extern char *entity_get_class_name (CharClass);

// FIXME: remove from game.h
/* typedef struct Defense {

    u32 armor;      // based on level, class, and equipment
    u32 dodge;      // dodge chance -> everyone can dodge
    u32 parry;      // parry chance -> only works with certain weapons and classes
    u32 block;      // block chance -> this only works with a certain class than can handle shields

} Defense;

typedef struct Attack {

    u32 hitchance;          // chance to not miss the target
    u32 baseDps;            // this is mostly for npcs
    u32 attackSpeed;        // how many hits per turn
    u32 spellPower;         // similar to attack power but for mages, etc
    u32 criticalStrike;     // chance to hit a critical (2x more powerful than normal)

} Attack; */

typedef struct LivingEntity {

    // base stats
    Genre genre;
    CharRace race;
    CharClass cClass;
    u32 maxHealth;
    u32 currHealth;
    char *name;

    u32 strength;
    u32 stamina;

    u32 level;

    // combat stats
    // Attack attack;
    // Defense defense;

} LivingEntity;

extern LivingEntity *entity_new (void);
extern void entity_destroy (LivingEntity *entity);

#endif