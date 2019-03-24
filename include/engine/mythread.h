#ifndef MY_THREAD_H
#define MY_THREAD_H

#include <pthread.h>

#include "blackrock.h"

// creates a custom detachable thread
extern u8 pthread_create_detachable (void *(*work) (void *), void *args);

// sets thread name from inisde it
extern u8 thread_set_name (const char *name);

#endif