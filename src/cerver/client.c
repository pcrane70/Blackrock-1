#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

#include "cerver/client.h"

#include "utils/objectPool.h"
#include "utils/config.h"
#include "utils/log.h"
#include "utils/myUtils.h"

/*** VALUES ***/

// these 2 are used to manage the packets
ProtocolId PROTOCOL_ID = 0x4CA140FF; // randomly chosen
Version PROTOCOL_VERSION = { 1, 1 };

// FIXME: don't forget to delete this when we disconnect from the server!!
Server *serverInfo = NULL;

/*** NETWORKING ***/

#pragma region NETWORKING 

// enable/disable blocking on a socket
// true on success, false if there was an eroror
bool sock_setBlocking (int32_t fd, bool isBlocking) {

    if (fd < 0) return false;

    int flags = fcntl (fd, F_GETFL, 0);
    if (flags == -1) return false;
    // flags = isBlocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);   // original
    flags = isBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return (fcntl (fd, F_SETFL, flags) == 0) ? true : false;

}

#pragma endregion

/*** PACKETS ***/

#pragma region PACKETS

PacketInfo *newPacketInfo (Client *client, char *packetData, size_t packetSize) {

    PacketInfo *p = NULL;

    /* if (client->packetPool) {
        if (POOL_SIZE (client->packetPool) > 0) {
            p = pool_pop (client->packetPool);
            if (!p) p = (PacketInfo *) malloc (sizeof (PacketInfo));
            else {
                // printf ("\nGot a packet info from the pool!\n");

                if (p->packetData) free (p->packetData);
            }
            // else if (p->packetData) free (p->packetData);
        }
    }

    else {
        p = (PacketInfo *) malloc (sizeof (PacketInfo));
        p->packetData = NULL;
    } */

    p = (PacketInfo *) malloc (sizeof (PacketInfo));

    if (p) {
        p->client = client;
        p->packetSize = packetSize;
        p->packetData = NULL;
        
        // copy the contents from the entry buffer to the packet info
        if (!p->packetData)
            p->packetData = (char *) calloc (MAX_UDP_PACKET_SIZE, sizeof (char));

        memcpy (p->packetData, packetData, MAX_UDP_PACKET_SIZE);
    }

    return p;

}

// used to destroy remaining packet info in the pools
void destroyPacketInfo (void *data) {

    if (data) {
        PacketInfo *packet = (PacketInfo *) data;
        packet->client = NULL;
        if (packet->packetData) free (packet->packetData);
    }

}

