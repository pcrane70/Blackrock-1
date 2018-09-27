#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdbool.h>

bool connectedToServer;

// FIXME:
typedef enum RequestType {

    REQ_GLOBAL_LB = 1,
    POST_GLOBAL_LB = 2

} RequestType;

extern int clientSocket;

/*** CONNECTION **/

extern int initConnection (void);
extern int closeConnection (void);

/*** REQUESTS ***/

extern int makeRequest (RequestType);

#endif