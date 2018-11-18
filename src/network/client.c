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
#include "utils/myUtils.h"

/*** VALUES ***/

// these 2 are used to manage the packets
ProtocolId PROTOCOL_ID = 0x4CA140FF; // randomly chosen
Version PROTOCOL_VERSION = { 1, 1 };

// FIXME: don't forget to delete this when we disconnect from the server!!
Server *serverInfo = NULL;

#pragma region PACKETS

// check for packets with bad size, protocol, version, etc
u8 checkPacket (size_t packetSize, char *packetData, PacketType expectedType) {

    if (packetSize < sizeof (PacketHeader)) {
        #ifdef CLIENT_DEBUG
        logMsg (stderr, WARNING, PACKET, "Recieved a to small packet!");
        #endif
        return 1;
    } 

    PacketHeader *header = (PacketHeader *) packetData;

    if (header->protocolID != PROTOCOL_ID) {
        #ifdef CLIENT_DEBUG
        logMsg (stdout, WARNING, PACKET, "Packet with unknown protocol ID.");
        #endif
        return 1;
    }

    Version version = header->protocolVersion;
    if (version.major != PROTOCOL_VERSION.major) {
        #ifdef CLIENT_DEBUG
        logMsg (stdout, WARNING, PACKET, "Packet with incompatible version.");
        #endif
        return 1;
    }

    // compare the size we got from recv () against what is the expected packet size
    // that the client created 
    if (packetSize != header->packetSize) {
        #ifdef CLIENT_DEBUG
        logMsg (stdout, WARNING, PACKET, "Recv packet size doesn't match header size.");
        #endif
        return 1;
    }

    if (expectedType != DONT_CHECK_TYPE) {
        // check if the packet is of the expected type
        if (header->packetType != expectedType) {
            #ifdef CLIENT_DEBUG
            logMsg (stdout, WARNING, PACKET, "Packet doesn't match expected type.");
            #endif
            return 1;
        }
    }

    return 0;   // packet is fine

}

void initPacketHeader (void *header, PacketType type, u32 packetSize) {

    PacketHeader h;
    h.protocolID = PROTOCOL_ID;
    h.protocolVersion = PROTOCOL_VERSION;
    h.packetType = type;
    h.packetSize = packetSize;

    memcpy (header, &h, sizeof (PacketHeader));

}

// generates a generic packet with the specified packet type
void *generatePacket (PacketType packetType, size_t packetSize) {

    size_t packet_size;
    if (packetSize > 0) packet_size = packetSize;
    else packet_size = sizeof (PacketHeader);

    PacketHeader *header = (PacketHeader *) malloc (packet_size);
    initPacketHeader (header, packetType, packet_size); 

    return header;

}

// TODO: 06/11/2018 -- test this!
i8 udp_sendPacket (Client *client, const void *begin, size_t packetSize, 
    const struct sockaddr_storage address) {

    ssize_t sent;
    const void *p = begin;
    while (packetSize > 0) {
        sent = sendto (client->clientSock, begin, packetSize, 0, 
            (const struct sockaddr *) &address, sizeof (struct sockaddr_storage));
        if (sent <= 0) return -1;
        p += sent;
        packetSize -= sent;
    }

    return 0;

}

