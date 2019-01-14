#ifndef VECTOR2D_H
#define VECTOR2D_H

typedef struct Point {

    int x, y;

} Point;

typedef struct Vector2D { 

    float x, y;

} Vector2D;

extern Vector2D vector_add (Vector2D a, Vector2D b);
extern void vector_add_equal (Vector2D *a, Vector2D b);

extern Vector2D vector_subtract (Vector2D a, Vector2D b);
extern void vector_substract_equal (Vector2D *a, Vector2D b);

extern Vector2D vector_multiply (Vector2D v, float scalar);
extern void vector_multiply_equal (Vector2D *v, float scalar);

extern Vector2D vector_divide (Vector2D v, float divisor);
extern void vector_divide_equal (Vector2D *v, float divisor);

extern Vector2D vector_negate (Vector2D v);
extern Vector2D vector_project (Vector2D project, Vector2D onto);
extern Vector2D unit_vector (Vector2D v);
extern Vector2D vector_rotate (Vector2D v, float radian);
// extern Vector2D vector_rotate_90 (Vector2D v);

#endif