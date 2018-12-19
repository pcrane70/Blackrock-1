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

PacketInfo *newPacketInfo (Client *client, Connection *connection,
     char *packetData, size_t packetSize) {

    if (client && connection) {
        PacketInfo *p = NULL;

        if (client->packetPool) {
            p = pool_pop (client->packetPool);
            if (!p) p = (PacketInfo *) malloc (sizeof (PacketInfo));
            // else if (p->packetData) free (p->packetData);
        }

        else {
            p = (PacketInfo *) malloc (sizeof (PacketInfo));
            p->packetData = NULL;
        } 

        p = (PacketInfo *) malloc (sizeof (PacketInfo));

        if (p) {
            p->client = client;
            p->connection = connection;
            p->packetSize = packetSize;

            p->packetData = packetData;
        }

        return p;
    }

    return NULL;

}

// used to destroy remaining packet info in the pools
void destroyPacketInfo (void *data) {

    if (data) {
        PacketInfo *packet = (PacketInfo *) data;
        packet->client = NULL;
        packet->connection = NULL;
        if (packet->packetData) free (packet->packetData);
    }

}

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

void initPacketHeader (PacketHeader *header, PacketType type, u32 packetSize) {

    header->protocolID = PROTOCOL_ID;
    header->protocolVersion = PROTOCOL_VERSION;
    header->packetType = type;
    header->packetSize = packetSize;

}

// generates a generic packet with the specified packet type
void *client_generatePacket (PacketType packetType, size_t packetSize) {

    size_t packet_size;
    if (packetSize > 0) packet_size = packetSize;
    else packet_size = sizeof (PacketHeader);

    PacketHeader *header = (PacketHeader *) malloc (packet_size);
    initPacketHeader (header, packetType, packet_size); 

    return header;

}

i8 udp_sendPacket (Connection *connection, const void *begin, size_t packetSize, 
    const struct sockaddr_storage address) {

    ssize_t sent;
    const void *p = begin;
    while (packetSize > 0) {
        sent = sendto (connection->sock_fd, begin, packetSize, 0, 
            (const struct sockaddr *) &address, sizeof (struct sockaddr_storage));
        if (sent <= 0) return -1;
        p += sent;
        packetSize -= sent;
    }

    return 0;

}

i8 tcp_sendPacket (i32 socket_fd, void *begin, size_t packetSize, int flags) {

    ssize_t sent;
    const void *p = begin;
    while (packetSize > 0) {
        sent = send (socket_fd, p, packetSize, flags);
        if (sent <= 0) return -1;
        p += sent;
        packetSize -= sent;
    }

    char *end = begin;
    DefAuthData *authdata = (DefAuthData *) (end + sizeof (PacketHeader) + sizeof (RequestType));

    return 0;

}

i8 client_sendPacket (Connection *connection, void *packet, size_t packetSize) {

    i8 retval = -1;

    if (connection) {
        switch (connection->protocol) {
            case IPPROTO_TCP:
                retval = tcp_sendPacket (connection->sock_fd, packet, packetSize, 0); break;
            case IPPROTO_UDP: 
                retval = udp_sendPacket (connection, packet, packetSize, 
                    connection->address); 
                break;
            default: break;
        }
    }

    return retval;

}

#pragma endregion

/*** ERRORS ***/

#pragma region ERRORS

ErrorData last_error;

void client_register_to_next_error (Client *client, Action action, void *args) {

    if (client) {
        client->errorAction = action;
        client->errorArgs = args;
    } 

}

void client_register_to_error_type (Client *client, Action action, void *args,
    ErrorType errorType) {

    if (client) {
        client->errorType = errorType;
        client->errorAction = action;
        client->errorArgs = args;
    }

}

void handleErrorPacket (PacketInfo *pack_info) {

    if (pack_info) {
        char *end = pack_info->packetData;
        ErrorData *error = (ErrorData *) (end += sizeof (PacketHeader));
        switch (error->type) {
            case ERR_SERVER_ERROR: break;
            case ERR_CREATE_LOBBY: break;
            case ERR_JOIN_LOBBY: break;
            case ERR_LEAVE_LOBBY: break;
            case ERR_FIND_LOBBY: break;
            case ERR_GAME_INIT: break;
            case ERR_FAILED_AUTH: 
                #ifdef CLIENT_DEBUG
                logMsg (stderr, ERROR, NO_TYPE, 
                    createString ("Failed to authenticate - %s", error->msg)); 
                #endif
                last_error.type = ERR_FAILED_AUTH;
                memset (last_error.msg, 0, sizeof (last_error.msg));
                strcpy (last_error.msg, error->msg);
                if (pack_info->client->errorType == ERR_FAILED_AUTH)
                    pack_info->client->errorAction (pack_info->client->errorArgs);
            break;
            default: logMsg (stdout, WARNING, NO_TYPE, "Unknown error recieved from server!"); break;
        }
    }

}

