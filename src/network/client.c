#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>

#include <errno.h>

#include "blackrock.h"      // only used for type defs

#include "network/client.h"

#include "config.h"
#include "utils/log.h"

// this must be the same as in the server, if not, the packets will be ignored
ProtocolId PROTOCOL_ID = 0x4CA140FF; // randomly chosen
Version PROTOCOL_VERSION = { 1, 1 };

#pragma region PACKETS

void initPacketHeader (void *header, PacketType type) {

    PacketHeader h;
    h.protocolID = PROTOCOL_ID;
    h.protocolVersion = PROTOCOL_VERSION;
    h.packetType = type;

    memcpy (header, &h, sizeof (PacketHeader));

}

// generates a generic packet with the specified packet type
void *generatePacket (PacketType packetType) {

    size_t packetSize = sizeof (PacketHeader);

    void *packetBuffer = malloc (packetSize);
    void *begin = packetBuffer;
    char *end = begin; 

    PacketHeader *header = (PacketHeader *) end;
    end += sizeof (PacketHeader);
    initPacketHeader (header, packetType); 

    return begin;

}

// send a packet to the specified address
void sendPacket (i32 sock, void *begin, size_t packetSize, struct sockaddr_storage address) {

    ssize_t sentBytes = sendto (sock, (const char *) begin, packetSize, 0,
		       (struct sockaddr *) &address, sizeof (struct sockaddr_storage));

	if (sentBytes < 0 || (unsigned) sentBytes != packetSize)
        logMsg (stderr, ERROR, PACKET, "Failed to send packet!") ;

}

#pragma endregion

/*** CLIENT LOGIC ***/

// TODO: maybe set the socket to nonblocking, because it might interfere with other game processes
// TODO: but not all of our requests are async, when we want to create or join a game, 
// we need to wait for the sever reponse

#pragma region CLIENT

// TODO: 31/10/2018 -- we only handle the logic for a connection using tcp
// we need to add the logic to be able to send packets via udp

Client *newClient (Client *c) {

    Client *new = (Client *) malloc (sizeof (Client));

    if (c) {
        new->useIpv6 = c->useIpv6;
        new->protocol = c->protocol;
        new->port = c->port;
    }

    return new;

}

// loads the client values from the config file
u8 getClientCfgValues (Client *client, ConfigEntity *cfgEntity) {

    char *ipv6 = getEntityValue (cfgEntity, "ipv6");
    if (ipv6) {
        client->useIpv6 = atoi (ipv6);
        // if we have got an invalid value, the default is NOT to use ipv6
        if (client->useIpv6 != 0 || client->useIpv6 != 1) client->useIpv6 = 0;
    }
    // if we do not have a value, use the default
    else client->useIpv6 = 0;

    #ifdef CLIENT_DEBUG
    logMsg (stdout, DEBUG_MSG, CLIENT, createString ("Use IPv6: %i", client->useIpv6));
    #endif

    char *tcp = getEntityValue (cfgEntity, "tcp");
    if (tcp) {
        u8 usetcp = atoi (tcp);
        if (usetcp < 0 || usetcp > 1) {
            logMsg (stdout, WARNING, CLIENT, "Unknown protocol. Using default: tcp protocol");
            usetcp = 1;
        }

        if (usetcp) client->protocol = IPPROTO_TCP;
        else client->protocol = IPPROTO_UDP;

    }
    // set to default (tcp) if we don't found a value
    else {
        logMsg (stdout, WARNING, CLIENT, "No protocol found. Using default: tcp protocol");
        client->protocol = DEFAULT_PROTOCOL;
    }

    char *port = getEntityValue (cfgEntity, "port");
    if (port) {
        client->port = atoi (port);
        // check that we have a valid range, if not, set to default port
        if (client->port <= 0 || client->port >= MAX_PORT_NUM) {
            logMsg (stdout, WARNING, CLIENT, 
                createString ("Invalid port number. Setting port to default value: %i", DEFAULT_PORT));
            client->port = DEFAULT_PORT;
        }

        #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT, createString ("Listening on port: %i", client->port));
        #endif
    }
    // set to default port
    else {
        logMsg (stdout, WARNING, CLIENT, 
            createString ("No port found. Setting port to default value: %i", DEFAULT_PORT));
        client->port = DEFAULT_PORT;
    } 

    return 0;

}

// inits a client structure with the specified values
u8 client_init (Client *client, Config *cfg) {

    if (!client) {
        logMsg (stderr, ERROR, CLIENT, "Can't init a NULL client!");
        return 1;
    }

    #ifdef CLIENT_DEBUG
    logMsg (stdout, DEBUG_MSG, CLIENT, "Initializing client...");
    #endif

    if (cfg) {
        ConfigEntity *cfgEntity = getEntityWithId (cfg, 1);
        if (!cfgEntity) {
            logMsg (stderr, ERROR, CLIENT, "Problems with client config!");
            return 1;
        }

        #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT, "Setting client values from config.");
        #endif

        if (!getClientCfgValues (client, cfgEntity))
            logMsg (stdout, SUCCESS, CLIENT, "Done getting cfg client values.");
    }

    // init the client socket
    switch (client->protocol) {
        case IPPROTO_TCP: 
            client->clientSock = socket ((client->useIpv6 == 1 ? AF_INET6 : AF_INET), SOCK_STREAM, 0);
            break;
        case IPPROTO_UDP:
            client->clientSock = socket ((client->useIpv6 == 1 ? AF_INET6 : AF_INET), SOCK_DGRAM, 0);
            break;

        default: logMsg (stderr, ERROR, CLIENT, "Unkonw protocol type!"); return 1;
    }

    if (client->clientSock < 0) {
        logMsg (stderr, ERROR, CLIENT, "Failed to create client socket!");
        return 1;
    }

    #ifdef CLIENT_DEBUG
    logMsg (stdout, DEBUG_MSG, CLIENT, "Created client socket");
    #endif

    // FIXME: 31/10/2018 -- do we need to set the client socket to non blocking mode?
    // set the socket to non blocking mode
    /* if (!sock_setBlocking (server->serverSock, server->blocking)) {
        logMsg (stderr, ERROR, SERVER, "Failed to set server socket to non blocking mode!");
        // perror ("Error");
        close (server->serverSock);
        return 1;
    }

    else {
        #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, SERVER, "Server socket set to non blocking mode.");
        #endif
    } */

    return 0;   // at this point, the client is ready to connect to the server
    
}

