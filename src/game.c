// 03/08/2018 -- 18:18
// For now this will be like our Game Controller, well kind of
// lets see how the game scales and the we will decide what to do

#include <assert.h>

#include "blackrock.h"
#include "game.h"
#include "objectPool.h"

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
// GameObject gameObjects[MAX_GO];

// // TODO: do we want this?? is there a better way around??
// Position positionComps[MAX_GO];
// Graphics graphicComps[MAX_GO];
// Physics physicsComps[MAX_GO];

// as of 03/08/2018 we identify a free space in gameObjects array if it has an id = 0
// GameObject *createGameObject () {

//     // Get an available object space (memory)
//     GameObject *go = NULL;
//     for (u32 i = 1; i < MAX_GO; i++) {
//         if (gameObjects[i].id == 0) {
//             // Set up the new GameObject??
//             go = &gameObjects[i];
//             go->id = i;
//             break;
//         }
//     }

//     // are we returning a valid GO??
//     assert (go != NULL);

//     // TODO: do we want this here or in destriyObject??
//     // cleanning up our components in the obj --- 04/08/2018
//     //for (u32 i = 0; i < MAX_COMP_COUNT; i++) 
//       // 1  go->components[i] = NULL;

//     return go;

// }

// void addComponentToGO (GameObject *obj, GameComponent comp, void *compData) {

//     // make sure that we have a valid GO
//     assert (obj->id != 0);

//     switch (comp) {
//         case POSITION: {
//             Position *pos = &positionComps[obj->id];
//             Position *posData = (Position *) compData;
//             pos->objectId = obj->id;
//             pos->x = posData->x;
//             pos->y = posData->y;

//             obj->components[comp] = pos;

//             break; }
//         case GRAPHICS: {
//             Graphics *graphics = &graphicComps[obj->id];
//             Graphics *graphicsData = (Graphics *) compData;
//             graphics->objectId = obj->id;
//             graphics->glyph = graphicsData->glyph;
//             graphics->fgColor = graphicsData->fgColor;
//             graphics->bgColor = graphicsData->bgColor;

//             obj->components[comp] = graphics;

//             break; }
//         case PHYSICS: {
//             Physics *physics = &physicsComps[obj->id];
//             Physics *physicsData = (Physics *) compData;
//             physics->objectId = obj->id;
//             physics->blocksMovement = physicsData->blocksMovement;
//             physics->blocksSight = physicsData->blocksSight;

//             obj->components[comp] = physics;

//             break; }

//         default: fprintf (stderr, "Unknown component!\n"); break;
//     }

// }

// void *getComponent (GameObject *obj, GameComponent comp) {

//     return obj->components[comp];

// }

// TODO: maybe in the future we will want to have a memory management system
// like an object pool... assuming that all of our objects have the same base!!

// as of 03/08/2018 we identify a free space in gameObjects array if it has an id = 0
// void destroyGO (GameObject *obj) {

//     // cleanning up the components in their arrays
//     positionComps[obj->id].objectId = 0;
//     graphicComps[obj->id].objectId = 0;
//     physicsComps[obj->id].objectId = 0;

//     obj->id = 0;

// }

/*** Using Linked List ***/

// TODO: how do we want to solve the problem of the first GO??

GameObject *createGOList (GameObject *first, GameObject *data) {

    GameObject *go, *ptr;

    go = (GameObject *) malloc (sizeof (GameObject));

    // FIXME: error handling
    if (go == NULL) return NULL;

    // FIXME: how to assign a unique id to each GO?
    // go->id = data->id;  

    go->x = data->x;
    go->y = data->y;
    go->glyph = data->glyph;
    go->fgColor = data->fgColor;
    go->bgColor = data->bgColor;
    go->blocksMovement = data->blocksMovement;
    go->blocksSight = data->blocksSight;

    if (first == NULL) {
        go->next = NULL;
        first = go;
    }

    else {
        ptr = first;
        while (ptr->next != NULL)
            ptr = ptr->next;

        ptr->next = go;
        go->next = NULL;
    }

    return go;

}

void createGO (GameObject *first, GameObject *data) {

    GameObject *ptr = first;
    GameObject *go = NULL;

    // First check if we have any object in our pool
    if (inactive == 0) {
        // if we don't have any...
        // create a new go, and assign the values directly
        go = (GameObject *) malloc (sizeof (GameObject));
          
    }

    else {
        // grab a GO from our Pool...
        // FIXME: how do we store a reference to the stack top?
        // GameObject *go = popGO ();

    }

    // FIXME: how to assign a unique id to each GO?
        // go->id = data->id;

    go->x = data->x;
    go->y = data->y;
    go->glyph = data->glyph;
    go->fgColor = data->fgColor;
    go->bgColor = data->bgColor;
    go->blocksMovement = data->blocksMovement;
    go->blocksSight = data->blocksSight;

    // and add it directly to the end of the GO list
    while (ptr->next != NULL) ptr = ptr->next;

    ptr->next = go;

}

// This calls the Object pooling to deactive the go and have it in memory 
// to reuse it when we need it
void destroyGO (GameObject *first, GameObject *go) {

    

}


// TODO: 07/08/2018
// I think we will need to handle the map objects separatly, 
// because we need to have a way of saving their state to a file,
// to allow the player to continue player where he has left over.

// Also when creating differnt areas, we need to handle allocating memory in the most efficent way
