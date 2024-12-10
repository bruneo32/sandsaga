#include "../graphics/graphics.h"
#include "physics.h"
#include <iostream>
#include <vector>

/* =============================================================== */
/* Debug draw for b2World */
Rect *renderCamera;

class DebugDraw : public b2Draw {
  public:
	/// Draw a closed polygon provided in CCW order.
	void DrawPolygon(const b2Vec2 *vertices, int32 vertexCount,
					 const b2Color &color) {
		Render_SetcolorRGBA(F2B(color.r), F2B(color.g), F2B(color.b),
							F2B(color.a));

		/* Start previous vertex with last vertex */
		b2Vec2 *pvex = (b2Vec2 *)&vertices[vertexCount - 1];
		for (int32 i = 0; i < vertexCount; ++i) {
			b2Vec2 *vex = (b2Vec2 *)&vertices[i];
			Render_Line(U_TO_X(pvex->x) - renderCamera->x,
						U_TO_X(pvex->y) - renderCamera->y,
						U_TO_X(vex->x) - renderCamera->x,
						U_TO_X(vex->y) - renderCamera->y);
			pvex = vex;
		}
	}

	/// Draw a solid closed polygon provided in CCW order.
	void DrawSolidPolygon(const b2Vec2 *vertices, int32 vertexCount,
						  const b2Color &color) {
		DrawPolygon(vertices, vertexCount, color);
	}

	/// Draw a circle.
	void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) {}

	/// Draw a solid circle.
	void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis,
						 const b2Color &color) {}

	/// Draw a line segment.
	void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) {
		Render_SetcolorRGBA(F2B(color.r), F2B(color.g), F2B(color.b),
							F2B(color.a));
		Render_Line(
			U_TO_X(p1.x) - renderCamera->x, U_TO_X(p1.y) - renderCamera->y,
			U_TO_X(p2.x) - renderCamera->x, U_TO_X(p2.y) - renderCamera->y);
	}

	/// Draw a transform. Choose your own length scale.
	/// @param xf a transform.
	void DrawTransform(const b2Transform &xf) {
		const float px	  = xf.p.x;
		const float py	  = xf.p.y;
		const float angle = xf.q.GetAngle();

		Render_Setcolor(C_GREEN);
		Render_Line(U_TO_X(px) - renderCamera->x, U_TO_X(py) - renderCamera->y,
					U_TO_X(px + 0.5f * cos(angle)) - renderCamera->x,
					U_TO_X(py + 0.5f * sin(angle)) - renderCamera->y);
	}

	/// Draw a point.
	void DrawPoint(const b2Vec2 &p, float size, const b2Color &color) {
		Render_SetcolorRGBA(F2B(color.r), F2B(color.g), F2B(color.b),
							F2B(color.a));
		Render_Pixel((int)U_TO_X(p.x), (int)U_TO_X(p.y));
	}
};
DebugDraw debug_draw;

/* =============================================================== */
/* Box2D World functions */
b2World *box2d_world_create(float gravity_x, float gravity_y) {
	b2World *world = new b2World(b2Vec2(gravity_x, gravity_y));
	world->SetDebugDraw(&debug_draw);
	return world;
}

void box2d_world_step(b2World *world, float timeStep, int velocityIterations,
					  int positionIterations) {
	world->Step(timeStep, velocityIterations, positionIterations);
}

void box2d_world_destroy(b2World *world) { delete world; }

void box2d_debug_draw_active(bool on) {
	if (!on) {
		/* Clear all flags */
		debug_draw.ClearFlags((uint32_t)-1);
	} else {
		/* Set all flags */
		debug_draw.SetFlags(
			b2Draw::e_shapeBit | b2Draw::e_jointBit | b2Draw::e_pairBit
			/* | b2Draw::e_centerOfMassBit | b2Draw::e_aabbBit*/);
	}
}

void box2d_debug_draw(b2World *world, Rect *camera) {
	renderCamera = camera;
	world->DebugDraw();
}

/* =============================================================== */
/* Box2D World/Body functions */
b2Body *box2d_world_get_bodies(b2World *world, uint32_t *count) {
	if (count)
		*count = world->GetBodyCount();
	return world->GetBodyList();
}

b2Body *box2d_body_get_next(b2Body *body) { return body->GetNext(); }

/* =============================================================== */
/* Box2D Body functions */
b2Body *box2d_body_create(b2World *world, float u, float v, int body_type,
						  bool fixed_rotation) {
	b2BodyDef bodyDef;
	bodyDef.type = static_cast<b2BodyType>(body_type);
	bodyDef.position.Set(u, v);
	bodyDef.fixedRotation = fixed_rotation;
	return world->CreateBody(&bodyDef);
}

void box2d_body_change_type(b2Body *body, int body_type) {
	body->SetType(static_cast<b2BodyType>(body_type));
}

void box2d_body_set_position(b2Body *body, float u, float v) {
	body->SetTransform(b2Vec2(u, v), body->GetAngle());
}

void box2d_body_set_velocity(b2Body *body, float velocity_x, float velocity_y) {
	body->SetLinearVelocity(b2Vec2(velocity_x, velocity_y));
}

void box2d_body_set_velocity_v(b2Body *body, float vspeed) {
	b2Vec2 velocity = body->GetLinearVelocity();
	velocity.y		= vspeed;
	body->SetLinearVelocity(velocity);
}

void box2d_body_set_velocity_h(b2Body *body, float hspeed) {
	b2Vec2 velocity = body->GetLinearVelocity();
	velocity.x		= hspeed;
	body->SetLinearVelocity(velocity);
}

void box2d_body_add_velocity(b2Body *body, float velocity_x, float velocity_y) {
	b2Vec2 velocity = body->GetLinearVelocity();
	velocity.x += velocity_x;
	velocity.y += velocity_y;
	body->SetLinearVelocity(velocity);
}

void box2d_body_get_position(b2Body *body, float *u, float *v) {
	b2Vec2 position = body->GetPosition();
	if (u)
		*u = position.x;
	if (v)
		*v = position.y;
}

void box2d_body_destroy(b2Body *body) {
	if (!body)
		return;
	b2World *world = body->GetWorld();
	world->DestroyBody(body);
}

b2Shape *box2d_shape_box(float width, float height) {
	b2PolygonShape *shape = new b2PolygonShape;
	shape->SetAsBox(width, height);
	return shape;
}

/**
 * p1, p2, p3 are the points of the triangle clockwise
 */
b2PolygonShape *box2d_triangle(Point2D p1, Point2D p2, Point2D p3) {
	b2Vec2 vertices[3];

	vertices[0].Set(p1.x, p1.y);
	vertices[1].Set(p2.x, p2.y);
	vertices[2].Set(p3.x, p3.y);

	b2PolygonShape *shape = new b2PolygonShape;
	shape->Set(vertices, sizeof(vertices) / sizeof(*vertices));
	return shape;
}

void box2d_body_create_fixture(b2Body *body, b2Shape *shape, float density,
							   float friction) {
	b2FixtureDef *fixture = new b2FixtureDef;
	fixture->shape		  = shape;
	fixture->density	  = density;
	fixture->friction	  = friction;
	body->CreateFixture(fixture);
}
