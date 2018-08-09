#ifndef POOL_H
#define POOL_H

#include "game.h"

extern GameObject *popGO (GameObject **top);
extern GameObject *pushGO (GameObject *top, GameObject *go);

#endif