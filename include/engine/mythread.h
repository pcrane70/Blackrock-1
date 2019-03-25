#ifndef MY_THREAD_H
#define MY_THREAD_H

#include "myos.h"

#if defined OS_LINUX
    #include <pthread.h>
#endif

// creates a custom detachable thread
extern int pthread_create_detachable (void *(*work) (void *), void *args);

// sets thread name from inisde it
extern int thread_set_name (const char *name);

#endif