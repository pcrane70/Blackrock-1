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

    // u32 px = (rand () % (room->w - 1)) + room->x;
    // u32 py = (rand () % (room->h - 1)) + room->y;

    u32 px = randomInt (room->x, (room->x + room->w - 1));
    u32 py = randomInt (room->y, (room->y + room->h - 1));

    Point randPoint = { px, py };
    return randPoint;

}

i32 roomWithPoint (Point pt, Room *first) {

    i32 retVal = 0;
    Room *ptr = first;
    while (ptr != NULL) {
        if ((ptr->x <= pt.x) && ((ptr->x + ptr->w) > pt.x) &&
        (ptr->y <= pt.y) && ((ptr->y + ptr->h) > pt.y))
            return retVal;

        ptr = ptr->next;
        retVal++;
    }

    return -1;

}

/*** CARVING ***/

bool carveRoom (u32 x, u32 y, u32 w, u32 h) {

    for (u8 i = x - 1; i < x + (w + 1); i++) 
        for (u8 j = y - 1; j < y + (h + 1); j++)
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

    bool usingWayPoint = false;
    Point waypoint = to;
    if (from.x != to.x && from.y != to.y) {
        usingWayPoint = true;
        if (randomInt (0, 1) == 0) {
            waypoint.x = to.x;
            waypoint.y = from.y;
        }

        else {
            waypoint.x = from.x;
            waypoint.y = to.y;
        }
    }

    Point curr = from;
    bool horizontal = false;
    i8 step = 1;
    if (from.y == waypoint.y) {
        horizontal = true;
        if (from.x > waypoint.x) step = -1;
    }

    else if (from.y > waypoint.y) step = -1;

    i32 currRoom = roomWithPoint (curr, firstRoom);
    Point lastPoint = from;
    bool done = false;
    Segment *turnSegment = NULL;
    while (!done) {
        i32 rm = roomWithPoint (curr, firstRoom);
        if (usingWayPoint && curr.x == waypoint.x && curr.y == waypoint.y) {
            // check if we are in a room
            if (rm != -1) {
                if (rm != currRoom) {
                    // we have a new segment between currRoom and rm
                    Segment *s = (Segment *) malloc (sizeof (Segment));
                    s->start = lastPoint;
                    s->end = curr;
                    s->roomFrom = currRoom;
                    s->roomTo = rm;
                    s->hasWayPoint = false;
                    insertAfter (segments, NULL, s);

                    currRoom = rm;
                }

                else lastPoint = waypoint;
            }

            else {
                turnSegment = (Segment *) malloc (sizeof (Segment));
                turnSegment->start = lastPoint;
                turnSegment->mid = curr;
                turnSegment->hasWayPoint = true;
                turnSegment->roomFrom = currRoom;
            }

            from = curr;
            horizontal = false;
            step = 1;
            if (from.y == to.y) {
                horizontal = true;
                if (from.x > to.x) step = -1;
            }

            else if (from.y > to.y) step = -1;

            if (horizontal) curr.x += step;
            else curr.y += step;
        }

        else if (curr.x == to.x && curr.y == to.y) {
            // we have hit our end point... 
            // check if we are inside another room or in the same one
            if (rm != currRoom) {
                if (turnSegment != NULL) {
                    // we already have a partial segment, so now complete it
                    turnSegment->end = curr;
                    turnSegment->roomTo = rm;
                    insertAfter (segments, NULL, turnSegment);
                    turnSegment = NULL;
                }

                else {
                    // we have a new segment between currRoom and rm
                    Segment *s = (Segment *) malloc (sizeof (Segment));
                    s->start = lastPoint;
                    s->end = curr;
                    s->roomFrom = currRoom;
                    s->roomTo = rm;
                    s->hasWayPoint = false;
                    insertAfter (segments, NULL, s);
                }
            }

            done = true;
        }

        else {
            if (rm != -1 && rm != currRoom) {
                if (turnSegment != NULL) {
                    // complete the partial segment
                    turnSegment->end = curr;
                    turnSegment->roomTo = rm;
                    insertAfter (segments, NULL, turnSegment);
                    turnSegment = NULL;
                }

                else {
                    // we have a new segment 
                    Segment *s = (Segment *) malloc (sizeof (Segment));
                    s->start = lastPoint;
                    s->end = curr;
                    s->roomFrom = currRoom;
                    s->roomTo = rm;
                    s->hasWayPoint = false;
                    insertAfter (segments, NULL, s);
                }

                currRoom = rm;
                lastPoint = curr;
            }

            if (horizontal) curr.x += step;
            else curr.y += step;
        }
    }

}

