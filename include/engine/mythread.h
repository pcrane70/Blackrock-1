#ifndef MY_THREAD_H
#define MY_THREAD_H

#include "blackrock.h"

extern u8 pthread_create_detachable (void *(*work) (void *), void *args);

#endif