// creates a new client
Client *client_create (Client *client) {

    // create a client with the requested parameters
    if (client) {
        Client *c = newClient (client);
        if (!client_init (c, NULL)) {
            logMsg (stdout, SUCCESS, CLIENT, "\nCreated a new client!\n");
            return c;
        }

        else {
            logMsg (stderr, ERROR, CLIENT, "Failed to init the client!");
            free (c);
            return NULL;
        }
    }

    // create the client from the default config file
    else {
        Config *clientConfig = parseConfigFile ("./config/client.cfg");
        if (!clientConfig) {
            logMsg (stderr, ERROR, NO_TYPE, "Problems loading client config!");
            return NULL;
        }

        else {
            Client *c = newClient (NULL);
            if (!client_init (client, clientConfig)) {
                logMsg (stdout, SUCCESS, CLIENT, "\nCreated a new client!\n");
                clearConfig (clientConfig);
                return c;
            }

            else {
                logMsg (stderr, ERROR, CLIENT, "Failed to init client!");
                clearConfig (clientConfig);
                free (c);
                return NULL;
            }
        }
    }

}

// TODO: does sleep also affects other processes?
// try to connect a client to an address (server) with exponential backoff
u8 connectRetry (Client *client, const struct sockaddr *address) {

    i32 numsec;
    for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
        if (!connect (client->clientSock, address, sizeof (struct sockaddr))) 
            return 0;   // the connection was successfull

        if (numsec <= MAXSLEEP / 2) sleep (numsec);
    }

    return 1;   // failed to connect to server after MAXSLEEP secs

}

// FIXME: get the correct ip of the server from the cfg file
// 31/10/2018 -- we are using 127.0.0.1
// connects a client to the specified server
u8 client_connectToServer (Client *client) {

    if (!client) {
        logMsg (stderr, ERROR, CLIENT, "Can't connect a NULL client to the server!");
        return 1;
    }

    if (client->isConnected) {
        logMsg (stdout, WARNING, CLIENT, "The client is already connected to the server.");
        return 1;
    }

    if (client->protocol != IPPROTO_TCP) {
        logMsg (stderr, ERROR, CLIENT, "Can't connect client. Wrong protocol!");
        return 1;
    }

    // set the address of the server 
    struct sockaddr_storage server;
    memset (&server, 0, sizeof (struct sockaddr_storage));
    
    if (client->useIpv6) {
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &server;
		addr->sin6_family = AF_INET6;
		addr->sin6_addr = in6addr_any;
		addr->sin6_port = htons (client->port);
	} 

    else {
		struct sockaddr_in *addr = (struct sockaddr_in *) &server;
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = INADDR_ANY;
		addr->sin_port = htons (client->port);
	} 

    // try to connect to the server with exponential backoff
    if (!connectRetry (client, (const struct sockaddr *) &server)) {
        logMsg (stdout, SUCCESS, CLIENT, "Connected to server!");

        // we expect a welcome message from the server...
        char serverResponse [256];
        recv (client->clientSock, &serverResponse, sizeof (serverResponse), 0);
        fprintf (stdout, "%s\n", serverResponse);

        client->isConnected = true;

        return 0;
    }

    else {
        logMsg (stderr, ERROR, CLIENT, "Failed to connect to server!");
        // TODO: does this works properly?
        fprintf (stderr, "%s\n", strerror (errno));
        return 1;
    }

}

// FIXME: send a disconnect packet to the server
u8 client_disconnectFromServer (Client *client) {

    if (!client) {
        logMsg (stderr, ERROR, CLIENT, "Can't disconnect a NULL client from the server!");
        return 1;
    }

    if (!client->isConnected) {
        logMsg (stdout, WARNING, CLIENT, "The client is not connedted to ther server.");
        return 1;
    }

    if (!client->protocol != IPPROTO_TCP) {
        logMsg (stderr, ERROR, CLIENT, "Can't disconnect client. Wrong protocol!");
        return 1;
    }

    // TODO: send a packet to the server so that it knows we will disconnect

    close (client->clientSock);
    client->isConnected = false;

    return 0;

}

#pragma endregion

/*** MULTIPLAYER ***/

// Here goes all the multiplayer logic and requests we need

#pragma region MULTIPLAYER

// FIXME: Move this from here
typedef enum GameType {

	ARCADE = 0,

} GameType;

// send a valid client authentication
u8 client_authentication () {}

// request to create a new multiplayer game
u8 client_createLobby (Client *owner, GameType gameType) {

    if (!owner) {
        logMsg (stderr, ERROR, GAME, "A NULL client can't create a lobby!");
        return 1;
    }

    

}

// request to join an on going game
u8 client_joinLobby (Client *owner, GameType gameType) {}

// the owner of the lobby can request to init the game
u8 client_startGame (Client *owner) {}

// request leaderboard data
u8 client_getLeaderBoard () {}

// request to send new leaderboard data
u8 client_sendLeaderBoard () {}

#pragma endregion