#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blackrock.h"
#include "myos.h"

#if defined OS_LINUX
    #include <sys/prctl.h>
#endif

#include "engine/mythread.h"

#include "utils/log.h"

// FIXME: handle portability
// creates a custom detachable thread
int thread_create_detachable (void *(*work) (void *), void *args) {

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
int thread_set_name (const char *name) {

    int retval = 1;

    if (name) {
        #if defined     OS_LINUX
            // use prctl instead to prevent using _GNU_SOURCE flag and implicit declaration
            retval = prctl (PR_SET_NAME, name);
        #elif defined   OS_MACOS
            retval = pthread_setname_np (name);
        #elif defined   BLACK_DEBUG
            logMsg (stdout, WARNING, NO_TYPE, "pthread_setname_np is not supported on this system.");
        #endif
    }

    return retval;

}

typedef struct HubWorker {

    // FIXME: handle portability
    pthread_t thread;

    const char *name;
    void *(*work) (void *);
    void *args;

} HubWorker;

typedef struct ThreadHub {

    const char *name;           // thread hub name

    unsigned int n_workers;
    HubWorker **workers;

} ThreadHub;

// inits a new thread hub
int thread_hub_int (const char *name) {

    ThreadHub *hub = (ThreadHub *) malloc (sizeof (ThreadHub));
    if (hub) {
        memset (hub, 0, sizeof (ThreadHub));
        hub->name = (char *) calloc (strlen (name) + 1, sizeof (char));
        strcpy (hub->name, name);
    }

    return hub;

}

// ends a thread hub
int thread_hub_end (ThreadHub *hub) {

    int retval = 1;

    if (hub) {

    }

    return retval;

}

static HubWorker *hub_worker_new (void *(*work) (void *), void *args, const char *name) {

    HubWorker *worker = (HubWorker *) malloc (sizeof (HubWorker));
    if (worker) {
        memset (worker, 0, sizeof (HubWorker));

        worker->name = (char *) calloc (strlen (name) + 1, sizeof (char));
        strcpy (worker->name, name);

        worker->work = work;
        worker->args = args;
    }

    return worker;

}

static void hub_worker_destroy (HubWorker *worker) {

    if (worker) {
        if (worker->name) free (worker->name);

        free (worker);
    }

}

// adds a thread to the hub
int thread_hub_add (ThreadHub *hub, void *(*work) (void *), void *args, const char *name) {

    int retval = 1;

    if (work && name) {
        HubWorker *worker = hub_worker_new (work, args, name);

        // if no hub provided, add the work to the global thread, but only if it exists...
        // if (!hub) 

        // if (pthread_create (&update_thread, NULL, update, NULL)) {
        //     logMsg (stderr, ERROR, NO_TYPE, "Failed to create update thread!");
        //     running = false;
        // }
    }

    return retval;

}

// FIXME: removes a thread from the hub
int thread_hub_remove (const char *name);