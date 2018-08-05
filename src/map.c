/*** MAP ***/

#include "game.h"
#include "console.h"

// TODO: how much space do we want to leave for the HUD??
#define MAP_WIDTH   80
#define MAP_HEIGHT  40

// TODO: is this the system that we want to go with??
// if yes, probably allocate inside the map generator, 
// we will not want this hanging out later
bool mapCells[MAP_WIDTH][MAP_HEIGHT];

bool carveRoom (u32 x, u32 y, u32 w, u32 h) {

    // checks if it overlaps another room
    for (u8 i = x; i < x + w; i++) 
        for (u8 j = y; j < y + h; j++)
            if (mapCells[i][j] == false) return false;

    // carve the room
    for (u8 i = x; i < x + w; i++) 
        for (u8 j = y; j < y + h; j++)
            mapCells[i][j] = false;

    return true;

}

void createWall (u32 x, u32 y) {

    GameObject *wall = createGameObject ();
    Position wallPos = { wall->id, x, y };
    addComponentToGO (wall, POSITION, &wallPos);
    Graphics wallGraphics = { wall->id, '|', 0xFFFFFFFF, 0x000000FF };
    addComponentToGO (wall, GRAPHICS, &wallGraphics);
    Physics wallPhysics = { wall->id, true, true };
    addComponentToGO (wall, PHYSICS, &wallPhysics);

}

void generateMap () {

    // mark all the cells as filled
    for (u32 x = 0; x < MAP_WIDTH; x++) 
        for (u32 y = 0; y < MAP_HEIGHT; y++) 
            mapCells[true];

    // carve out non-overlaping rooms that are randomly placed, 
    // and of random size
    bool roomsDone = false;
    Rect rooms[100];
    u32 roomCount = 0;

    u32 cellsUsed = 0;
    
    // create room data
    while (!roomsDone) {
        // generate a random width/height for a room
        // TODO: create our own rand function
        u32 w = (rand () % 17) + 3; 
        u32 h = (rand () % 17) + 3; 

        u32 x = rand () % MAP_WIDTH - w - 1;
        u32 y = rand () % MAP_HEIGHT - h - 1;

        if (carveRoom (x, y, w, h)) {
            Rect rect = { x, y, w, h };
            rooms[roomCount] = rect;
            roomCount++;
            cellsUsed += (w * h);
        }

        if (((float) cellsUsed / (float) (MAP_HEIGHT * MAP_WIDTH)) > 0.65) roomsDone = true;

    }

    // draw rooms

    // join all the rooms with corridors,
    // and make sure that all of them are reachable

}