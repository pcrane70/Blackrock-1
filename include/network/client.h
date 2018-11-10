#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdbool.h>

#define MAXSLEEP                60        // used for connection with exponential backoff (secs)     

#define MAX_PORT_NUM            65535

#define DEFAULT_PROTOCOL                IPPROTO_TCP
#define DEFAULT_PORT                    7001

#pragma region CLIENT

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

extern Client *client_create (Client *);

extern u8 client_connectToServer (Client *);
extern u8 client_disconnectFromServer (Client *);

#pragma endregion

#pragma region PACKETS

// These section needs to be identical as in the server so that we can handle
// the correct requests

typedef u32 ProtocolId;

typedef struct Version {

	u16 major;
	u16 minor;
	
} Version;

extern ProtocolId PROTOCOL_ID;
extern Version PROTOCOL_VERSION;

// 01/11/2018 -- this indicates what type of packet we are sending/recieving
typedef enum PacketType {

    ERROR_PACKET = 1,
	REQUEST,
    AUTHENTICATION,
    GAME_PACKET,

    SERVER_TEARDOWN,    // FIXME: create a better packet type for server functions

    TEST_PACKET = 100,
    DONT_CHECK_TYPE,

} PacketType;


typedef struct PacketHeader {

	ProtocolId protocolID;
	Version protocolVersion;
	PacketType packetType;

} PacketHeader;

// 01/11/2018 -- this indicates the data and more info about the packet type
typedef enum RequestType {

    REQ_GET_FILE = 1,
    POST_SEND_FILE,
    
    REQ_AUTH_CLIENT,

    LOBBY_CREATE,
    LOBBY_JOIN,
    LOBBY_LEAVE,
    LOBBY_UPDATE,
    LOBBY_DESTROY,

    GAME_INPUT_UPDATE,
    GAME_SEND_MSG,

} RequestType;

// here we can add things like file names or game types
typedef struct RequestData {

    RequestType type;

} RequestData;

// 23/10/2018 -- lests test how this goes...
typedef enum ErrorType {

    ERR_SERVER_ERROR = 0,   // internal server error, like no memory

    ERR_CREATE_LOBBY = 1,
    ERR_JOIN_LOBBY,

    ERR_FAILED_AUTH,

} ErrorType;

typedef struct ErrorData {

    ErrorType type;
    char msg[256];

} ErrorData;

#pragma endregion

#endif