#pragma endregion

/*** CONNECTION HANDLER ***/

#pragma region CONNECTION HANDLER

void server_handlePacket (PacketInfo *packet);

// called with the th pool to handle a new packet
void handlePacket (void *data) {

    if (data) {
        PacketInfo *pack_info = (PacketInfo *) data;

        if (!checkPacket (pack_info->packetSize, pack_info->packetData, DONT_CHECK_TYPE))  {
            PacketHeader *header = (PacketHeader *) pack_info->packetData;

            switch (header->packetType) {
                // handles a packet with server info
                case SERVER_PACKET: server_handlePacket (pack_info); break;

                // handles an error from the server
                case ERROR_PACKET: handleErrorPacket (pack_info); break;
                
                // handles authentication packets
                case AUTHENTICATION: {
                    char *end = pack_info->packetData;
                    RequestData *reqdata = (RequestData *) (end += sizeof (PacketHeader));

                    switch (reqdata->type) {
                        case CLIENT_AUTH_DATA: {
                            Token *tokenData = (Token *) (end += sizeof (RequestData));
                            #ifdef CLIENT_DEBUG 
                            logMsg (stdout, DEBUG_MSG, CLIENT,
                                createString ("Token recieved from server: %s", tokenData->token));
                            #endif  
                            Token *token_data = (Token *) malloc (sizeof (Token));
                            memcpy (token_data->token, tokenData->token, sizeof (token_data->token));
                            pack_info->connection->server->token_data = token_data;
                        } break;
                        default: break;
                    }
                    break;
                }

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
        pool_push (pack_info->client->packetPool, pack_info);
    }

}

Connection *connection_get_by_socket (Client *client, i32 sock_fd);

// split the entry buffer in packets of the correct size
void handleRecvBuffer (Client *client, i32 socket_fd, char *buffer, size_t total_size) {

    if (buffer && (total_size > 0)) {
        u32 buffer_idx = 0;
        char *end = buffer;

        PacketHeader *header = NULL;
        u32 packet_size;
        char *packet_data = NULL;

        PacketInfo *info = NULL;

        while (buffer_idx < total_size) {
            header = (PacketHeader *) end;

            // check the packet size
            packet_size = header->packetSize;
            if (packet_size > 0) {
                // copy the content of the packet from the buffer
                packet_data = (char *) calloc (packet_size, sizeof (char));
                for (u32 i = 0; i < packet_size; i++, buffer_idx++) 
                    packet_data[i] = buffer[buffer_idx];

                info = newPacketInfo (client, connection_get_by_socket (client, socket_fd),
                     packet_data, packet_size);
                
                if (info) 
                    thpool_add_work (client->thpool, (void *) handlePacket, info);

                end += packet_size;
            }

            else break;
        }

        // free (buffer);
    }

}

// FIXME: correctly end the connections
// TODO: add support for handling large files transmissions
// recive all incoming data from this socket
/* void client_recieve (Client *client, i32 socket_fd) {

    ssize_t rc;
    char *packet_buffer = (char *) calloc (MAX_UDP_PACKET_SIZE, sizeof (char));
    memset (packet_buffer, 0, MAX_UDP_PACKET_SIZE);

    // do {
        rc = recv (socket_fd, packet_buffer, MAX_UDP_PACKET_SIZE, 0);
        
        if (rc < 0) {
            if (errno != EWOULDBLOCK) {     // no more data to read 
                // logMsg (stderr, ERROR, CLIENT, "Recv failed!");
                // perror ("Error:");
                logMsg (stdout, DEBUG_MSG, CLIENT, "client_recieve () - rc < 0");
                close (socket_fd);  // just close the socket
                free (packet_buffer);
            }
            // /break;
        }

        else if (rc == 0) {
            // man recv -> steam socket perfomed an orderly shutdown
            // but in dgram it might mean something?
            logMsg (stdout, DEBUG_MSG, CLIENT, 
                    "Ending connection - client_recieve () - rc == 0");

            close (socket_fd);  // close the socket
            free (packet_buffer);

            // FIXME: close the connection
            // client_disconnectFromServer (client);
            // break;
        }

        else 
            handleRecvBuffer (client, socket_fd, packet_buffer, rc);
    // } while (true);

} */

// FIXME:
void client_recieve (Client *client, i32 fd) {

    ssize_t rc;
    char packetBuffer[MAX_UDP_PACKET_SIZE];
    memset (packetBuffer, 0, MAX_UDP_PACKET_SIZE);

    // do {
        rc = recv (fd, packetBuffer, sizeof (packetBuffer), 0);
        
        if (rc < 0) {
            if (errno != EWOULDBLOCK) {     // no more data to read 
                // logMsg (stderr, ERROR, CLIENT, "Recv failed!");
                // perror ("Error:");
                // logMsg (stdout, DEBUG_MSG, CLIENT, 
                //     "Ending server connection - client_recieve () - rc < 0");
                // client_disconnectFromServer (client);
            }

            // /break;
        }

        if (rc == 0) {
            // man recv -> steam socket perfomed an orderly shutdown
            // but in dgram it might mean something?
            // logMsg (stdout, DEBUG_MSG, CLIENT, 
            //         "Ending server connection - client_recieve () - rc == 0");
            // client_disconnectFromServer (client);
            // break;
        }

        /* #ifdef CLIENT_DEBUG
            logMsg (stdout, DEBUG_MSG, CLIENT, 
                createString ("recv () - buffer size: %li", rc));
        #endif */

        char *buffer_data = (char *) calloc (MAX_UDP_PACKET_SIZE, sizeof (char));
        if (buffer_data) {
            memcpy (buffer_data, packetBuffer, rc);
            handleRecvBuffer (client, fd, packetBuffer, rc);
        }
        
    // } while (true);

}

#pragma endregion

/*** SERVER ***/

#pragma region SERVER

// compare the info the server sent us with the one we expected 
// and ajust our connection values if necessary
void server_checkServerInfo (Client *client, Connection *connection, SServer *server_info) {

    if (client && connect && server_info) {
        // TODO: we need to update our connection based on these values
        connection->server->port = server_info->port;
        connection->server->protocol = server_info->protocol;
        connection->server->useIpv6 = server_info->useIpv6;

        connection->server->type = server_info->type;
        switch (connection->server->type) {
            case FILE_SERVER: 
                logMsg (stdout, DEBUG_MSG, SERVER, "Connected to a file server.");
                break;
            case WEB_SERVER:
                logMsg (stdout, DEBUG_MSG, SERVER, "Connected to a web server.");
                break;
            case GAME_SERVER: 
                logMsg (stdout, DEBUG_MSG, SERVER, "Connected to a game server.");
                break;
            default: 
                logMsg (stdout, WARNING, SERVER, 
                    createString ("Connected to a server of an unknown type: %i.", 
                    server_info->type)); 
                break;
        }

        connection->server->authRequired = server_info->authRequired;
        if (connection->server->authRequired) {
            #ifdef CLIENT_DEBUG
                logMsg (stdout, DEBUG_MSG, SERVER, "Server requires authentication...");
            #endif
            connection->authentication (connection->authData);
        }

        else {
            #ifdef CLIENT_DEBUG
                logMsg (stdout, DEBUG_MSG, SERVER, "Server does NOT require authentication.");
            #endif
        }
    }

}

u8 client_disconnectFromServer (Client *client, Connection *connection);

// FIXME:
void server_handlePacket (PacketInfo *pack_info) {

    char *end =pack_info->packetData;  
    RequestData *reqdata = (RequestData *) (end += sizeof (PacketHeader));

    switch (reqdata->type) {
        case SERVER_INFO: {
            #ifdef CLIENT_DEBUG
                logMsg (stdout, DEBUG_MSG, SERVER, "Recieved a server info packet.");
            #endif
            SServer *serverInfo = (SServer *) (end += sizeof (RequestData));
            server_checkServerInfo (pack_info->client, pack_info->connection, serverInfo);
        } break;
        
        // FIXME:
        case SERVER_TEARDOWN: 
            // logMsg (stdout, WARNING, SERVER, "--> Server teardown!! <--"); 
            // if (!client_disconnectFromServer (packet->client))  
            //     logMsg (stdout, SUCCESS, CLIENT, "Disconnected client from server!");
            // else logMsg (stderr, ERROR, CLIENT, "Failed to disconnect client from server!");
            break;
        default: logMsg (stderr, WARNING, PACKET, "Unknown server type packet."); break;
    }

}

#pragma endregion

/*** AUTHENTICATION ***/

#pragma region AUTHENTICATION

void client_send_default_auth_data (void *data) {

    if (data) {
        ClientConnection *cc = (ClientConnection *) data;
        Client *client = cc->client;
        Connection *connection = cc->connection;

        if (client && connection) {
            size_t packetSize = sizeof (PacketHeader) + 
                sizeof (RequestData) + sizeof (DefAuthData);
            void *req = client_generatePacket (AUTHENTICATION, packetSize);

            if (req) {
                char *end = req;
                RequestData *reqdata = (RequestData *) (end += sizeof (PacketHeader));
                reqdata->type = CLIENT_AUTH_DATA;

                DefAuthData *authData = (DefAuthData *) (end += sizeof (RequestData));
                authData->code = DEFAULT_AUTH_CODE;

                if (tcp_sendPacket (connection->sock_fd, req, packetSize, 0) < 0)
                    logMsg (stderr, ERROR, PACKET, "Failed to send test packet!");
                else {
                    #ifdef CLIENT_DEBUG
                        logMsg (stdout, SUCCESS, PACKET, "Sent authentication packet to server.");
                    #endif
                }

                free (req);
            }
        }
    }

}

void client_set_send_auth_data (Client *client, Connection *connection,
    Action send_auth_data, void *auth_data) {

    if (client && connection && send_auth_data) {
        connection->authentication = send_auth_data;
        connection->authData = auth_data;
    }

}

#pragma endregion

#pragma region CONNECTION

// inits a client structure with the specified values
u8 connection_init (Connection *connection, u16 port, bool async) {

    #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT, "Initializing connection values...");
    #endif

    connection->useIpv6 = DEFAULT_USE_IPV6;
    #ifdef CLIENT_DEBUG
    logMsg (stdout, DEBUG_MSG, CLIENT, connection->useIpv6 == 1 ? 
        "Use IPv6: yes." : "Use IPv6: no.");
    #endif

    connection->protocol = DEFAULT_PROTOCOL;
    #ifdef CLIENT_DEBUG
    logMsg (stdout, DEBUG_MSG, CLIENT, connection->protocol == IPPROTO_TCP ? 
        "Using TCP protocol." : "Using UDP protocol.");
    #endif
        
    if (port <= 0 || port >= MAX_PORT_NUM) {
        logMsg (stdout, WARNING, CLIENT, 
            createString ("Invalid port number. Setting port to default value: %i", DEFAULT_PORT));
        connection->port = DEFAULT_PORT;
    }

    else connection->port = port;

    #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT,
            createString ("Using port: %i.", connection->port));
    #endif

    // init the new connection socket
    switch (connection->protocol) {
        case IPPROTO_TCP: 
            connection->sock_fd = socket ((connection->useIpv6 == 1 ? AF_INET6 : AF_INET), SOCK_STREAM, 0);
            break;
        case IPPROTO_UDP:
            connection->sock_fd = socket ((connection->useIpv6 == 1 ? AF_INET6 : AF_INET), SOCK_DGRAM, 0);
            break;

        default: logMsg (stderr, ERROR, CLIENT, "Unkonw protocol type!"); return 1;
    }

    if (connection->sock_fd < 0) {
        logMsg (stderr, ERROR, CLIENT, "Failed to create client socket!");
        return 1;
    }

    #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT, "Created new connection socket!");
    #endif

    if (async) {
        // set the socket to non blocking mode
        if (!sock_setBlocking (connection->sock_fd, connection->blocking)) {
            logMsg (stderr, ERROR, NO_TYPE, "Failed to set connection socket to non blocking mode!");
            close (connection->sock_fd);
            return 1;
        }

        else {
            connection->blocking = false;
            #ifdef CLIENT_DEBUG
            logMsg (stdout, DEBUG_MSG, NO_TYPE, "Connection socket set to non blocking mode.");
            #endif
        }
    }

    return 0;
    
}

Connection *client_connection_new (u16 port, bool async) {

    Connection *connection = (Connection *) malloc (sizeof (Connection));

    if (connection) {
        if (!connection_init (connection, port, async)) {
            connection->async = async;
            connection->isConnected = false;

            return connection;
        }
    }

    return NULL;

}

Connection *connection_get_by_socket (Client *client, i32 sock_fd) {

    if (client) {
        for (u8 i = 0; i < client->n_active_connections; i++)
            if (client->active_connections[i]->sock_fd == sock_fd)
                return client->active_connections[i];
    }

    return NULL;

}

#pragma endregion

/*** CLIENT LOGIC ***/

// TODO: we can add a restart client function similar to restart server

#pragma region CLIENT

// FIXME:
// cerver client constructor
Client *newClient (void) {

    Client *newClient = (Client *) malloc (sizeof (Client));

    if (newClient) {
        newClient->active_connections = 
            (Connection **) calloc (DEFAULT_MAX_CONNECTIONS, sizeof (Connection *));
        if (!newClient->active_connections) {
            logMsg (stderr, ERROR, CLIENT, "Failed to allocate client's active_connections!");
            return NULL;
        }

        newClient->n_active_connections = 0;

        // set default values
        newClient->running = false;

        // FIXME:
        newClient->inLobby = false;
        newClient->isOwner = false;

        newClient->errorType = -1;
        newClient->errorAction = NULL;
        newClient->errorArgs = NULL;
    }

    return newClient;

}

// public function to create a new client
Client *client_create (void) {

    Client *client = newClient ();

    if (client) {
        // init the client's poll strcuture
        memset (client->fds, 0, sizeof (client->fds));
        for (u8 i = 0; i < DEFAULT_MAX_CONNECTIONS; i++)    
            client->fds[i].fd = -1;

        client->nfds = 0;

        // init the client's packet info pool with some memebers in it
        client->packetPool = pool_init (destroyPacketInfo);
        if (client->packetPool) 
            for (int i = 0; i < DEFAULT_PACKET_POOL_INIT; i++)
                pool_push (client->packetPool, malloc (sizeof (PacketInfo)));
        
        else {
            #ifdef CLIENT_DEBUG
            logMsg (stderr, ERROR, CLIENT, "Failed to init client's packet info pool!");
            #endif
            free (client);
            return NULL;
        } 

        client->thpool = thpool_init (DEFAULT_THPOOL_INIT);
        if (!client->thpool) {
            #ifdef CLIENT_DEBUG
            logMsg (stderr, ERROR, CLIENT, "Failed to init client thpool!");
            #endif
            pool_clear (client->packetPool);
            free (client);
            return NULL;
        }

        client->pollTimeout = DEFAULT_POLL_TIMEOUT;

        logMsg (stdout, SUCCESS, CLIENT, "Created a new client!");
        return client;
    }

    logMsg (stderr, ERROR, CLIENT, "Failed to create a new client!");

    return NULL;

}

u8 client_get_free_poll_idx (Client *client) {

    for (u8 i = 0; i < DEFAULT_MAX_CONNECTIONS; i++)
        if (client->fds[i].fd == -1)
            return i;

    return -1;

}

u8 client_poll (void *data) {

    if (!data) {
        logMsg (stderr, ERROR, SERVER, "Can't poll on a NULL client!");
        return 1;
    }

    Client *client = (Client *) data;

    int poll_retval;    

    #ifdef CLIENT_DEBUG
        logMsg (stdout, SUCCESS, CLIENT, "Client poll has started!");
    #endif

    while (client->running) {
        poll_retval = poll (client->fds, DEFAULT_MAX_CONNECTIONS, client->pollTimeout);

        // poll failed
        if (poll_retval < 0) {
            logMsg (stderr, ERROR, NO_TYPE, "Client poll failed!");
            perror ("Error");
            // FIXME: close all of our active connections...
            break;
        }

        // if poll has timed out, just continue to the next loop... 
        if (poll_retval == 0) {
            // #ifdef CLIENT_DEBUG
            //     logMsg (stdout, DEBUG_MSG, NO_TYPE, "Poll timeout.");
            // #endif
            continue;
        }

        // one or more fd(s) are readable, need to determine which ones they are
        for (u8 i = 0; i < DEFAULT_MAX_CONNECTIONS; i++) {
            if (client->fds[i].revents == 0) continue;
            if (client->fds[i].revents != POLLIN) continue;

            client_recieve (client, client->fds[i].fd);
        }
    }

    #ifdef CLIENT_DEBUG
        logMsg (stdout, DEBUG_MSG, CLIENT, "Client poll has ended,");
    #endif

}

// try to connect a client to an address (server) with exponential backoff
u8 connectRetry (Connection *connection, const struct sockaddr_storage address) {

    i32 numsec;
    for (numsec = 2; numsec <= MAXSLEEP; numsec <<= 1) {
        if (!connect (connection->sock_fd, 
            (const struct sockaddr *) &address, 
            sizeof (struct sockaddr))) 
            return 0;

        if (numsec <= MAXSLEEP / 2) sleep (numsec);
    } 

    return 1;

}

// connects a client to the specified ip address, it does not have to be a cerver type server
Connection *client_make_new_connection (Client *client, const char *ip_address, u16 port, 
    bool async) {

    if (!ip_address) {
        logMsg (stderr, ERROR, NO_TYPE, "Failed to make new connection, no ip provided.");
        return NULL;
    }

    Connection *new_con = client_connection_new (port, async);

    if (new_con) {
        new_con->ip = (char *) calloc (strlen (ip_address) + 1, sizeof (char));
        strcpy (new_con->ip, ip_address);

        memset (&new_con->address, 0, sizeof (struct sockaddr_storage));

        if (new_con->useIpv6) {
            struct sockaddr_in6 *addr = 
                (struct sockaddr_in6 *) &new_con->address;
            addr->sin6_family = AF_INET6;
            // FIXME: addr->sin6_addr = inet;         
            addr->sin6_port = htons (new_con->port);
        } 

        else {
            struct sockaddr_in *addr = 
                (struct sockaddr_in *) &new_con->address;
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = inet_addr (new_con->ip);
            addr->sin_port = htons (new_con->port);
        } 

        // try to connect to the server with exponential backoff
        if (!connectRetry (new_con, new_con->address)) {
            client->active_connections[client->n_active_connections] = new_con;
            client->n_active_connections++;

            if (new_con->async) {
                // add the new socket to the poll structure
                u8 idx = client_get_free_poll_idx (client);

                if (idx >= 0) {
                    client->fds[idx].fd = new_con->sock_fd;
                    client->fds[idx].events = POLLIN;
                    client->nfds++;

                    // check if we walready have the client poll running
                    if (client->running == false) {
                        thpool_add_work (client->thpool, (void *) client_poll, client);
                        client->running = true;
                    }
                }     

                else logMsg (stderr, ERROR, CLIENT, 
                    "Failed to get free poll idx. Is the client full?");
            }

            logMsg (stdout, SUCCESS, CLIENT, "Connected to address!"); 

            return new_con;
        } 
    }

    logMsg (stderr, ERROR, NO_TYPE, "Failed to make new connection!");
    return NULL;

}

u8 client_end_connection (Client *client, Connection *connection) {

    if (client && connection) {
        close (connection->sock_fd);

        if (connection->server) {
            if (connection->server->ip) free (connection->server->ip);
            free (connection->server);
        }

        if (connection->ip) free (connection->ip);

        u8 idx = -1;
        for (u8 i = 0; i < client->n_active_connections; i++)
            if (connection->sock_fd == client->active_connections[i]->sock_fd)
                break;

        if (idx >= 0) {
            for (u8 i = idx; i < client->n_active_connections - 1; i++) 
                client->active_connections[i] = client->active_connections[i + 1];
            
            client->n_active_connections--;
        }

        free (connection);
        
        return 0;
    }

    return 1;

}

// by default the connection is async
// connects the client to a cerver type server
u8 client_connect_to_server (Client *client, Connection *con, const char *serverIp, u16 port,
    ServerType expectedType, Action send_auth_data, void *auth_data) {

    if (!serverIp) {
        logMsg (stderr, ERROR, SERVER, "Failed to connect to server, no ip provided.");
        return 1;
    }

    if (con) {
        con->server = (Server *) malloc (sizeof (Server));
        con->server->type = expectedType;

        con->server->ip = (char *) calloc (strlen (serverIp) + 1, sizeof (char));
        strcpy (con->server->ip, serverIp);

        memset (&con->server->address, 0, sizeof (struct sockaddr_storage));

        if (con->useIpv6) {
            struct sockaddr_in6 *addr = 
                (struct sockaddr_in6 *) &con->server->address;
            addr->sin6_family = AF_INET6;
            // FIXME: addr->sin6_addr = inet;         
            addr->sin6_port = htons (con->port);
        } 

        else {
            struct sockaddr_in *addr = 
                (struct sockaddr_in *) &con->server->address;
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = inet_addr (con->server->ip);
            addr->sin_port = htons (con->port);
        } 

        if (send_auth_data) client_set_send_auth_data (client, con, send_auth_data, auth_data);
            
        else {
            ClientConnection *cc = (ClientConnection *) malloc (sizeof (ClientConnection));
            cc->client = client;
            cc->connection = con;

            client_set_send_auth_data (client, con, client_send_default_auth_data, cc);
        }

        // try to connect to the server with exponential backoff
        if (!connectRetry (con, con->server->address)) {
            client->active_connections[client->n_active_connections] = con;
            client->n_active_connections++;

            // add the new socket to the poll structure
            u8 idx = client_get_free_poll_idx (client);
            if (idx >= 0) {
                client->fds[idx].fd = con->sock_fd;
                client->fds[idx].events = POLLIN;
                client->nfds++;

                // check if we walready have the client poll running
                if (client->running == false) {
                    thpool_add_work (client->thpool, (void *) client_poll, client);
                    client->running = true;
                }

                logMsg (stdout, SUCCESS, CLIENT, "Connected to server!"); 

                return 0;
            }              

            else logMsg (stderr, ERROR, CLIENT, 
                "Failed to get free poll idx. Is the client full?");
        } 
    }

    logMsg (stderr, ERROR, CLIENT, "Failed to connect to server!");
    return 1;

}

void *generateRequest (PacketType packetType, RequestType reqType);

// disconnect from the server
u8 client_disconnectFromServer (Client *client, Connection *connection) {

    if (client && connection) {
        if (connection->server) {
            if (connection->server->type == GAME_SERVER) {
                i8 client_game_leaveLobby (Client *client, Connection *connection);
                client_game_leaveLobby (client, connection);
            }

            // send a disconnect packet to the server
            size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
            void *pack = generateRequest (CLIENT_PACKET, CLIENT_DISCONNET);
            if (pack) {
                if (tcp_sendPacket (connection->sock_fd, pack, packetSize, 0) >= 0)
                    logMsg (stdout, DEBUG_MSG, PACKET, "Sent client disconnect packet.");

                else logMsg (stderr, ERROR, PACKET, "Failed to send client disconnect packet.");

                free (pack);
            }

            client_end_connection (client, connection);

            if (client->n_active_connections <= 0) 
                client->running = false;

            #ifdef CLIENT_DEBUG
                logMsg (stdout, SUCCESS, CLIENT, "Client disconnected from server!");
            #endif

            return 0;
        }
    }

    logMsg (stderr, ERROR, CLIENT, "Failed to disconnect client from server!");

    return 1;

}

// stop any on going process and destroy
u8 client_teardown (Client *client) {

    if (client) {
        if (client->running) {
            while (client->n_active_connections > 0) 
                client_end_connection (client, 
                    client->active_connections[client->n_active_connections - 1]);
            
            free (client->active_connections);
            client->running = false;
        }
        
        if (client->thpool) {
            #ifdef CLIENT_DEBUG
                logMsg (stdout, DEBUG_MSG, CLIENT,
                    createString ("Active threads in thpool: %i", 
                    thpool_num_threads_working (client->thpool)));
            #endif

            thpool_destroy (client->thpool);
            #ifdef CLIENT_DEBUG
                logMsg (stdout, SUCCESS, CLIENT, "Client thpool got destroyed!");
            #endif
        } 

        // if (client->packetPool) pool_clear (client->packetPool); 

        free (client);

        return 0;
    }

    return 1;

}

#pragma endregion

/*** REQUESTS ***/

// These are the requests that we send to the server and we expect a response 

#pragma region REQUESTS

void *generateRequest (PacketType packetType, RequestType reqType) {

    size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
    void *begin = client_generatePacket (packetType, packetSize);
    char *end = begin;

    RequestData *reqdata = (RequestData *) (end += sizeof (PacketHeader));
    reqdata->type = reqType;

    return begin;

}

u8 client_makeTestRequest (Client *client, Connection *connection) {

    if (client && connection) {
        size_t packetSize = sizeof (PacketHeader);
        void *req = client_generatePacket (TEST_PACKET, packetSize);
        if (req) {
            if (client_sendPacket (connection, req, packetSize) < 0) 
                logMsg (stderr, ERROR, PACKET, "Failed to send test packet!");

            else logMsg (stdout, TEST, PACKET, "Sent test packet to server.");

            free (req);

            return 0;
        }
    }

    return 1;

}

#pragma endregion

/*** FILE SERVER ***/

#pragma region FILE SERVER

// TODO:
// request a file from the server
i8 client_file_get (Client *client, Connection *connection, const char *filename) {

    if (client && connection) {}

}

// TODO:
// send a file to the server
i8 client_file_send (Client *client, Connection *connection, const char *filename) {

    if (client && connection) {}

}

#pragma endregion

/*** GAME SERVER ***/

#pragma region GAME SERVER

// request to create a new multiplayer game
void *client_game_createLobby (Client *owner, Connection *connection, GameType gameType) {

    Lobby *new_lobby = NULL;

    // create a new connection
    Connection *new_con = client_make_new_connection (owner, connection->server->ip, 
        connection->server->port, false);

    if (new_con) {
        char buffer[1024];
        memset (buffer, 0, 1024);
        int rc = read (new_con->sock_fd, buffer, 1024);

        if (rc > 0) {
            char *end = buffer;
            PacketHeader *header = (PacketHeader *) end;
            #ifdef CLIENT_DEBUG
                if (header->packetType == SERVER_PACKET)
                    logMsg (stdout, DEBUG_MSG, NO_TYPE, "New connection - got a server packet.");
            #endif

            // authenticate using our server token
            size_t token_packet_size = sizeof (PacketHeader) + sizeof (RequestData) + sizeof (Token);
            void *token_packet = client_generatePacket (AUTHENTICATION, token_packet_size);
            if (token_packet) {
                char *end = token_packet;
                RequestData *req = (RequestData *) (end += sizeof (PacketHeader));
                req->type = CLIENT_AUTH_DATA;

                Token *tok = (Token *) (end += sizeof (RequestData));
                memcpy (tok->token, connection->server->token_data->token, sizeof (tok->token));

                client_sendPacket (new_con, token_packet, token_packet_size);
                free (token_packet);
            }

            else {
                logMsg (stderr, ERROR, CLIENT, "New connection - failed to create auth packet!");
                client_end_connection (owner, new_con);
                return NULL;
            }

            memset (buffer, 0, 1024);
            rc = read (new_con->sock_fd, buffer, 1024);

            if (rc > 0) {
                end = buffer;
                RequestData *reqdata = (RequestData *) (end + sizeof (PacketHeader));
                if (reqdata->type == SUCCESS_AUTH) {
                    #ifdef CLIENT_DEBUG
                        logMsg (stdout, DEBUG_MSG, NO_TYPE, 
                            "New connection - authenticated to server.");
                    #endif

                    sleep (1);

                    // make the create lobby request
                    size_t create_packet_size = sizeof (PacketHeader) + sizeof (RequestData);
                    void *lobby_req = generateRequest (GAME_PACKET, LOBBY_CREATE);
                    if (lobby_req) {
                        client_sendPacket (new_con, lobby_req, create_packet_size);
                        free (lobby_req);
                    }

                    else {
                        logMsg (stderr, ERROR, CLIENT, 
                            "New connection - failed to create lobby packet!");
                        client_end_connection (owner, new_con);
                        return NULL;
                    }
                    
                    memset (buffer, 0, 1024);
                    rc = read (new_con->sock_fd, buffer, 1024);

                    if (rc > 0) {
                        end = buffer;
                        RequestData *reqdata = (RequestData *) (end += sizeof (PacketHeader));
                        if (reqdata->type == LOBBY_UPDATE) {
                            SLobby *got_lobby = (SLobby *) (end += sizeof (RequestData));
                            new_lobby = (Lobby *) malloc (sizeof (SLobby));
                            memcpy (new_lobby, got_lobby, sizeof (SLobby));
                        }
                    }
                }
                   
            }
        }

        client_end_connection (owner, new_con);
    }

    return new_lobby;

}

// FIXME: send game type to server
// request to join an on going game
void *client_game_joinLobby (Client *client, Connection *connection, GameType gameType) {

    if (client && connection) {
        // create & send a join lobby req packet to the server
        size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
        void *req = generateRequest (GAME_PACKET, LOBBY_JOIN);

        if (req) {
            i8 retval = client_sendPacket (connection, req, packetSize);
            free (req);
            // return retval;
        }
    }

    return NULL;

}

// request the server to leave the lobby
i8 client_game_leaveLobby (Client *client, Connection *connection) {

    if (client && connection) {
        if (client->inLobby) {
            // create & send a leave lobby req packet to the server
            size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
            void *req = generateRequest (GAME_PACKET, LOBBY_LEAVE);

            if (req) {
                i8 retval = client_sendPacket (connection, req, packetSize);
                free (req);
                return retval;
            }
        }
    }

    return -1;

}

// request to destroy the current lobby, only if the client is the owner
i8 client_game_destroyLobby (Client *client, Connection *connection) {

    if (client && connection) {
        if (client->inLobby) {
            // create & send a leave lobby req packet to the server
            size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
            void *req = generateRequest (GAME_PACKET, LOBBY_DESTROY);

            if (req) {
                i8 retval = client_sendPacket (connection, req, packetSize);
                free (req);
                return retval;
            }
        }
    }

    return -1;

}

// the owner of the lobby can request to init the game
i8 client_game_startGame (Client *client, Connection *connection) {

    if (client && connection) {
        if (client->inLobby) {
            // create & send a leave lobby req packet to the server
            size_t packetSize = sizeof (PacketHeader) + sizeof (RequestData);
            void *req = generateRequest (GAME_PACKET, GAME_INIT);

            if (req) {
                i8 retval = client_sendPacket (connection, req, packetSize);
                free (req);
                return retval;
            }
        }
    }

    return -1;

}

#pragma endregion