i8 tcp_sendPacket (i32 socket_fd, const void *begin, size_t packetSize, int flags) {

    ssize_t sent;
    const void *p = begin;
    while (packetSize > 0) {
        sent = send (socket_fd, p, packetSize, flags);
        if (sent <= 0) return -1;
        p += sent;
        packetSize -= sent;
    }

    return 0;

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

// TODO: maybe later we can use this to connect to other clients directly?
// 18/11/2018 - adding support for async request and responses using a similar logic
// as in the server. We only support poll to be used when connected to the server.
u8 client_poll (Client *client) {

    if (!client) {
        logMsg (stderr, ERROR, SERVER, "Can't poll on a NULL client!");
        return 1;
    }

    // TODO: do we also want to connect to other clients?
    // i32 serverSocket;   
	// struct sockaddr_storage serverAddress;
	// memset (&serverAddress, 0, sizeof (struct sockaddr_storage));
    // socklen_t sockLen = sizeof (struct sockaddr_storage);

    ssize_t rc;                                   // retval from recv -> size of buffer
    char packetBuffer[MAX_UDP_PACKET_SIZE];       // buffer for data recieved from fd
    PacketInfo *info = NULL;

    int poll_retval;    // ret val from poll function
    int currfds;        // copy of n active server poll fds

    int newfd;          // fd of new connection

    #ifdef CLIENT_DEBUG
        logMsg (stdout, SUCCESS, CLIENT, "Client has started!");
        logMsg (stdout, DEBUG_MSG, CLIENT, "Waiting for connections...");
    #endif

    // 18/11/2018 - we only want to communicate with the server
    while (client->isConnected) {
        poll_retval = poll (client->fds, client->nfds, client->pollTimeout);

        // poll failed
        if (poll_retval < 0) {
            logMsg (stderr, ERROR, CLIENT, "Poll failed!");
            perror ("Error");
            client->isConnected;
            break;
        }

        // if poll has timed out, just continue to the next loop... 
        if (poll_retval == 0) {
            #ifdef DEBUG
                logMsg (stdout, DEBUG_MSG, CLIENT, "Poll timeout.");
            #endif
            continue;
        }

        // one or more fd(s) are readable, need to determine which ones they are
        // currfds = server->nfds;
        for (u8 i = 0; i < 2; i++) {
            if (client->fds[i].revents == 0) continue;

            // FIXME: how to hanlde an unexpected result??
            if (client->fds[i].revents != POLLIN) {
                // TODO: log more detailed info about the fd, or client, etc
                // printf("  Error! revents = %d\n", fds[i].revents);
                logMsg (stderr, ERROR, CLIENT, "Unexpected poll result!");
            }

            // 18/11/2018 -- we only want to be connected to the server!
            // listening fd is readable (client socket)
            if (client->fds[i].fd == client->clientSock) {
                #ifdef CLIENT_DEBUG
                    logMsg (stdout, WARNING, CLIETN, "Some tried to connect to this client.");
                #endif
            }

            // not the clitn socket, so the server fd must be readable
            else {
                // recive all incoming data from this socket
                // TODO: 03/11/2018 - add support for multiple reads to the socket
                // what happens if my buffer isn't enough, for example a larger file?
                do {
                    rc = recv (client->fds[i].fd, packetBuffer, sizeof (packetBuffer), 0);
                    
                    // recv error - no more data to read
                    if (rc < 0) {
                        if (errno != EWOULDBLOCK) {
                            logMsg (stderr, ERROR, CLIENT, "Recv failed!");
                            perror ("Error:");
                        }

                        break;  // there is no more data to handle
                    }

                    if (rc == 0) {
                        // man recv -> steam socket perfomed an orderly shutdown
                        // but in dgram it might mean something?
                        // 03/11/2018 -- we just ignore the packet or whatever
                        break;
                    }

                    // FIXME:
                    // info = newPacketInfo (server, 
                    //     getClientBySock (server->clients, server->fds[i].fd), packetBuffer, rc);

                    // thpool_add_work (server->thpool, (void *) handlePacket, info);
                } while (true);
            }

        }
    }

}


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

// FIXME: check for the correct values to make this a flexible framework!!
// check to see if we are connecting to the right server
u8 checkServerInfo (Server *server) {

    if (server) {
        if (!server->isRunning) return 1;
        // if (server->type != GAME_SERVER)
    }

}

// FIXME: get the correct ip of the server from the cfg file
// 31/10/2018 -- we are using 127.0.0.1
// connects a client to the specified server
u8 client_connectToServer (Client *client) {

    if (!client) {
        // init a new client
        client = client_create (NULL);
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

        // FIXME: 18/11/2018 -- 17:34 -- we need to take into account the client poll

        // we expect a welcome message from the server -> a server info packet
        /* char serverResponse[2048];
        ssize_t rc = recv (client->clientSock, serverResponse, sizeof (serverResponse), 0);
        if (rc > 0) {
            // process packet
            if (!checkPacket (rc, serverResponse, SERVER_PACKET)) {
                Server *serverData = (Server *) (serverResponse + sizeof (PacketHeader));

                // TODO: check server info to see if we are making a proper connection
                if (serverData->authRequired) {
                    // FIXME: we need to send the server the correct authentication info!!

                    // we expect a response from the server
                }

                // make a copy of the server data
                if (serverInfo) free (serverInfo);

                serverInfo = (Server *) malloc (sizeof (Server));
                serverInfo->authRequired = serverData->authRequired;
                serverInfo->isRunning = serverData->isRunning;
                serverInfo->port = serverData->port;
                serverInfo->protocol = serverData->protocol;
                serverInfo->type = serverData->type;
                serverInfo->useIpv6 = serverData->useIpv6;

                client->isConnected = true;
                logMsg (stdout, SUCCESS, NO_TYPE, "Connected to server!");

                return 0;   // success connecting to server
            }

            // TODO: what to do next? --> retry the connection?
            else logMsg (stderr, ERROR, PACKET, "Got an invalid server info packet!");
        } */
    }

    else {
        logMsg (stderr, ERROR, CLIENT, "Failed to connect to server!");
        // TODO: does this works properly?
        fprintf (stderr, "%s\n", strerror (errno));
        return 1;
    }

}

// disconnect from the server
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

    // send a disconnect packet to the server
    if (client->isGameServer) {
        if (client->inLobby) {
            u8 client_leaveLobby (Client *client);
            client_leaveLobby (client);
        }
    }

    close (client->clientSock);
    client->isConnected = false;

    return 0;

}

