/*** MAP ***/

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "blackrock.h"
#include "game.h"

#include "room.h"
#include "map.h"

#include "console.h"

#include "list.h"

#include "myUtils.h"

// TODO: is this the system that we want to go with??
// if yes, probably allocate inside the map generator, 
// we will not want this hanging out later
bool mapCells[MAP_WIDTH][MAP_HEIGHT];

/*** OTHER ***/

Point randomRoomPoint (Room *room) {

    u32 px = (room->x, (room->x + room->w));
    u32 py = (room->y, (room->y + room->h));

    Point randPoint = { px, py };
    return randPoint;

}

/*** CARVING ***/

bool carveRoom (unsigned int x, unsigned int y, unsigned int w, unsigned int h) {

    // checks if it overlaps another room
    // and also check that their are not to close together
    for (u8 i = x; i < x + (w + 1); i++) 
        for (u8 j = y; j < y + (h + 1); j++)
            if (mapCells[i][j] == false) return false;

    // carve the room
    for (u8 i = x; i < x + w; i++) 
        for (u8 j = y; j < y + h; j++)
            mapCells[i][j] = false;

    return true;

}

void carveCorridorHor (Point from, Point to) {

    u32 first, last;
    if (from.x < to.x) {
        first = from.x;
        last = to.x;
    }

    else {
        first = to.x;
        last = from.x;
    }

    for (u32 x = first; x <= last; x++) 
        mapCells[x][from.y] = false;

}

void carveCorridorVer (Point from, Point to) {

    u32 first, last;
    if (from.y < to.y) {
        first = from.y;
        last = to.y;
    }

    else {
        first = to.y;
        last = from.y;
    }

    for (u32 y = first; y <= last; y++)
        mapCells[from.x][y] = false;

}

/*** SEGMENTS ***/ 

void getSegments (List *segments, Point from, Point to, Room *firstRoom) {

    // find all the spans between rooms
    Point curr = from;
    bool horizontal = false;
    i8 step = 1;
    if (from.y == to.y) {
        horizontal = true;
        if (from.x > to.x) step = -1;
    }

    else if (from.y > to.y) step = -1;

    i8 currRoom = -1;
    Point lastPoint = from;

    while (curr.x != to.x && curr.y != to.y) {
        i8 rm = roomWithPoint (curr, firstRoom);

        if (rm != -1 && rm != currRoom) {
            // we have a new segment
            Segment *s = (Segment *) malloc (sizeof (Segment));
            s->start = lastPoint;
            s->end = curr;
            s->roomFrom = currRoom;
            s->roomTo = rm;
            insertAfter (segments, NULL, s);

            currRoom = rm;
            lastPoint = curr;
        }

        // move to the next cell
        if (horizontal) curr.x += step;
        else curr.y += step;
    }

}

void carveSegment (List *segments, List *hallways) {



}


/*** DRAWING ***/

// TODO: we are testing having the walls in a separte array in memory for conviniece
// for the other systems tha we want to implement in the other gameObjects...

Wall walls[MAX_WALLS];

// TODO: what color do we want for walls?
void createWall (u32 x, u32 y, u32 wallCount) {

    Wall *new = &walls[wallCount];

    // TODO: better error checking here...
    // are we returning a valid GO??
    assert (new != NULL);

    new->x = x;
    new->y = y;
    new->glyph = '|';
    new->fgColor = 0xFFFFFFFF;
    new->bgColor = 0x000000FF;
    new->blocksMovement = true;
    new->blocksSight = true;

}

