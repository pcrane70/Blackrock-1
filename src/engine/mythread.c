#include "blackrock.h"
#include "myos.h"

#if defined OS_LINUX
    #include <sys/prctl.h>
#endif

#include "engine/mythread.h"

#include "utils/log.h"

// FIXME: handle portability
// creates a custom detachable thread
u8 thread_create_detachable (void *(*work) (void *), void *args) {

    u8 retval = 1;

    #ifdef OS_LINUX
        pthread_attr_t attr;
        pthread_t thread;

        int rc = pthread_attr_init (&attr);
        rc = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create (&thread, &attr, work, args) != THREAD_OK) 
            logMsg (stderr, ERROR, NO_TYPE, "Failed to create detachable thread!");
        else retval = 0;
    #endif

    return retval;

}

// sets thread name from inisde it
u8 thread_set_name (const char *name) {

    int retval = 1;

    #if defined     OS_LINUX
        // use prctl instead to prevent using _GNU_SOURCE flag and implicit declaration
        retval = prctl (PR_SET_NAME, name);
    #elif defined   OS_MACOS
        retval = pthread_setname_np (name);
    #elif defined   BLACK_DEBUG
        logMsg (stdout, WARNING, NO_TYPE, "pthread_setname_np is not supported on this system.");
    #endif

    return retval;

}