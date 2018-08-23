// FOV = Field of View
/* This is an experimental function to simulate a player that is inside a dungeon
    and there are no lights, so this can simulate the player holding a torch
    to see where he is going. */

#include <string.h>

#include "blackrock.h"
#include "game.h"

#include "utils/list.h"

#define FOV_DISTANCE    8

typedef struct FovCell {

    u32 x, y;

} FovCell;

typedef struct Shadow {

    float startSlope;
    float endSlope;

} Shadow;

static u8 shadoCount = 0;
static Shadow knownShadows[10];
static u8 shadowCount = 0;


/*** SHADOWS ***/

void addShadow (Shadow s) {

    knownShadows[shadoCount] = s;
    shadowCount++;

}

bool cellInShadow (float cellSlope) {

    for (short unsigned int i = 0; i < shadowCount; i++) {
        Shadow s = knownShadows[i];
        if (s.startSlope <= cellSlope && s.endSlope >= cellSlope) return true;
    }

    return false;

}


/*** FOV ***/

float lineSlopeBetween (float x1, float y1, float x2, float y2) {

    if (x2 - x1 <= 0) return 0;
    else return (x2 - x1) / (y2 - y1);

}


FovCell mapCellForLocalCell (u8 sector, FovCell heroMapCell, FovCell cellToTranslate) {

    switch (sector) {
        case 1: { FovCell mapCell = { heroMapCell.x + cellToTranslate.x, heroMapCell.y - cellToTranslate.y };
        return mapCell; } break;
        case 2: { FovCell mapCell = { heroMapCell.x + cellToTranslate.y, heroMapCell.y - cellToTranslate.x };
        return mapCell; } break;
        case 3: { FovCell mapCell = { heroMapCell.x + cellToTranslate.y, heroMapCell.y + cellToTranslate.x };
        return mapCell; } break;
        case 4: { FovCell mapCell = { heroMapCell.x + cellToTranslate.x, heroMapCell.y + cellToTranslate.y };
        return mapCell; } break;
        case 5: { FovCell mapCell = { heroMapCell.x - cellToTranslate.x, heroMapCell.y + cellToTranslate.y };
        return mapCell; } break;
        case 6: { FovCell mapCell = { heroMapCell.x - cellToTranslate.y, heroMapCell.y + cellToTranslate.x };
        return mapCell; } break;
        case 7: { FovCell mapCell = { heroMapCell.x - cellToTranslate.y, heroMapCell.y - cellToTranslate.x };
        return mapCell; } break;
        case 8: { FovCell mapCell = { heroMapCell.x - cellToTranslate.x, heroMapCell.y - cellToTranslate.y };
        return mapCell; } break;
        default: { FovCell c = { 0, 0 }; return c; } break;
    }

}

bool cellBlocksSight (u32 x, u32 y) {

    List *gos = getObjectsAtPos (x, y);
    if (gos != NULL) {
        for (ListElement *e = LIST_START (gos); e != NULL; e = e->next) {
            if (((Physics *) getComponent ((GameObject *) LIST_DATA (e), PHYSICS))->blocksSight) {
                free (gos);
                return true;
            } 
        }
    }

    free (gos);
    return false;

}

float distnaceBetween (u32 x1, u32 y1, u32 x2, u32 y2) {

    return sqrt (pow (x2 - x1, 2) + pow (y2 - y1, 2));

}

void calculateFov (u32 xPos, u32 yPos, u32 fovmap[MAP_WIDTH][MAP_HEIGHT]) {

    // default state
    // memset (fovmap, 0, sizeof (fovmap));
    for (u32 x = 0; x < MAP_WIDTH; x++)
        for (u32 y = 0; y < MAP_HEIGHT; y++)
            fovmap[x][y] = 0;

    // mark the position as visible
    fovmap[xPos][yPos] = 1;

    // loop through all 8 sectors around the pos
    for (u8 i = 1; i <= 8; i++) {
        bool prevBlocking = false;
        shadoCount = 0;
        float shadowStart = 0.0;
        float shadowEnd = 0.0;

        for (u32 cellY = 1; cellY < FOV_DISTANCE; cellY++) {
            prevBlocking = false;
            for (u32 cellX = 0; cellX <= cellY; cellX++) {
                // translate cellX, cellY to map coordinates
                FovCell heroCell = { xPos, yPos };
                FovCell cellToTranslate = { cellX, cellY };
                FovCell mapCell = mapCellForLocalCell (i, heroCell, cellToTranslate);

                if ((mapCell.x < MAP_WIDTH) && (mapCell.y < MAP_HEIGHT)) {
                    if (distnaceBetween (0, 0, cellX, cellY) <= FOV_DISTANCE) {
                        float cellSlope = lineSlopeBetween (0, 0, cellX, cellY);
                        if (!cellInShadow (cellSlope)) {
                            fovmap[mapCell.x][mapCell.y] = 1;
                            if (cellBlocksSight (mapCell.x, mapCell.y)) {
                                if (prevBlocking == false) {
                                    shadowStart = lineSlopeBetween (0, 0, cellX, cellY);
                                    prevBlocking = true;
                                }
                            }

                            else {
                                if (prevBlocking) {
                                    shadowEnd = lineSlopeBetween (0, 0, cellX + 0.5, cellY);
                                    Shadow s = { shadowStart, shadowEnd };
                                    addShadow (s);
                                }
                            }
                        }
                    }
                }
            }

            if (prevBlocking) {
                shadowEnd = lineSlopeBetween (0, 0, cellY + 0.5, cellY);
                Shadow s = { shadowStart, shadowEnd };
                addShadow (s);
            }
        }
    }

}