#ifndef CLIENT_H_
#define CLIENT_H_

// FIXME:
typedef enum RequestType {

    REQ_GLOBAL_LB = 1

} RequestType;

/*** CONNECTION **/

extern int initConnection (void);
extern int closeConnection (void);

/*** REQUESTS ***/

int makeRequest (RequestType);

#endif