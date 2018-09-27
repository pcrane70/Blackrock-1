#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>

#include <errno.h>

#include "network/client.h"
#include "network/requests.h"

#include "utils/myUtils.h"

#define PORT    9001

// FIXME:
// #define SERVER_ADDRESS  "192.168.1.7

int clientSocket;

bool connectedToServer = false;

/*** CONNECTION ***/

// TODO: what else do we want to init later?
int initClient (void) {

    // create client socket
    int client = socket (AF_INET, SOCK_STREAM, 0);

    return client;

}

int initConnection (void) {

    clientSocket = initClient ();
    if (clientSocket < 0) {
        fprintf (stderr, "Error creating client socket!\n");
        close (clientSocket);
        return 1;
    }

    struct sockaddr_in serverAddress;

    memset (&serverAddress, 0, sizeof (struct sockaddr_in));

    serverAddress.sin_family = AF_INET;
    // FIXME:
    // inet_pton(AF_INET, SERVER_ADDRESS, &(remote_addr.sin_addr));
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons (PORT);

    // try to connect to the server
    // FIXME: add the ability to try multiple times

    if (connect (clientSocket, (struct sockaddr *) &serverAddress, sizeof (struct sockaddr)) < 0) {
        fprintf (stderr, "%s\n", strerror (errno));
        fprintf (stderr, "Error connecting to server!\n");
        close (clientSocket);
        return 1;
    }

    // we expect a welcome message from the server
    char serverResponse [256];
    recv (clientSocket, &serverResponse, sizeof (serverResponse), 0);

    // handle the server response
    printf ("\n\nThe server sent the data:\n\n%s\n\n", serverResponse);

    connectedToServer = true;

    // connection is successfull
    return 0;

}

// TODO: wrap things up and disconnect from the server
int closeConnection (void) {

    close (clientSocket);

    connectedToServer = false;

    return 0;

}

/*** REQUESTS ***/

int makeRequest (RequestType type) {

    int retval;
    char *request = createString ("%i", type);

    switch (type) {
        case 1: 
            retval = recieveFile (request);
            if (retval == 0) fprintf (stdout, "Got the file!\n");
            else fprintf (stderr, "Error recieving file!\n");
            break;
        case 2: 
            // FIXME: post global db file
            break;
        case 3: break;
        default: fprintf (stderr, "Invalid request!\n"); break;
    }

    // do {
        // FIXME:
    // } while (request != 0);

    return retval;

}