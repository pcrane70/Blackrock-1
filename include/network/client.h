#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdbool.h>

#include <poll.h>

// FIXME: fix this path in the cerver framwork
#include "objectPool.h"

#define MAX_PORT_NUM            65535
#define MAX_UDP_PACKET_SIZE     65515

#define MAXSLEEP                60        // used for connection with exponential backoff (secs)     

#define MAX_PORT_NUM            65535

#define DEFAULT_PROTOCOL                IPPROTO_TCP
#define DEFAULT_PORT                    7001

#pragma region GAME

// this is the same as in cerver
typedef enum GameType {

	ARCADE = 0,

} GameType;

#pragma endregion

#pragma region SERVER 

typedef enum ServerType {

    FILE_SERVER = 1,
    WEB_SERVER, 
    GAME_SERVER

} ServerType;

// this is the same as SSrver in the server
// some useful info about the server
typedef struct Server {

    u8 useIpv6;  
    u8 protocol;            // we only support either tcp or udp
    u16 port; 

    bool isRunning;         // the server is recieving and/or sending packets

    ServerType type;
    bool authRequired;      // authentication required by the server

} Server;

#pragma endregion

#pragma region CLIENT

typedef struct Client {

    i32 clientSock;

    // details about our connection to the server
    u8 useIpv6;  
    u8 protocol;            // 12/10/2018 - we only support either tcp or udp
    u16 port; 

    bool isConnected;       // connected to the server

    bool blocking;          // 31/10/2018 - sokcet fd is blocking?

    // FIXME: don't forget to init the poll structure and the packet pool!!

    // TODO: in a more complex application, maybe the client needs to open
    // mutiple connections to the same server or to other clients
    struct pollfd fds[2];      // 18/11/2018 - we only communicate with the server
    u16 nfds;                  // n of active fds in the pollfd array
    u32 pollTimeout;   

    // TODO: 18/11/2018 - for now we will have this here...
    Pool *packetPool;           //  packet info pool

    // only used in a game server
    // TODO: get details from the server when connecting to it...
    // TODO: move this from here to a server structure
    bool isGameServer;      // is the client connected to a game server?
    bool inLobby;           // is the client inside a lobby?
    bool isOwner;           // is the client the owner of the lobby?

} Client;

extern Client *client_create (Client *);

extern u8 client_connectToServer (Client *);
extern u8 client_disconnectFromServer (Client *);

// TODO: where do we want to put this requests?
extern u8 client_createLobby (Client *owner, GameType gameType);
extern u8 client_joinLobby (Client *owner, GameType gameType);

#pragma endregion

#pragma region PACKETS

// These section needs to be identical as in the server so that we can handle
// the correct requests

// 01/11/2018 - info from a recieved packet to be handle
struct _PacketInfo {

    // Server *server;
    Client *client;
    char packetData[MAX_UDP_PACKET_SIZE];
    size_t packetSize;

};

typedef struct _PacketInfo PacketInfo;

typedef u32 ProtocolId;

typedef struct Version {

	u16 major;
	u16 minor;
	
} Version;

extern ProtocolId PROTOCOL_ID;
extern Version PROTOCOL_VERSION;

// 01/11/2018 -- this indicates what type of packet we are sending/recieving
typedef enum PacketType {

    SERVER_PACKET = 0,
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
    u32 packetSize;             // expected packet size

} PacketHeader;

// 01/11/2018 -- this indicates the data and more info about the packet type
typedef enum RequestType {

    REQ_GET_FILE = 1,
    POST_SEND_FILE,
    
    REQ_AUTH_CLIENT,
    SUCCESS_AUTH,

    LOBBY_CREATE,
    LOBBY_JOIN,
    LOBBY_LEAVE,
    LOBBY_UPDATE,
    LOBBY_DESTROY,

    GAME_INIT,      // prepares the game structures
    GAME_START,     // strat running the game
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