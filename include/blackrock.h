#ifndef BLACKROCK_H_
#define BLACKROCK_H_

#include <stdint.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define SCREEN_WIDTH    1280    
#define SCREEN_HEIGHT   720

// TODO: how much space do we want to leave for the HUD??
#define MAP_WIDTH   80
#define MAP_HEIGHT  40

#define NUM_COLS    80
#define NUM_ROWS    45

// TODO:
// 07/08/2018 --> I removed this variables and forget about portability for now
// we refactor our code to use the standar variable types and see how it goes...
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned char asciiChar;

#define internal    static
#define localPersist    static
#define global  static



#endif