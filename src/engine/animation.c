#include <stdlib.h>
#include <stdarg.h>

#include <SDL2/SDL.h>

#include "blackrock.h"

#include "engine/sprites.h"
#include "engine/animation.h"

/*** ANIMATION ***/

Animation *animation_create (u8 n_frames, ...) {

    va_list valist;
    va_start (valist, n_frames);

    Animation *animation = (Animation *) malloc (sizeof (Animation));
    if (animation) {
        animation->speed = DEFAULT_ANIM_SPEED;
        animation->n_frames = n_frames;
        animation->frames = (IndividualSprite **) calloc (n_frames, sizeof (IndividualSprite *));

        for (u8 i = 0; i < n_frames; i++)
            animation->frames[i] = va_arg (valist, IndividualSprite *);

        va_end (valist);
    }

    return animation;

}

void animation_destroy (Animation *animation) {

    if (animation) {
        if (animation->frames) free (animation->frames);

        free (animation);
    }

}

void animation_set_speed (Animation *animation, u32 speed) {

    if (animation) animation->speed = speed;

}

/*** ANIMATOR ***/

Animator *animator_new (u32 objectID) {

    Animator *new_animator = (Animator *) malloc (sizeof (Animator));
    if (new_animator) {
        new_animator->goID = objectID;
        new_animator->currAnimation = NULL;
        new_animator->currFrame = 0;
        new_animator->n_animations = 0;
        new_animator->animations = NULL;
    }
    
    return new_animator;

}

void animator_destroy (Animator *animator) {

    if (animator) {
        if (animator->animations) {
            for (u8 i = 0; i < animator->n_animations; i++)
                if (animator->animations[i])
                    animation_destroy (animator->animations[i]);

            free (animator->animations);
        }

        free (animator);
    }

}

void animator_set_current_animation (Animator *animator, Animation *animation) {

    if (animator && animation) 
        if (!animator->playing) animator->currAnimation = animation; 

}

void animator_play_animation (Animator *animator, Animation *animation) {

    if (animator && animation) {
        animator_set_current_animation (animator, animation);
        animator->playing = true;
        animator->currFrame = 0;
    } 

}