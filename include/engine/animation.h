#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdbool.h>

#include "sprites.h"

#define DEFAULT_ANIM_SPEED      100

typedef struct Animation {

    IndividualSprite **frames;
    u8 n_frames;
    u32 speed;

} Animation;

typedef struct Animator {

    u32 goID;
    bool playing;
    Animation *currAnimation;
    u8 n_animations;
    u8 currFrame;
    Animation **animations;

} Animator;

extern Animation *animation_create (u8 n_frames, ...);
extern void animation_destroy (Animation *animation);
extern void animation_set_speed (Animation *animation, u32 speed);

extern Animator *animator_new (u32 objectID);
extern void animator_destroy (Animator *animator);
extern void animator_set_current_animation (Animator *animator, Animation *animation);
extern void animator_play_animation (Animator *animator, Animation *animation);

#endif