void carveSegment (List *hallways) {

    ListElement *ptr = LIST_START (hallways);
    while (ptr != NULL) {
        Segment *seg = (Segment *) ptr->data;

        if (seg->hasWayPoint) {
            Point p1 = seg->start;
            Point p2 = seg->mid;

            if (p1.x == p2.x) carveCorridorVer (p1, p2);
            else carveCorridorHor (p1, p2);

            p1 = seg->mid;
            p2 = seg->end;

            if (p1.x == p2.x) carveCorridorVer (p1, p2);
            else carveCorridorHor (p1, p2);
        }

        else {
            Point p1 = seg->start;
            Point p2 = seg->end;

            if (p1.x == p2.x) carveCorridorVer (p1, p2);
            else carveCorridorHor (p1, p2);
        }
    }

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
    Room *firstRoom = NULL;

    u32 roomCount = 0;
    u32 cellsUsed = 0;
    
    // create room data
    fprintf (stdout, "Generating rooms...\n");
    while (!roomsDone) {
        // generate a random width/height for a room
        unsigned int w = randomInt (5, 12);
        unsigned int h = randomInt (5, 12);

        // generate random positions
        unsigned int x = randomInt (1, MAP_WIDTH - w - 1);
        unsigned int y = randomInt (1, MAP_HEIGHT - h - 1);

        if (carveRoom (x, y, w, h)) {
            // Rect rect = { x, y, w, h };
            // rooms[roomCount] = rect;

            Room roomData = { x, y, w, h, NULL };
            if (roomCount == 0) firstRoom = createRoomList (firstRoom, &roomData);
            else addRoom (firstRoom, &roomData);
            
            roomCount++;
            cellsUsed += (w * h);
        }

        if (((float) cellsUsed / (float) (MAP_HEIGHT * MAP_WIDTH)) > 0.45) roomsDone = true;

    }

    // This is a simple way of drawing corridors, is this a good way, 
    // or do we need a more advanced system??
    // join all the rooms with corridors
    fprintf (stdout, "Generating corridors...\n");

    // FIXME: 09/08/2018 -- 23:20 -- corridors still not working good

    {

        /* 
            
        // creating corridors using the list of rooms
        // 08/08/2018 -- 7:55
        // I think we got it working the same way as the array, but we still need to tweak
        // how the map generates in general..

        // join all the rooms with corridors 
        List *hallways = initList (free);

        Room *ptr = firstRoom->next, *preptr = firstRoom;
        while (ptr != NULL) {
            Room *from = preptr;
            Room *to = ptr;

            Point fromPt = randomRoomPoint (from);
            Point toPt = randomRoomPoint (to);

            List *segments = initList (free);

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
            getSegments (segments, fromPt, midPt, firstRoom); 
            getSegments (segments, midPt, toPt, firstRoom);

            // get rid of any segment that connects rooms that are already joined
            ListElement *ptr = LIST_START (segments);
            while (ptr != NULL) {
                i32 rm1 = ((Segment *) (ptr->data))->roomFrom;
                i32 rm2 = ((Segment *) (ptr->data))->roomTo;

                bool foundMatch;
                ListElement *uSeg = NULL;
                ListElement *hallElement = LIST_START (hallways);
                while (hallElement != NULL) {
                    if (((((Segment *) (hallElement->data))->roomFrom == rm1) && 
                        (((Segment *) (hallElement->data))->roomTo == rm2)) ||
                        ((((Segment *) (hallElement->data))->roomTo == rm1) && 
                        (((Segment *) (hallElement->data))->roomFrom == rm2))) {
                            uSeg = hallElement;
                            break;
                    }
                }

                if (uSeg != NULL) insertAfter (hallways, NULL, uSeg);

                ptr = ptr->next;
            }

            // continue looping through the rooms
            preptr = preptr->next;
            ptr = ptr->next;

            // clean up lists
            destroyList (segments);
        }

        // carve out new segments and add them to the hallways list
        carveSegment (hallways);

        */

    } 

    // clean up rooms
    firstRoom = deleteList (firstRoom);

    // destroyList (hallways);

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