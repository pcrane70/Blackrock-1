#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdbool.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <poll.h>

#include "utils/objectPool.h"
#include "utils/thpool.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int32_t i32;
typedef int64_t i64;

#define MAX_PORT_NUM            65535
#define MAX_UDP_PACKET_SIZE     65515

#define MAXSLEEP                60        // used for connection with exponential backoff (secs)     

#define MAX_PORT_NUM            65535

#define DEFAULT_USE_IPV6                0
#define DEFAULT_PROTOCOL                IPPROTO_TCP
#define DEFAULT_PORT                    7001

#define DEFAULT_POLL_TIMEOUT            180000      // 3 min in mili secs
#define DEFAULT_PACKET_POOL_INIT        4
#define DEFAULT_THPOOL_INIT             4

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

// some useful info about the server we are connectiing to
typedef struct Server {

    u8 useIpv6;  
    u8 protocol;            // we only support either tcp or udp
    u16 port; 

    char *ip;
    struct sockaddr_storage address;

    bool isRunning;         // the server is recieving and/or sending packets

    ServerType type;
    bool authRequired;      // authentication required by the server

} Server;

#pragma endregion

#pragma region CLIENT

// TODO: if a client can only connect to one address at a time, we need to support 
// multiple clients so that we can have multiple connections at the same time
// add the clients in the poll structure...
typedef struct Client {

    // TODO: add the hability to connect to other clients directly
    // 18/11/2018 -- a client can only connect to one address a time right?
    // if so, we are only handling a connection with one server
    Server connectionServer;

    i32 clientSock;
    u8 useIpv6;  
    u8 protocol;            // 12/10/2018 - we only support either tcp or udp
    u16 port; 

    bool blocking;          // 31/10/2018 - sokcet fd is blocking?
    bool running;           // the client is ready to listen & send
    bool isConnected;       // connected to the server

    // TODO: in a more complex application, maybe the client needs to open
    // mutiple connections to the same server or to other clients
    struct pollfd fds[2];      // 18/11/2018 - we only communicate with the server
    u16 nfds;                  // n of active fds in the pollfd array
    u32 pollTimeout;   

    // TODO: 18/11/2018 - for now we will have this here...
    Pool *packetPool;           //  packet info pool

    // 18/11/2018 -- we will have our own thpoll inside the clien
    threadpool thpool;

    // only used in a game server
    // TODO: get details from the server when connecting to it...
    // TODO: move this from here to a server structure
    bool isGameServer;      // is the client connected to a game server?
    bool inLobby;           // is the client inside a lobby?
    bool isOwner;           // is the client the owner of the lobby?

} Client;

extern Client *client_create (Client *);
extern u8 client_start (Client *client);
extern u8 client_teardown (Client *client);

extern u8 client_connectToServer (Client *client, char *serverIp);
extern u8 client_disconnectFromServer (Client *);

#pragma endregion

#pragma region PACKETS

// These section needs to be identical as in the server so that we can handle
// the correct requests

// 01/11/2018 - info from a recieved packet to be handle
struct _PacketInfo {

    // Server *server;
    Client *client;

    // we need this to be dynamic to avoid any memory leak 
    // when we resuse it with the pool
    char *packetData;
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

    SERVER_INFO,
    SERVER_TEARDOWN,

    REQ_GET_FILE,
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

extern u8 client_makeTestRequest (Client *client);

/*** SERIALIZATION ***/

// cerver framework serialized data
#pragma region SERIALIZATION

// 17/11/2018 - send useful server info to the client trying to connect
typedef struct SServer {

    u8 useIpv6;  
    u8 protocol;            // we only support either tcp or udp
    u16 port; 

    bool isRunning;         // the server is recieving and/or sending packets

    ServerType type;
    bool authRequired;      // authentication required by the server

} SServer;

// TODO:
typedef struct SLobby {

    // struct _GameSettings settings;      // 24/10/2018 -- we dont have any ptr in this struct
    bool inGame;

    // FIXME: how do we want to send this info?
    // Player owner;               // how do we want to send which is the owner
    // Vector players;             // ecah client also needs to keep track of other players in the lobby

} SLobby;

// TODO: we need to create a more complex seralized data
// keep in mind that the admin can set a reference to the data and function
// that can handle specific serealization
// 03/11/2018 -> default auth data to use by default auth function
typedef struct DefAuthData {

    u32 code;

} DefAuthData;


#pragma endregion

#endif