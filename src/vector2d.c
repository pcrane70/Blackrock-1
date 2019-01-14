#include <math.h>

#include "vector2d.h"

Vector2D vector_add_vector (Vector2D a, Vector2D b) {

    Vector2D c = { a.x + b.x, a.y + b.y };
    return c;

}

void vector_add_equal (Vector2D *a, Vector2D b) { a->x += b.x; a->y += b.y; }

Vector2D vector_subtract (Vector2D a, Vector2D b) {

    Vector2D c = { a.x - b.x, a.y - b.y };
    return c;

}

void vector_subtract_equal (Vector2D *a, Vector2D b) { a->x -= b.x; a->y -= b.y; }

Vector2D vector_negate (Vector2D v) {

    Vector2D n = { -v.x, -v.y };
    return n;

}

Vector2D vector_multiply (Vector2D v, float scalar) {

    Vector2D r = { v.x * scalar, v.y * scalar };
    return r;
}

void vector_multiply_equal (Vector2D *v, float scalar) { v->x *= scalar; v->y *= scalar; }

Vector2D vector_divide (Vector2D v, float divisor) {

    Vector2D r = { 0, 0 };
    if (divisor != 0) 
    {
        r.x = v.x / divisor;
        r.y = v.y / divisor;
    }
    return r;
}

void vector_divide_equal (Vector2D *v, float divisor) { 
    
    if (divisor) {
        v->x /= divisor; 
        v->y /= divisor;
    }  
    
}

float vector_length (Vector2D v) { return sqrtf (v.x * v.x + v.y * v.y); }

Vector2D unit_vector (Vector2D v) {

    float length = vector_length (v);
    if (length > 0) return vector_divide (v, length);
    return v;

}

Vector2D rotate_vector (Vector2D v, float radian) {

    float sine = sinf (radian);
    float cosine = cosf (radian);
    Vector2D r = { v.x * cosine + v.y * sine, v.x * sine + v.y * cosine };
    return r;

}