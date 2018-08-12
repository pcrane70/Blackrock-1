#ifndef POOL_H
#define POOL_H

typedef struct PoolMember {

    void *data;
    struct ListElement *next;

} PoolMember;

// The pool is just a custom stack implementation
typedef struct Pool {

    unsigned int size;

    ListElement *top;

} Pool;


#define POOL_SIZE(pool) ((pool)->size)

#define POOL_TOP(pool) ((pool)->top)

#define POOL_DATA(member) ((member)->data)


extern Pool *initPool (void);
extern void push (Pool *, void *);

#endif