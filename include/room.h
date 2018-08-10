#ifndef ROOM_H
#define ROOM_H

#include "blackrock.h"

typedef struct Room {

    u32 x, y;
    u32 w, h;

    struct Room *next;

} Room;

extern Room *createRoomList (Room *first, Room *data);
extern void addRoom (Room *first, Room *data);
extern Room *deleteList (Room *first);

#endif