// Controls the algorithms for generating random levels with the desired data
void generateMap () {

    fprintf (stdout, "Generating the map...\n");

    // mark all the cells as filled
    for (u32 x = 0; x < MAP_WIDTH; x++) 
        for (u32 y = 0; y < MAP_HEIGHT; y++) 
            mapCells[x][y] = true;

    // carve out non-overlaping rooms that are randomly placed, 
    // and of random size
    bool roomsDone = false;


    // FIXME: do we want to allocate so much memory like this?
    // 08/08/2018 7:05 --> testing a linked list to allocate this memory
    // Rect rooms[100];
    // our list of rooms
    Room *first = NULL;

    u32 roomCount = 0;
    u32 cellsUsed = 0;
    
    // create room data
    fprintf (stdout, "Generating rooms...\n");
    while (!roomsDone) {
        // generate a random width/height for a room
        unsigned int w = randomInt (4, 12);
        unsigned int h = randomInt (4, 12);

        // generate random positions
        unsigned int x = randomInt (1, MAP_WIDTH - w - 1);
        unsigned int y = randomInt (1, MAP_HEIGHT - h - 1);

        if (carveRoom (x, y, w, h)) {
            // Rect rect = { x, y, w, h };
            // rooms[roomCount] = rect;

            Room roomData = { x, y, w, h, NULL };
            if (roomCount == 0) first = createRoomList (first, &roomData);
            else addRoom (first, &roomData);
            
            roomCount++;
            cellsUsed += (w * h);
        }

        if (((float) cellsUsed / (float) (MAP_HEIGHT * MAP_WIDTH)) > 0.45) roomsDone = true;

    }

    // This is a simple way of drawing corridors, is this a good way, 
    // or do we need a more advanced system??
    // join all the rooms with corridors
    fprintf (stdout, "Generating corridors...\n");
    
    // creating corridors using arrays...
    {
        // for (u32 r = 1; r < roomCount; r++) {
        //     // Rect from = rooms[r - 1];
        //     // Rect to = rooms[r];

        //     Room *from;
        //     Room *to;

        //     Point fromPt = randomRoomPoint (from);
        //     Point toPt = randomRoomPoint (to);

        //     // simple way of making corridors
        //     // TODO: do we want a more complex method?
        //     // think in project prourcopine pathfinding...
        //     if (randomInt (0, 1) == 0) {
        //         // move horizontal, then vertical
        //         Point midPt = { toPt.x, fromPt.y };
        //         carveCorridorHor (fromPt, midPt);
        //         carveCorridorVer (midPt, toPt);
        //     }

        //     else {
        //         // move vertical, then horizontal
        //         Point midPt = { fromPt.x, toPt.y };
        //         carveCorridorVer (fromPt, midPt);
        //         carveCorridorHor (midPt, toPt);
        //     }
        // }
    }
    
    // creating corridors using the list of rooms
    // 08/08/2018 -- 7:55
    // I think we got it working the same way as the array, but we still need to tweak
    // how the map generates in general..

    // join all the rooms with corridors
    List *hallways;
    initList (&hallways, &free);

    Room *ptr = first->next, *preptr = first;
    while (ptr != NULL) {
        Room *from = preptr;
        Room *to = ptr;

        Point fromPt = randomRoomPoint (from);
        Point toPt = randomRoomPoint (to);

        List *segments = (List *) malloc (sizeof (List));
        initList (&segments, &free);

        // simple way of making corridors
        // TODO: do we want a more complex method?
        // think in project prourcopine pathfinding...
        Point midPt;
        if (randomInt (0, 1) == 0) {
            // move horizontal, then vertical
            // midPt = { toPt.x, fromPt.y };
            midPt.x = toPt.x;
            midPt.y = fromPt.y;


            // carveCorridorHor (fromPt, midPt);
            // carveCorridorVer (midPt, toPt);
        }

        else {
            // move vertical, then horizontal
            // midPt = { fromPt.x, toPt.y };
            midPt.x = fromPt.x;
            midPt.y = toPt.y;
            // carveCorridorVer (fromPt, midPt);
            // carveCorridorHor (midPt, toPt);
        }

        // break the hallway into segments
        getSegments (segments, fromPt, midPt); 
        getSegments (segments, midPt, toPt);

        // get rid of any segment that connects rooms that are already joined
        ListElement *ptr = LIST_START (segments);
        while (ptr != NULL) {


            ptr = ptr->next;
        }

        // carve out new segments and add them to the hallways list
        carveSegment (segments, hallways);

        preptr = preptr->next;
        ptr = ptr->next;

        // clean up lists
        destroyList (segments);
        free (segments);
    }

    // clean up rooms
    first = deleteList (first);

    destroyList (hallways);
    free (hallways);

}

// This function controls the flow of execution on how to generate a new map
// this primarilly should be called after we have decided to go to the adventure from the main menu (tavern)
unsigned int initWorld (GameObject *player) {

    // TODO: make sure that we have cleared the last level data
    // clear gameObjects and properly handle memory 

    // TODO: this can be a good place to check if we have a save file of a map and load thhat from disk

    // generate a random world froms scratch
    // TODO: maybe later we want to specify some parameters based on difficulty?
    // or based on the type of terrain that we want to generate.. we don't want to have the same algorithms
    // to generate rooms and for generating caves or open fiels
    generateMap ();

    // draw the map
    fprintf (stdout, "Drawing the map...\n");
    unsigned int wallCount = 0;
    for (u32 x = 0; x < MAP_WIDTH; x++)
        for (u32 y = 0; y < MAP_HEIGHT; y++)
            if (mapCells[x][y]) createWall (x, y, wallCount), wallCount++;

    // TODO: how do we want to initialize other objects or NPCs and enemies??

    // the last thing is to place our player in a random place
    for (;;) {
        u32 spawnX = (u32) randomInt (0, MAP_WIDTH);
        u32 spawnY = (u32) randomInt (0, MAP_HEIGHT);

        if (mapCells[spawnX][spawnY] == false) {
            // Position pos = { player->id, x, y };
            // addComponentToGO (player, POSITION, &pos);
            player->x = spawnX;
            player->y = spawnY;
            break;
        }
    }

    return wallCount;

}