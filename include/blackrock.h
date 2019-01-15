#ifndef BLACKROCK_H_
#define BLACKROCK_H_

#include <stdint.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define FPS_LIMIT   30

#define DEFAULT_SCREEN_WIDTH    1920    
#define DEFAULT_SCREEN_HEIGHT   1080

#define FPS_LIMIT   30

#define MAP_WIDTH   80
#define MAP_HEIGHT  40

#define NUM_COLS    80
#define NUM_ROWS    45

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned char asciiChar;

#define internal    static
#define localPersist    static
#define global  static

#define THREAD_OK   0

/*** PLATFORM ***/

#if defined (_WIN32)
    #define WINDOWS
#elif defined (__APPLE__)
    #define MACOS
#else
    #define LINUX
#endif

/*** PATH ***/

#ifdef DEV
    #define ASSETS_PATH "../assets/"
#elif PRODUCTION
    #define ASSETS_PATH "./assets/"
#else 
    #define ASSETS_PATH "../assets/"
#endif

/*** MULTIPLAYER ***/

#include "cerver/client.h"

extern char *black_server_ip;
extern u16 black_port;

extern Client *player_client;

extern Connection *main_connection;

/*** MISC ***/

extern void die (const char *error);

#endif