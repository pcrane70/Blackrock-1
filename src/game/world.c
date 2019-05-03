#include <stdlib.h>

#include "cengine/renderer.h"

#include "game/game.h"
#include "game/camera.h"
#include "game/world.h"

World *world = NULL;

World *world_create (void) {

    World *new_world = (World *) malloc (sizeof (World));
    if (new_world) {
        new_world->game_map = NULL;
        new_world->game_camera = camera_new (windowSize.width, windowSize.height);

        new_world->players = dlist_init (game_object_destroy_ref, NULL);
        new_world->enemies = dlist_init (game_object_destroy_ref, NULL);
    }

    return new_world;
}

void world_destroy (World *world) {

    if (world) {
        if (world->game_map) map_destroy (world->game_map);
        if (world->game_camera) camera_destroy (world->game_camera);

        dlist_destroy (world->players);
        dlist_destroy (world->enemies);

        free (world);
    }

}