// check for packets with bad size, protocol, version, etc
u8 checkPacket (size_t packetSize, char *packetData, PacketType expectedType) {

    printf ("checkPacket ()\n");

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

void initPacketHeader (PacketHeader *header, PacketType type, u32 packetSize) {

    header->protocolID = PROTOCOL_ID;
    header->protocolVersion = PROTOCOL_VERSION;
    header->packetType = type;
    header->packetSize = packetSize;

    /* PacketHeader h;
    h.protocolID = PROTOCOL_ID;
    h.protocolVersion = PROTOCOL_VERSION;
    h.packetType = type;
    h.packetSize = (u32) packetSize;

    memcpy (header, &h, sizeof (PacketHeader)); */

}

// generates a generic packet with the specified packet type
void *generatePacket (PacketType packetType, size_t packetSize) {

    size_t packet_size;
    if (packetSize > 0) packet_size = packetSize;
    else packet_size = sizeof (PacketHeader);

    PacketHeader *header = (PacketHeader *) malloc (packet_size);
    initPacketHeader (header, packetType, (u32) packet_size); 

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

/*** CONNECTION HANDLER ***/

#pragma region CONNECTION HANDLER

void server_handlePacket (PacketInfo *packet);

// called with the th pool to handle a new packet
void handlePacket (void *data) {

    if (data) {
        PacketInfo *packet = (PacketInfo *) data;

        if (!checkPacket (packet->packetSize, packet->packetData, DONT_CHECK_TYPE))  {
            PacketHeader *header = (PacketHeader *) packet->packetData;

            switch (header->packetType) {
                // handles a packet with server info
                case SERVER_PACKET: server_handlePacket (packet); break;

                // handles an error from the server
                case ERROR_PACKET: break;

                // handles authentication packets
                case AUTHENTICATION: break;

                // handles a request made from the server
                case REQUEST: break;

                // handle a game packet sent from the server
                case GAME_PACKET: break;

                case TEST_PACKET: 
                    logMsg (stdout, TEST, NO_TYPE, "Got a successful test packet!"); 
                    break;

                default: 
                    #ifdef CLIENT_DEBUG
                        logMsg (stderr, WARNING, PACKET, "Got a packet of incompatible type.");
                    #endif 
                    break;
            }
        }

        // no matter the case, we always send the packet info to the client pool here!
        pool_push (packet->client->packetPool, packet);
    }

}

// recive all incoming data from this socket
// what happens if my buffer isn't enough, for example a larger file?
// TODO: 03/11/2018 - add support for multiple reads to the socket
void client_recieve (Client *client, i32 fd) {

    ssize_t rc;
    char packetBuffer[MAX_UDP_PACKET_SIZE];
    PacketInfo *info = NULL;

    do {
        rc = recv (fd, packetBuffer, sizeof (packetBuffer), 0);
        
        // TODO: just close the connection
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

        info = newPacketInfo (client, packetBuffer, rc);
        thpool_add_work (client->thpool, (void *) handlePacket, info);
    } while (true);

}

// TODO: 18/11/2018 -- 22:17 -- maybe we can't have the same logic as in the server,
// because the client socket is the one that connect to the server!!!
// so we might need the poll structure to be one levele above and each time have a new connection, 
// we need to create a new client structure!!!
// but for now i think we can handle one connection just for testing...

// FIXME: add support for multiple connections to multiple servers at the same time
// TODO: maybe later we can use this to connect to other clients directly?
// 18/11/2018 - adding support for async request and responses using a similar logic
// as in the server. We only support poll to be used when connected to the server.
u8 client_poll (void *data) {

    if (!data) {
        logMsg (stderr, ERROR, SERVER, "Can't poll on a NULL client!");
        return 1;
    }

    Client *client = (Client *) data;

    // TODO: do we also want to connect to other clients?
    // i32 serverSocket;   
	// struct sockaddr_storage serverAddress;
	// memset (&serverAddress, 0, sizeof (struct sockaddr_storage));
    // socklen_t sockLen = sizeof (struct sockaddr_storage);

    int poll_retval;    // ret val from poll function
    int newfd;          // fd of new connection

    #ifdef CLIENT_DEBUG
        logMsg (stdout, SUCCESS, CLIENT, "Client has started!");
        logMsg (stdout, DEBUG_MSG, CLIENT, "Waiting for connections...");
    #endif

    // 18/11/2018 - we only want to communicate with the server
    while (client->running) {
        poll_retval = poll (client->fds, 2, client->pollTimeout);

        // poll failed
        // FIXME: disconnect the client from the server
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

            // FIXME: close the connection
            if (client->fds[i].revents != POLLIN) {
                logMsg (stderr, ERROR, CLIENT, "Unexpected poll result!");
                continue;  
            }

            // 18/11/2018 -- we only want to be connected to the server!
            // listening fd is readable (client socket)
            /* if (client->fds[i].fd == client->clientSock) {
                #ifdef CLIENT_DEBUG
                    logMsg (stdout, WARNING, CLIENT, "Some tried to connect to this client.");
                #endif
            }

            // someone sent us data
            else client_recieve (client, client->fds[i].fd); */

            client_recieve (client, client->fds[i].fd);
        }
    }

    #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT, "Poll end!");
    #endif

}

/*** SERVER ***/

#pragma region SERVER

// FIXME: check for the correct values to make this a flexible framework!!
// check to see if we are connecting to the right server
u8 checkServerInfo (Server *server) {

    if (server) {
        if (!server->isRunning) return 1;
        // if (server->type != GAME_SERVER)
    }

}

/* u8 useIpv6;  
u8 protocol;            // we only support either tcp or udp
u16 port; 

bool isRunning;         // the server is recieving and/or sending packets

ServerType type;
bool authRequired;      // authentication required by the server */


// FIXME: handle all the values!!
// TODO: this can have a lot of potential!!
// compare the info the server sent us with the one we expected 
// and ajust our connection values if necessary
void server_handleServerInfo (Server *curr_info, SServer *recv_info) {

    if (curr_info && recv_info) {
        curr_info->type = recv_info->type;
        #ifdef CLIENT_DEBUG
        switch (curr_info->type) {
            case GAME_SERVER: logMsg (stdout, DEBUG_MSG, SERVER, "Connected to a game server."); break;
            default: 
            logMsg (stdout, WARNING, SERVER, 
                createString ("Connected to a server of an unknown type: %i.", recv_info->type)); 
            break;
        }
        #endif
    }

}

void server_handlePacket (PacketInfo *packet) {

    RequestData *reqdata = (RequestData *) packet->packetData + sizeof (PacketHeader);
    switch (reqdata->type) {
        case SERVER_INFO: {
            #ifdef CLIENT_DEBUG
                logMsg (stdout, DEBUG_MSG, SERVER, "Recieved a server info packet.");
            #endif
            SServer *serverInfo = 
                (SServer *) (packet->packetData + sizeof (PacketHeader) + sizeof (RequestData));
            server_handleServerInfo (packet->client->connectionServer, serverInfo);
        } break;
        
        // FIXME: disconnect from the server
        case SERVER_TEARDOWN: logMsg (stdout, WARNING, SERVER, "--> Server teardown!! <--"); break;
        default: break;
    }

}

// TODO: we need to have a server structure that has the expected parameters of the server
// that we want to connect to...
// FIXME: 
void handle_serverInfo (PacketInfo *packet) {

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

#pragma endregion

#pragma endregion

/*** CLIENT LOGIC ***/

// TODO: 22/11/2018 - we can add a restart client function similar to restart server

// TODO: 31/10/2018 -- we only handle the logic for a connection using tcp
// we need to add the logic to be able to send packets via udp

#pragma region CLIENT

Client *newClient (Client *c) {

    Client *newClient = (Client *) malloc (sizeof (Client));

    if (newClient) {
        // create a client with the requested parameters
        if (c) {
            newClient->useIpv6 = c->useIpv6;
            newClient->protocol = c->protocol;
            newClient->port = c->port;
        }

        // set the values to default
        else {
            newClient->useIpv6 = DEFAULT_USE_IPV6;
            newClient->protocol = DEFAULT_PROTOCOL;
            newClient->port = DEFAULT_PORT;
        }

        // set default values
        newClient->isConnected = false;
        newClient->blocking = true;
        newClient->running = false;

        newClient->inLobby = false;
        newClient->isOwner = false;
    }

    return newClient;

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

// init client data structues inside client such as pool and poll
void initClientData (Client *client) {

    if (client) {
        // init the poll strcuture and add the client socket as the first one
        memset (client->fds, 0, sizeof (client->fds));
        
        client->nfds = 0;
        client->fds[0].fd = client->clientSock;
        client->fds[0].events = POLLIN;
        client->nfds++; 

        // init the client's packet info pool with some memebers in it
        client->packetPool = pool_init (destroyPacketInfo);
        if (client->packetPool) 
            for (int i = 0; i < DEFAULT_PACKET_POOL_INIT; i++)
                pool_push (client->packetPool, malloc (sizeof (PacketInfo)));
        
        else logMsg (stderr, ERROR, CLIENT, "Failed to init client's packet info pool!");

        client->thpool = thpool_init (DEFAULT_THPOOL_INIT);
    }

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
            logMsg (stdout, DEBUG_MSG, CLIENT, "Setting client values from config...");
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

    // set the socket to non blocking mode
    if (!sock_setBlocking (client->clientSock, client->blocking)) {
        logMsg (stderr, ERROR, CLIENT, "Failed to set client socket to non blocking mode!");
        close (client->clientSock);
        return 1;
    }

    else {
        client->blocking = false;
        #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT, "Client socket set to non blocking mode.");
        #endif
    }

    client->pollTimeout = DEFAULT_POLL_TIMEOUT;

    return 0;
    
}

// TODO: how can we handle multiple connections?
// prepare the client to listen and send info
u8 client_start (Client *client) {

    if (!client->running) {
        client->running = true;

        // start the client poll in its own thread so that we can recieve packets
        /* if (client->thpool) {
            thpool_add_work (client->thpool, (void *) client_poll, client);
            return 0;
        } 

        else logMsg (stderr, ERROR, CLIENT, "Client doesn't have a reference to a thpool!"); */
    }

    else logMsg (stderr, ERROR, CLIENT, "Can't start the client. It is already running!");

    return 1;

}

// creates a new client
Client *client_create (Client *client) {

    // create a client with the requested parameters
    if (client) {
        Client *c = newClient (client);
        if (!client_init (c, NULL)) {
            initClientData (c);
            logMsg (stdout, SUCCESS, CLIENT, "Created a new client!");
            return c;
        }

        else {
            logMsg (stderr, ERROR, CLIENT, "Failed to init the client!");
            free (c);
        }
    }

    // create the client from the default config file
    else {
        Config *clientConfig = parseConfigFile ("./config/client.cfg");
        if (!clientConfig) 
            logMsg (stderr, ERROR, NO_TYPE, "Problems loading client config!");

        else {
            Client *c = newClient (NULL);
            if (!client_init (c, clientConfig)) {
                initClientData (c);
                logMsg (stdout, SUCCESS, CLIENT, "Created a new client!");
                clearConfig (clientConfig);
                return c;
            }

            else {
                logMsg (stderr, ERROR, CLIENT, "Failed to init client!");
                clearConfig (clientConfig);
                free (c);
            }
        }
    }

    return NULL;

}

// check that the client has the correct values
u8 client_check (Client *client) {

    if (!client) {
        logMsg (stderr, ERROR, CLIENT, "A NULL client can't connect to a server!");
        return 1;
    }

    if (!client->running) {
        logMsg (stderr, ERROR, CLIENT, "Need to start client first!");
        return 1;
    }

    if (client->protocol != IPPROTO_TCP) {
        logMsg (stderr, ERROR, CLIENT, "Can't connect client. Wrong protocol!");
        return 1;
    }

    return 0;

}

// try to connect a client to an address (server) with exponential backoff
u8 connectRetry (Client *client, const struct sockaddr_storage address) {

    i32 numsec;
    for (numsec = 2; numsec <= MAXSLEEP; numsec <<= 1) {
        if (!connect (client->clientSock, 
            (const struct sockaddr *) &address, 
            sizeof (struct sockaddr))) 
            return 0;   // the connection was successfull

        if (numsec <= MAXSLEEP / 2) sleep (numsec);
    } 

    return 1;   // failed to connect to server after MAXSLEEP secs

}

// FIXME: add support for multiple connections to multiple servers at the same time
// TODO: add support for ipv6 connections
// connects the client to the specified server
u8 client_connectToServer (Client *client, char *serverIp) {

    if (!client_check (client) && !client->isConnected) {
        Server *server = (Server *) malloc (sizeof (Server));

        if (!serverIp) {
            logMsg (stderr, ERROR, SERVER, "Failed to connect to server, no ip provided.");
            return 1;
        }

        // copy the new ip to the client server data
        else {
            server->ip = (char *) calloc (strlen (serverIp) + 1, sizeof (char));
            strcpy (server->ip, serverIp);
        } 

        // set the address of the server 
        memset (&server->address, 0, sizeof (struct sockaddr_storage));

        if (client->useIpv6) {
            struct sockaddr_in6 *addr = 
                (struct sockaddr_in6 *) &server->address;
            addr->sin6_family = AF_INET6;
            // addr->sin6_addr = inet;      
            addr->sin6_port = htons (client->port);
        } 

        else {
            struct sockaddr_in *addr = 
                (struct sockaddr_in *) &server->address;
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = inet_addr (server->ip);
            addr->sin_port = htons (client->port);
        } 

        // try to connect to the server with exponential backoff
        if (!connectRetry (client, server->address)) {
            // start the client poll in its own thread so that we can recieve packets
            if (client->thpool)
                thpool_add_work (client->thpool, (void *) client_poll, client);

            else logMsg (stderr, ERROR, CLIENT, "Client doesn't have a reference to a thpool!");

            client->connectionServer = server;
            client->isConnected = true;
            logMsg (stdout, SUCCESS, CLIENT, "Connected to server!"); 

            return 0;
        } 
    }

    logMsg (stderr, ERROR, CLIENT, "Failed to connect to server!");
    return 1;

}

void *generateRequest (PacketType packetType, RequestType reqType);

// FIXME: add support for multiple connections to multiple servers at the same time
// disconnect from the server
u8 client_disconnectFromServer (Client *client) {

    if (!client_check (client) && client->isConnected) {
        if (client->connectionServer->type == GAME_SERVER) {
            if (client->inLobby) {
                u8 client_game_leaveLobby (Client *client);
                client_game_leaveLobby (client);
            }
        }

        // send a disconnect packet to the server
        size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
        void *pack = generateRequest (CLIENT_PACKET, CLIENT_DISCONNET);
        if (pack) {
            if (tcp_sendPacket (client->clientSock, pack, packetSize, 0) >= 0)
                logMsg (stdout, DEBUG_MSG, PACKET, "Sent client disconnect packet.");

            else logMsg (stderr, ERROR, PACKET, "Failed to send client disconnect packet.");

            free (pack);
        }

        client->running = false;

        if (!close (client->clientSock))
            logMsg (stdout, SUCCESS, CLIENT, "Client socket has been closed!");

        client->isConnected = false;

        logMsg (stdout, SUCCESS, CLIENT, "Client disconnected from server!");

        return 0;
    }

    logMsg (stderr, ERROR, CLIENT, "Failed to disconnect client from server!");

    return 1;

}

// stop any on going process and destroy
u8 client_teardown (Client *client) {

    if (client) {
        if (client->isConnected) client_disconnectFromServer (client);

        client->running = false;
        #ifdef CLIENT_DEBUG
            logMsg (stdout, DEBUG_MSG, CLIENT,
                createString ("Active threads in thpool: %i", 
                thpool_num_threads_working (client->thpool)));
        #endif
        // if (client->thpool) {
        //     thpool_destroy (client->thpool);
        //     #ifdef CLIENT_DEBUG
        //         logMsg (stdout, SUCCESS, CLIENT, "Client thpool got destroyed!");
        //     #endif
        // }  
        free (client->thpool);

        // if (client->packetPool) pool_clear (client->packetPool); 

        // free (client);

        return 0;
    }

    return 1;

}

#pragma endregion

// TODO: need to refactor this code!
/*** REQUESTS ***/

// These are the requests that we send to the server and we expect a response 

void *generateRequest (PacketType packetType, RequestType reqType) {

    size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
    void *begin = generatePacket (packetType, packetSize);
    char *end = begin + sizeof (PacketHeader); 

    RequestData *reqdata = (RequestData *) end;
    reqdata->type = reqType;

    return begin;

}

// TODO: 
// send a valid client authentication
u8 client_authentication () {}

u8 client_makeTestRequest (Client *client) {

    if (client && client->isConnected) {
        size_t packetSize = sizeof (PacketHeader);
        void *req = generatePacket (TEST_PACKET, packetSize);
        if (req) {
            if (tcp_sendPacket (client->clientSock, req, packetSize, 0) < 0)
                logMsg (stderr, ERROR, PACKET, "Failed to send test packet!");
            free (req);
        }
    }

}

/*** FILE SERVER ***/

#pragma region FILE SERVER

// TODO:
// request a file from the server
u8 client_file_get () {}

// TODO:
// send a file to the server
u8 client_file_send () {}

#pragma endregion

/*** GAME SERVER ***/

#pragma region GAME SERVER

// FIXME: don't forget to make the cient the owner
// request to create a new multiplayer game
u8 client_game_createLobby (Client *owner, GameType gameType) {

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
u8 client_game_joinLobby (Client *owner, GameType gameType) {

    if (owner) {
        // create & send a join lobby req packet to the server
        size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
        void *req = generateRequest (GAME_PACKET, LOBBY_JOIN);

        if (req) {
            
            tcp_sendPacket (owner->clientSock, req, packetSize, 0);

            // FIXME: we need to wait for the server response

            free (req);

            return 0;
        }

        else logMsg (stderr, ERROR, PACKET, "Failed to generate join lobby request packet!");
    }

    return 1;

}

// TODO: check that we are making valid requests to a game server
// request the server to leave the lobby
u8 client_game_leaveLobby (Client *client) {

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
u8 client_game_destroyLobby (Client *client) {

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
u8 client_game_startGame (Client *owner) {

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

#pragma endregion