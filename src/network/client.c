#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>

#include "blackrock.h"      // only used for type defs

#include "network/client.h"

#include "config.h"
#include "utils/log.h"

bool connected = false;     // connected to server?

#pragma PACKETS

#pragma endregion

#pragma CLIENT

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

// connects a client to the specified server
u8 client_connectToServer () {


}

u8 client_disconnectFromServer () {

}

#pragma endregion