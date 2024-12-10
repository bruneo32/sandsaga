#ifndef PHYSICS_H
#define PHYSICS_H

#ifdef __cplusplus
#include <box2d/box2d.h> /* Include Box2D only in C++ */
extern "C" {
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <stddef.h>
#else
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Forward declarations for Box2D in C */
typedef struct b2World		  b2World;
typedef struct b2Body		  b2Body;
typedef struct b2Shape		  b2Shape;
typedef struct b2Fixture	  b2Fixture;
typedef struct b2PolygonShape b2PolygonShape;

enum b2BodyType {
	b2_staticBody = 0,
	b2_kinematicBody,
	b2_dynamicBody,
};

enum {
	e_shapeBit		  = 0x0001, ///< draw shapes
	e_jointBit		  = 0x0002, ///< draw joint connections
	e_aabbBit		  = 0x0004, ///< draw axis aligned bounding boxes
	e_pairBit		  = 0x0008, ///< draw broad-phase pairs
	e_centerOfMassBit = 0x0010, ///< draw center of mass frame
};

#endif

/** Box2D world pixels per unit */
#define B2D_WORLD_SCALE 8

/** Convert Box2D units to pixels. UV->XY */
#define U_TO_X(__u) ((__u) * ((double)B2D_WORLD_SCALE))

/** Convert pixels to Box2D units. XY->UV */
#define X_TO_U(__x) (((double)(__x)) / ((double)B2D_WORLD_SCALE))

/* Convert float range (0.0-1.0) to byte range (0-255) */
#define F2B(__f) ((uint8_t)((__f) * 255.0f))

typedef struct _Point2D {
	double x;
	double y;
} Point2D;

typedef struct _Triangle {
	Point2D p1;
	Point2D p2;
	Point2D p3;
} Triangle;

typedef struct _TriangleMesh {
	size_t	  count;
	Triangle *triangles;
} TriangleMesh;

/* For interoperability with SDL2 */
typedef struct Rect {
	int x, y;
	int w, h;
} Rect;

/* Box2D World functions */
b2World *box2d_world_create(float gravity_x, float gravity_y);
void box2d_world_step(b2World *world, float timeStep, int velocityIterations,
					  int positionIterations);
void box2d_world_destroy(b2World *world);
void box2d_debug_draw_active(bool on);
void box2d_debug_draw(b2World *world, Rect *camera);

/* Box2D World/Body functions */
b2Body *box2d_world_get_bodies(b2World *world, uint32_t *count);
b2Body *box2d_body_get_next(b2Body *body);

/* Box2D Body functions */
b2Body *box2d_body_create(b2World *world, float u, float v, int body_type,
						  bool fixed_rotation);
void	box2d_body_set_position(b2Body *body, float u, float v);
void	box2d_body_change_type(b2Body *body, int body_type);
void box2d_body_set_velocity(b2Body *body, float velocity_x, float velocity_y);
void box2d_body_set_velocity_v(b2Body *body, float vspeed);
void box2d_body_set_velocity_h(b2Body *body, float hspeed);
void box2d_body_add_velocity(b2Body *body, float velocity_x, float velocity_y);
void box2d_body_get_position(b2Body *body, float *u, float *v);
void box2d_body_destroy(b2Body *body);

b2Shape		   *box2d_shape_box(float width, float height);
b2PolygonShape *box2d_triangle(Point2D p1, Point2D p2, Point2D p3);
void box2d_body_create_fixture(b2Body *body, b2Shape *shape, float density,
							   float friction);

/* Meshgen functions */
TriangleMesh *triangulate(size_t start_i, size_t end_i, size_t start_j,
						  size_t end_j, bool (*is_valid)(size_t x, size_t y));

void convert_shape_to_box2d_units(TriangleMesh *mesh, double *centroid_x,
								  double *centroid_y);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_H
