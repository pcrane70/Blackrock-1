#ifndef BLACKROCK_H_
#define BLACKROCK_H_

#include <stdint.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define FPS_LIMIT   20

#define SCREEN_WIDTH    1280    
#define SCREEN_HEIGHT   720

#define MAP_WIDTH   80
#define MAP_HEIGHT  40

#define NUM_COLS    80
#define NUM_ROWS    45

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned char asciiChar;

#define internal    static
#define localPersist    static
#define global  static

#define THREAD_OK   0

/*** MULTIPLAYER ***/

#include "cerver/client.h"

extern char *black_server_ip;
extern u16 black_port;

extern Client *player_client;

extern Connection *main_connection;

/*** MISC ***/

extern void pthread_create_detachable (void *(*work) (void *), void *args);
extern void die (const char *error);

#endif