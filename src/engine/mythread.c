#include "blackrock.h"

#ifdef LINUX
#include <pthread.h>
#endif

#include "utils/log.h"

// FIXME: handle portability
u8 pthread_create_detachable (void *(*work) (void *), void *args) {

    u8 retval = 1;

    #ifdef LINUX
    pthread_attr_t attr;
    pthread_t request_Thread;

    int rc = pthread_attr_init (&attr);
    rc = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create (&request_Thread, &attr, work, args) != THREAD_OK) 
       logMsg (stderr, ERROR, NO_TYPE, "Failed to create request thread!");

    else retval = 0;
    #endif

    return retval;

}