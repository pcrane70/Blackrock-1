#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdbool.h>

#define MAXSLEEP                60        // used for connection with exponential backoff (secs)     

#define MAX_PORT_NUM            65535

#define DEFAULT_PROTOCOL                IPPROTO_TCP
#define DEFAULT_PORT                    7001

typedef struct Client {

    i32 clientSock;

    // details about our connection to the server
    u8 useIpv6;  
    u8 protocol;            // 12/10/2018 - we only support either tcp or udp
    u16 port; 

    bool isConnected;       // connected to the server

    // FIXME: do we need to set to nonblocking?
    // bool blocking;          // 31/10/2018 - sokcet fd is blocking?

} Client;

// FIXME:
typedef enum RequestType {

    REQ_GLOBAL_LB = 1,
    POST_GLOBAL_LB = 2

} RequestType;


extern Client *client_create (Client *);

extern u8 client_connectToServer (Client *);
extern u8 client_disconnectFromServer (Client *);

#endif