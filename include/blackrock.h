#ifndef BLACKROCK_H_
#define BLACKROCK_H_

#include <stdint.h>

#define FPS_LIMIT   30

#define DEFAULT_SCREEN_WIDTH    1280    
#define DEFAULT_SCREEN_HEIGHT   720

#define FPS_LIMIT   30

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

/*** PATHS ***/

// FIXME: portability!!

#ifdef DEV
    #define ASSETS_PATH "./assets/"
#elif PRODUCTION
    #define ASSETS_PATH "../assets/"
#else 
    #define ASSETS_PATH "./assets/"
#endif

#ifdef DEV
    #define CONFIG_PATH "./config/"
#elif PRODUCTION    
    #define CONFIG_PATH "../config/"
#else 
    #define CONFIG_PATH "./config/"
#endif

#ifdef DEV
    #define DATA_PATH "./data/"
#elif PRODUCTION    
    #define DATA_PATH "../data/"
#else 
    #define DATA_PATH "./data/"
#endif

/*** THREAD ***/

extern float deltaTime;
extern u32 fps;

/*** MULTIPLAYER ***/

#include "cerver/client.h"

extern char *black_server_ip;
extern u16 black_port;

extern Client *player_client;

extern Connection *main_connection;

/*** MISC ***/

extern void die (const char *error);

extern bool running;
extern bool inGame;
extern bool wasInGame;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif