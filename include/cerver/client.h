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

// takes no argument and returns a value (int)
typedef u8 (*Func)(void);
// takes an argument and does not return a value
typedef void (*Action)(void *);
// takes an argument and returns a value (int)
typedef u8 (*delegate)(void *);

#define MAX_PORT_NUM            65535
#define MAX_UDP_PACKET_SIZE     65515

#define MAXSLEEP                60        // used for connection with exponential backoff (secs)     

#define MAX_PORT_NUM            65535

#define DEFAULT_USE_IPV6                0
#define DEFAULT_PROTOCOL                IPPROTO_TCP
#define DEFAULT_PORT                    7001

#define DEFAULT_MAX_CONNECTIONS         10
#define DEFAULT_POLL_TIMEOUT            2000
#define DEFAULT_PACKET_POOL_INIT        4

#define DEFAULT_THPOOL_INIT             4

#define DEFAULT_AUTH_CODE               0x4CA140FF

#pragma region ERRORS

struct _Client;

typedef enum ErrorType {

    ERR_SERVER_ERROR = 0,   // internal server error, like no memory

    ERR_CREATE_LOBBY = 1,
    ERR_JOIN_LOBBY,
    ERR_LEAVE_LOBBY,
    ERR_FIND_LOBBY,

    ERR_GAME_INIT,

    ERR_FAILED_AUTH,

 } ErrorType;

typedef struct ErrorData {

    ErrorType type;
    char msg[256];

} ErrorData;

extern ErrorData last_error;

extern void client_register_to_next_error (struct _Client *client, Action action, void *args); 
extern void client_register_to_error_type (struct _Client *client, Action action, void *args,
    ErrorType errorType);

#pragma endregion

#pragma region SERVER 

typedef enum ServerType {

    FILE_SERVER = 1,
    WEB_SERVER, 
    GAME_SERVER

} ServerType;

struct _Token;

// some useful info about the server we are connecting to
typedef struct Server {

    u8 useIpv6;  
    u8 protocol;
    u16 port; 

    char *ip;
    struct sockaddr_storage address;

    ServerType type;
    bool authRequired;

    struct _Token *token_data;

} Server;

#pragma endregion

#pragma region CLIENT

typedef struct Connection {

    i32 sock_fd;
    u8 useIpv6;  
    u8 protocol;
    u16 port; 

    bool async;
    bool blocking;          // sokcet fd is blocking?
    bool isConnected;       // is the socket connected?

    char *ip;
    struct sockaddr_storage address;

    Server *server;

    Action authentication;
    void *authData;
    Action successAuthAction;
    void *successAuthArgs;

} Connection;

struct _Client {

    Connection **active_connections;
    u8 n_active_connections;

    bool running;           // client poll is running

    struct pollfd fds[DEFAULT_MAX_CONNECTIONS];
    u16 nfds;                  // n of active fds in the pollfd array
    u32 pollTimeout;   

    threadpool thpool;

    Pool *packetPool;           //  packet info pool

    // only used in a game server
    // TODO: get details from the server when connecting to it...
    // TODO: move this from here to a server structure
    bool inLobby;           // is the client inside a lobby?
    bool isOwner;           // is the client the owner of the lobby?

    ErrorType errorType;
    Action errorAction;
    void *errorArgs;

};

typedef struct _Client Client;

typedef struct ClientConnection {

    Client *client;
    Connection *connection;

} ClientConnection;

/*** PUBLIC CLIENT FUNCTIONS ***/

extern Client *client_create (void);
extern u8 client_teardown (Client *client);

extern Connection *client_connection_new (u16 port, bool async);
extern Connection *client_make_new_connection (Client *client, const char *ip_address, u16 port,
    bool async);
extern u8 client_end_connection (Client *client, Connection *connection);

extern u8 client_connect_to_server (Client *client, Connection *con, const char *serverIp, u16 port,
    ServerType expectedType, Action send_auth_data, void *auth_data);
extern u8 client_disconnect_from_server (Client *client, Connection *connection);

extern void connection_set_send_auth_data (Connection *connection,
    Action send_auth_data, void *auth_data);
extern void connection_set_auth_data (Connection *connection, void *auth_data);
extern void connection_remove_auth_data (Connection *connection);
extern void connection_register_to_success_auth (Connection *connection, 
    Action succes_action, void *args);

#pragma endregion

#pragma region PACKETS

struct _PacketInfo {

    Client *client;
    Connection *connection;

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
    size_t packetSize;             // expected packet size

} PacketHeader;

extern void *client_generatePacket (PacketType packetType, size_t packetSize);
extern i8 client_sendPacket (Connection *connection, void *packet, size_t packetSize);

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

#pragma endregion

/*** PUBLIC REQUESTS FUNCTIONS ***/

extern u8 client_makeTestRequest (Client *client, Connection *connection);

extern u8 client_sendAuthPacket (Client *client, Connection *connection);

extern i8 client_file_get (Client *client, Connection *connection, const char *filename);
extern i8 client_file_send (Client *client, Connection *connection, const char *filename);

#pragma region GAME

// this is the same as in cerver
typedef enum GameType {

	ARCADE = 1,

} GameType;

typedef struct GameReqData {

    Client *client;
    Connection *connection;

    GameType game_type;

} GameReqData;

extern void *client_game_createLobby (Client *owner, Connection *connection, GameType gameType);
extern void *client_game_joinLobby (Client *client, Connection *connection, GameType gameType);
extern i8 client_game_leaveLobby (Client *client, Connection *connection);
extern i8 client_game_destroyLobby (Client *client, Connection *connection);

extern i8 client_game_startGame (Client *client, Connection *connection);

#pragma endregion

/*** SERIALIZATION ***/

// cerver framework serialized data
#pragma region SERIALIZATION

typedef struct SServer {

    u8 useIpv6;  
    u8 protocol;
    u16 port;

    ServerType type;
    bool authRequired;

} SServer;

typedef struct DefAuthData {

    u32 code;

} DefAuthData;

// session id - token
struct _Token {

    char token[65];

};

typedef struct _Token Token;

typedef struct GameSettings {

	GameType gameType;

	u8 playerTimeout; 	// in seconds.
	u8 fps;

	u8 minPlayers;
	u8 maxPlayers;

} GameSettings;

// info that we need to send to the client about the players
typedef struct Splayer {

	// TODO:
	// char name[64];

	// TODO: 
	// we need a way to add info about the players info for specific game
	// such as their race or level in blackrock

	bool owner;

} SPlayer;

// FIXME: players and a reference to the owner
// info that we need to send to the client about the lobby he is in
typedef struct SLobby {

    GameSettings settings;
    bool inGame;

    // FIXME: how do we want to send this info?
    // Player owner;               // how do we want to send which is the owner
    // Vector players;             // ecah client also needs to keep track of other players in the lobby

} SLobby;

typedef SLobby Lobby;

#pragma endregion

#endif