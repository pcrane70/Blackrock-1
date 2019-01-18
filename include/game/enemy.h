#ifndef ENEMY_H
#define ENEMY_H

#include "blackrock.h"

#include "game/entity.h"

#include "utils/llist.h"

#define N_MONSTER_TYPES       9

typedef struct EnemyLoot {

    u8 minGold;
    u8 maxGold;
    u32 *drops;
    u8 dropCount;

} EnemyLoot;

#define DB_COL_ENEMY_HEALTH             1
#define DB_COL_ENEMY_STAMINA            2
#define DB_COL_ENEMY_STRENGTH           3

#define DB_COL_ENEMY_ARMOUR             4
#define DB_COL_ENEMY_DODGE              5
#define DB_COL_ENEMY_PARRY              6
#define DB_COL_ENEMY_BLOCK              7

#define DB_COL_ENEMY_HITCHANCE          8
#define DB_COL_ENEMY_DPS                9
#define DB_COL_ENEMY_ATTACK_SPEED       10
#define DB_COL_ENEMY_SPELL_POWER        11
#define DB_COL_ENEMY_CRITICAL           12

// basic enemy data
typedef struct EnemyData {

    u32 dbId;
    char *name;
    double probability;
    EnemyLoot loot;

} EnemyData;

// basic info of all of oour enemies
extern LList *enemyData;

extern void enemy_data_delete_all (void);

// component for a game object
typedef struct Enemy {

    u32 goID;

    u32 dbID;
    LivingEntity *entity;

} Enemy;

extern u8 enemies_connect_db (void);

extern Enemy *enemy_create_comp (u32 goID);
extern void enemy_destroy_comp (Enemy *enemy);

extern void enemy_update (void *data);

extern GameObject *enemy_create (u32 dbID);

#endif