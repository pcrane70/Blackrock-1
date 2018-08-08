#ifndef ROOM_H
#define ROOM_H

typedef struct Room {

    unsigned int x, y;
    unsigned int w, h;

    struct Room *next;

} Room;

extern Room *createRoomList (Room *first, Room *data);
extern void addRoom (Room *first, Room *data);
extern Room *deleteList (Room *first);

#endif