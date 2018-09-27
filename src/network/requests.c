#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#include "network/client.h"

/*** FILES ***/

// file data
struct stat fileStats;
char fileSize[256];
int fd;

// this are from the makefile
const char localLBPath[64] = "./data/localLB.cfg";
const char globalLBPath[64] = "./data/globalLB.cfg";

int getFileData (void) {

    // prepare the file to send
    int fd = open (globalLBPath, O_RDONLY);
    if (fd < 0) return -1;

    // get file stats
    if (fstat (fd, &fileStats) < 0) return -1;

    return fd;

}

// FIXME:
int sendFile (int peerSocket) {

    // get the file data
    fd = getFileData ();

    // FIXME: handle this error
    if (fd < 0) {
        fprintf (stderr, "Error getting file data!\n");
        return -1;
    } 

    sprintf (fileSize, "%ld", fileStats.st_size);
    fprintf (stdout, "File size: %ld bytes.\n", fileStats.st_size);

    // Seinding file size
    ssize_t len;

    len = send (peerSocket, fileSize, sizeof (fileSize), 0);
    if (len < 0) {
        fprintf (stderr, "Error sending file size!\n");
        return 1;
    } 
    else fprintf (stdout, "Server sent %ld bytes for the size.\n", len);

    // sending file data
    off_t offset = 0;
    int remainData = fileStats.st_size;
    int sentBytes = 0;

    while (((sentBytes = sendfile (peerSocket, fd, &offset, BUFSIZ)) > 0) && (remainData > 0)) {
        remainData -= sentBytes;
        fprintf (stdout, "Server sent %d bytes from file's data, offset is now: %ld and remaining data = %d\n", sentBytes, offset, remainData);
    }

    if (sentBytes == fileStats.st_size) return 0;
    else return 1;

}

int recieveFile (char *request) {

    // FIXME: make sure that we create a new file if it does not exists
    // prepare the file where we are copying to
    FILE *file = fopen (globalLBPath, "w+");
    if (file == NULL) {
        fprintf (stderr, "%s\n", strerror (errno));
        return 1;
    }

    if (write (clientSocket, request, strlen (request)) < 0) {
        fprintf (stderr, "Error on writing!\n\n");
        return 1; 
    }

    size_t len;
    char buffer[BUFSIZ];
    int fileSize;
    int remainData = 0;

    recv (clientSocket, buffer, BUFSIZ, 0);
    fileSize = atoi (buffer);
    fprintf (stdout, "\nRecieved file size : %d\n", fileSize);

    remainData = fileSize;

    // get the file data
    while (((len = recv (clientSocket, buffer, BUFSIZ, 0)) > 0) && (remainData > 0)) {
        fwrite (buffer, sizeof (char), len, file);
        remainData -= len;
        fprintf (stdout, "Received %ld bytes and we hope %d more bytes\n", len, remainData);
    }

    fprintf (stdout, "Done!");

    fclose (file);
    return 0;

}