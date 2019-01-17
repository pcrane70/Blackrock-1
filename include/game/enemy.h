#ifndef ENEMY_H
#define ENEMY_H

#include "blackrock.h"

#include "game/entity.h"

#define N_MONSTER_TYPES       9

typedef struct EnemyLoot {

    u8 minGold;
    u8 maxGold;
    u32 *drops;
    u8 dropCount;

} EnemyLoot;

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

    u32 dbid;
    LivingEntity *entity;

} Enemy;

extern u8 enemies_connect_db (void);

#endif