#pragma endregion

/*** REQUESTS ***/

// These are the requests that we send to the server and we expect a response 

#pragma region REQUESTS

void *generateRequest (PacketType packetType, RequestType reqType) {

    size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
    void *begin = generatePacket (packetType, packetSize);
    char *end = begin + sizeof (PacketHeader); 

    RequestData *reqdata = (RequestData *) end;
    reqdata->type = reqType;

    return begin;

}

// send a valid client authentication
u8 client_authentication () {}

// FIXME: don't forget to make the cient the owner
// request to create a new multiplayer game
u8 client_createLobby (Client *owner, GameType gameType) {

    if (!owner) {
        logMsg (stderr, ERROR, GAME, "A NULL client can't create a lobby!");
        return 1;
    }

    if (owner) {
        // create & send a join lobby req packet to the server
        size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
        void *req = generateRequest (GAME_PACKET, LOBBY_CREATE);

        if (req) {
            tcp_sendPacket (owner->clientSock, req, packetSize, 0);

            // FIXME: we need to wait for the respponse of the server

            return 0;
        }

        else logMsg (stderr, ERROR, PACKET, "Failed to generate create lobby request packet!");
    }

    return 1;

}

// FIXME:
// request to join an on going game
u8 client_joinLobby (Client *owner, GameType gameType) {

    if (owner) {
        // create & send a join lobby req packet to the server
        size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
        void *req = generateRequest (GAME_PACKET, LOBBY_JOIN);

        if (req) {
            
            tcp_sendPacket (owner->clientSock, req, packetSize, 0);

            // FIXME: we need to wait for the server response

            return 0;
        }

        else logMsg (stderr, ERROR, PACKET, "Failed to generate join lobby request packet!");
    }

    return 1;

}

// TODO: check that we are making valid requests to a game server
// request the server to leave the lobby
u8 client_leaveLobby (Client *client) {

    if (client) {
        if (client->inLobby) {
            // create & send a leave lobby req packet to the server
            size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
            void *req = generateRequest (GAME_PACKET, LOBBY_LEAVE);

            if (req) {
                tcp_sendPacket (client->clientSock, req, packetSize, 0);
                return 0;
            } 

            else logMsg (stderr, ERROR, PACKET, "Failed to generate leave lobby request packet!");
        }
    }

    return 1;

}

// TODO: check that we are making valid requests to a game server
// request to destroy the current lobby, only if the client is the owner
u8 client_destroyLobby (Client *client) {

    if (client) {
        if (client->inLobby) {
            // create & send a leave lobby req packet to the server
            size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
            void *req = generateRequest (GAME_PACKET, LOBBY_DESTROY);

            if (req) {
                tcp_sendPacket (client->clientSock, req, packetSize, 0);

                // TODO: do we need to wait for a server response?

                return 0;
            } 

            else logMsg (stderr, ERROR, PACKET, "Failed to generate destroy lobby request packet!");
        }
    }

    return 1;

}

// TODO: check that we are making valid requests to a game server
// the owner of the lobby can request to init the game
u8 client_startGame (Client *owner) {

    if (owner) {
        if (owner->inLobby && owner->isOwner) {
            // create & send a leave lobby req packet to the server
            size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
            void *req = generateRequest (GAME_PACKET, GAME_INIT);

            if (req) {
                tcp_sendPacket (owner->clientSock, req, packetSize, 0);

                // TODO: do we need to wait for a server response?

                return 0;
            } 

            else logMsg (stderr, ERROR, PACKET, "Failed to generate destroy lobby request packet!");
        }
    }

    return 1;

}

// request leaderboard data
u8 client_getLeaderBoard () {}

// request to send new leaderboard data
u8 client_sendLeaderBoard () {}

#pragma endregion