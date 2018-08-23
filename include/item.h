#ifndef ITEM_H
#define ITEM_H

#include "utils/list.h"
#include "objectPool.h"

extern List *items;
extern Pool *itemsPool;

extern void getItem (void);

#endif