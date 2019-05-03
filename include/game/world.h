#ifndef _WORLD_H_
#define _WORLD_H_

#include "game/camera.h"
#include "game/map/map.h"

#include "collections/dlist.h"

struct _Camera;

typedef struct World {

    Map *game_map;
    struct _Camera *game_camera;

    DoubleList *players;
    DoubleList *enemies;

} World;

extern World *world;

extern World *world_create (void);
extern void world_destroy (World *world);

#endif