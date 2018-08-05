// 03/08/2018 -- 18:18
// For now this will be like our Game Controller, well kind of
// lets see how the game scales and the we will decide what to do

#include <assert.h>

#include "blackrock.h"
#include "game.h"

/*** GAME MANAGER ***/

// WORLD STATE




/*** Game Object Management **/

// FIXME: take into account that probably we will want a differnt treatment for the walls,
// because they can scaled so fast and will always be static right?? -- or do we want to be able to break them?
// if they are always static, we will only have to generate them few times, 
// we can make a simplier logic than this.

// Also note that we still don't know how do we want the levels to regenerate when we die
// - do we want a new random level??
// or the same one and only regenerate the levels when we start a new session??
GameObject gameObjects[MAX_GO];

// TODO: do we want this?? is there a better way around??
Position positionComps[MAX_GO];
Graphics graphicComps[MAX_GO];
Physics physicsComps[MAX_GO];

// as of 03/08/2018 we identify a free space in gameObjects array if it has an id = 0
GameObject *createGameObject () {

    // Get an available object space (memory)
    GameObject *go = NULL;
    for (u32 i = 1; i < MAX_GO; i++) {
        if (gameObjects[i].id == 0) {
            // Set up the new GameObject??
            go = &gameObjects[i];
            go->id = i;
            break;
        }
    }

    // are we returning a valid GO??
    assert (go != NULL);

    // TODO: do we want this here or in destriyObject??
    // cleanning up our components in the obj --- 04/08/2018
    for (u32 i = 0; i < MAX_COMP_COUNT; i++) 
        go->components[i] = NULL;

    return go;

}

void addComponentToGO (GameObject *obj, GameComponent comp, void *compData) {

    // make sure that we have a valid GO
    assert (obj->id != 0);

    switch (comp) {
        case POSITION: {
            Position *pos = &positionComps[obj->id];
            Position *posData = (Position *) compData;
            pos->objectId = obj->id;
            pos->x = posData->x;
            pos->y = posData->y;

            obj->components[comp] = pos;

            break; }
        case GRAPHICS: {
            Graphics *graphics = &graphicComps[obj->id];
            Graphics *graphicsData = (Graphics *) compData;
            graphics->objectId = obj->id;
            graphics->glyph = graphicsData->glyph;
            graphics->fgColor = graphicsData->fgColor;
            graphics->bgColor = graphicsData->bgColor;

            obj->components[comp] = graphics;

            break; }
        case PHYSICS: {
            Physics *physics = &physicsComps[obj->id];
            Physics *physicsData = (Physics *) compData;
            physics->objectId = obj->id;
            physics->blocksMovement = physicsData->blocksMovement;
            physics->blocksSight = physicsData->blocksSight;

            obj->components[comp] = physics;

            break; }

        default: fprintf (stderr, "Unknown component!\n"); break;
    }

}

void *getComponent (GameObject *obj, GameComponent comp) {

    return obj->components[comp];

}

// TODO: maybe in the future we will want to have a memory management system
// like an object pool... assuming that all of our objects have the same base!!

// as of 03/08/2018 we identify a free space in gameObjects array if it has an id = 0
void destroyGO (GameObject *obj) {

    // cleanning up the components in their arrays
    positionComps[obj->id].objectId = 0;
    graphicComps[obj->id].objectId = 0;
    physicsComps[obj->id].objectId = 0;

    obj->id = 0;

}