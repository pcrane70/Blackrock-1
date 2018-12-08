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

#define DEFAULT_MAX_CONNECTIONS         10
#define DEFAULT_POLL_TIMEOUT            180000      // 3 min in mili secs
#define DEFAULT_PACKET_POOL_INIT        4

#define DEFAULT_THPOOL_INIT             4

#define DEFAULT_AUTH_CODE               0x4CA140FF

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

typedef struct Connection {

    i32 sock_fd;
    u8 useIpv6;  
    u8 protocol;
    u16 port; 

    bool blocking;          // sokcet fd is blocking?
    bool isConnected;       // is the socket connected?

} Connection;

typedef struct Client {

    Connection **active_connections;
    u8 n_active_connections;

    bool running;           // client poll is running

    struct pollfd fds[DEFAULT_MAX_CONNECTIONS];
    u16 nfds;                  // n of active fds in the pollfd array
    u32 pollTimeout;   

    threadpool thpool;

    Pool *packetPool;           //  packet info pool

    // FIXME: fix below logic!!!

    // TODO: add the hability to connect to other clients directly
    // 18/11/2018 -- a client can only connect to one address a time right?
    // if so, we are only handling a connection with one server
    Server *connectionServer;

    // only used in a game server
    // TODO: get details from the server when connecting to it...
    // TODO: move this from here to a server structure
    bool inLobby;           // is the client inside a lobby?
    bool isOwner;           // is the client the owner of the lobby?


} Client;

/*** PUBLIC CLIENT FUNCTIONS ***/

extern Client *client_create (void);
extern u8 client_teardown (Client *client);

extern u8 client_connectToServer (Client *client, 
    char *serverIp, u16 port, ServerType expectedType);
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

// these indicate what type of packet we are sending/recieving
typedef enum PacketType {

    SERVER_PACKET = 0,
    CLIENT_PACKET,
    ERROR_PACKET,
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

// these indicate the data and more info about the packet type
typedef enum RequestType {

    SERVER_INFO = 0,
    SERVER_TEARDOWN,

    CLIENT_DISCONNET,

    REQ_GET_FILE,
    POST_SEND_FILE,
    
    REQ_AUTH_CLIENT,
    CLIENT_AUTH_DATA,
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

/*** PUBLIC REQUESTS FUNCTIONS ***/

extern u8 client_makeTestRequest (Client *client);

extern i8 client_file_get (Client *client, char *filename);
extern i8 client_file_send (Client *client, char *filename);

extern i8 client_game_createLobby (Client *owner, GameType gameType);
extern i8 client_game_joinLobby (Client *client, GameType gameType);
extern i8 client_game_leaveLobby (Client *client);
extern i8 client_game_destroyLobby (Client *client);

extern i8 client_game_startGame (Client *client);

/*** SERIALIZATION ***/

// cerver framework serialized data
#pragma region SERIALIZATION

// 17/11/2018 - send useful server info to the client trying to connect
typedef struct SServer {

    u8 useIpv6;  
    u8 protocol;            // we only support either tcp or udp
    u16 port;

    ServerType type;
    bool authRequired;      // authentication required by the server

} SServer;

// TODO: we need to create a more complex seralized data
// keep in mind that the admin can set a reference to the data and function
// that can handle specific serealization
// 03/11/2018 -> default auth data to use by default auth function
typedef struct DefAuthData {

    u32 code;

} DefAuthData;

typedef struct GameSettings {

	GameType gameType;

	u8 playerTimeout; 	// in seconds.
	u8 fps;

	u8 minPlayers;
	u8 maxPlayers;

	// duration?

} GameSettings;

// FIXME: players and a reference to the owner
// info that we need to send to the client about the lobby he is in
typedef struct SLobby {

    GameSettings settings;
    bool inGame;

    // FIXME: how do we want to send this info?
    // Player owner;               // how do we want to send which is the owner
    // Vector players;             // ecah client also needs to keep track of other players in the lobby

} SLobby;

#pragma endregion

#pragma region TESTING

// 02/11/2018 -- session token
typedef struct Token {

    char token[128];

} Token;

extern void client_sendAuthPacket (Client *client);

#pragma endregion

#endif