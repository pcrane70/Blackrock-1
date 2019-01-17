#include <stdlib.h>

#include "blackrock.h"
#include "game/game.h"
#include "game/camera.h"

#include "engine/input.h"
#include "vector2d.h"

#include "utils/myUtils.h"

static void camera_init (Camera *cam, u32 windowWidth, u32 windowHeight);

// camera constructor
Camera *camera_new (u32 windowWidth, u32 windowHeight) {

    Camera *cam = (Camera *) malloc (sizeof (Camera));
    if (cam) camera_init (cam, windowWidth, windowHeight);

    return cam;

}

void camera_destroy (Camera *cam) {

    if (cam) {
        cam->target = NULL;

        free (cam);
    }

}

void camera_set_center (Camera *cam, u32 x, u32 y) {

    if (cam) {
        cam->center.x = x;
        cam->center.y = y;
        cam->bounds.x = x - (cam->bounds.w * 0.5);
        cam->bounds.y = y - (cam->bounds.h * 0.5);
    }

}

// TODO:
void camera_set_size (Camera *cam, u32 width, u32 height) {

    if (cam) {

    }

}

void camera_set_vel (Camera *cam, Vector2D vel) { if (cam) cam->velocity = vel; }

void camera_set_acceleration (Camera *cam, Vector2D accel) { if (cam) cam->acceleration = accel; }

void camera_set_direction (Camera *cam, Vector2D dir) { if (cam) cam->direction = dir; }

// set camera parameters to default
static void camera_init (Camera *cam, u32 windowWidth, u32 windowHeight) {

    // position
    cam->transform.position.x = DEFAULT_SCREEN_WIDTH / 2;
    cam->transform.position.y = DEFAULT_SCREEN_HEIGHT / 2;

    cam->windowWidth = windowWidth;
    cam->windowHeight = windowHeight;

    cam->bounds.x = cam->bounds.y = 0;
    cam->bounds.w = windowWidth;
    cam->bounds.h = windowHeight;

    // motion
    cam->accelerationRate = DEFAULT_CAM_ACCEL;
    cam->maxVel = DEFAULT_CAM_MAX_VEL;
    cam->drag = DEFAULT_CAM_DRAG;

    camera_set_center (cam, 0, 0);

    Vector2D nullVector = { 0, 0 };
    camera_set_vel (cam, nullVector);
    camera_set_acceleration (cam, nullVector);
    camera_set_direction (cam, nullVector);

    cam->isFollwing = false;
    cam->target = NULL;

}

/*** SCREEN ***/

#pragma region SCREEN

static Point point_world_to_screen (Camera *cam, const Point p, 
    float xScale, float yScale) {

    Point retPoint = p;

    retPoint.x -= cam->bounds.x;
    retPoint.y -= cam->bounds.y;

    retPoint.x *= xScale;
    retPoint.y *= yScale;

    return retPoint;

}

CamRect camera_world_to_screen (Camera *cam, const CamRect destRect) {

    if (cam) {
        CamRect screenRect = destRect;

        float xScale = (float) cam->windowWidth / cam->bounds.w;
        float yScale = (float) cam->windowHeight / cam->bounds.h;

        Point screenPoint = { screenRect.x, screenRect.y };
        screenPoint = point_world_to_screen (cam, screenPoint, xScale, yScale);

        screenRect.x = screenPoint.x;
        screenRect.y = screenPoint.y;
        screenRect.w = (int) (screenRect.w * xScale);
        screenRect.h = (int) (screenRect.h * yScale);

        return screenRect;
    }

}

// FIXME:
CamRect *camera_screen_to_world (Camera *cam, const CamRect *sr) {

    // CamRect *rect = sr;

    // float xScale = (float) cam->bounds.w / cam->windowWidth;
    // float yScale = (float) cam->bounds.h / cam->windowHeight;

    // Point p = { rect->x, rect->y };
    // FIXME:

    // rect->x = 

}

#pragma endregion

/*** MOVEMENT ***/

#pragma region MOVEMENT

void camera_set_max_vel (Camera *cam, float maxVel) { 
    
    if (cam) cam->maxVel = maxVel > 0 ? maxVel : 0; 
    
}

// TODO: do we need to normalize the movement when in diagonal?
// TODO: check again with draft
void camera_pan (Camera *cam, float xdir, float ydir) {

    if (cam) {
        Vector2D move = { xdir, ydir };
        // vector_multiply_equal (&move, cam->accelerationRate);
        // vector_add_equal (&cam->acceleration, move);

        vector_add_equal (&cam->center, move);
    }

}

#pragma endregion

// void Camera2D::Camera::updateMotion(float deltaTime)
// {

// 		m_acceleration.limit(m_accelerationRate);
// 		m_velocity += m_acceleration * deltaTime;

// 		m_centre += m_velocity * deltaTime;
// 		m_velocity.limit(m_maxVelocity);

// 	m_timeSinceLastXAccel += deltaTime;
// 	m_timeSinceLastYAccel += deltaTime;

// 	if (m_timeSinceLastXAccel > MAX_TIME_BEFORE_ACCEL_RESET && m_zoomToFitActive == false) //too long since last x accel
// 	{
// 		if (abs(m_velocity.x) < MIN_VEL) //moving slow enough then just stop 
// 		{
// 			m_velocity.x = 0.f;
// 		}
// 		else //apply drag
// 		{
// 			m_velocity.x -= m_velocity.x *  deltaTime * m_drag;
// 		}
// 	}
// 	if (m_timeSinceLastYAccel > MAX_TIME_BEFORE_ACCEL_RESET && m_zoomToFitActive == false) //too long since last y accel
// 	{
// 		if (abs(m_velocity.y) < MIN_VEL) //moving slow enough then just stop 
// 		{
// 			m_velocity.y = 0.f;
// 		}
// 		else //apply drag
// 		{
// 			m_velocity.y -= m_velocity.y *  deltaTime * m_drag;
// 		}
// 	}

// 	if (m_timeSinceLastXAccel > MAX_TIME_BEFORE_ACCEL_RESET && m_timeSinceLastYAccel > MAX_TIME_BEFORE_ACCEL_RESET && m_zoomToFitActive == false)
// 	{
// 		m_acceleration.limit(0.f);
// 	}

// }

void camera_update (Camera *cam) {

    // camera input
    #ifdef DEV
        if (input_is_key_down (SDL_SCANCODE_LEFT)) camera_pan (cam, -1, 0);
        if (input_is_key_down (SDL_SCANCODE_RIGHT)) camera_pan (cam, 1, 0);
        if (input_is_key_down (SDL_SCANCODE_UP)) camera_pan (cam, 0, -1);
        if (input_is_key_down (SDL_SCANCODE_DOWN)) camera_pan (cam, 0, 1);
    #endif

    // bounds - used to calculate what gets rendered to the screen
    cam->bounds.x = (cam->center.x - cam->bounds.w * 0.5);
    cam->bounds.y = (cam->center.y - cam->bounds.h * 0.5);

}