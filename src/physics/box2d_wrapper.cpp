#include "../graphics/graphics.h"
#include "physics.h"
#include <iostream>
#include <unordered_set>
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
							F2B(color.a * 0.5f));

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
	void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) {
		Render_SetcolorRGBA(F2B(color.r), F2B(color.g), F2B(color.b),
							F2B(color.a * 0.5f));
		Render_Ellipse((int)U_TO_X(radius), (int)U_TO_X(radius),
					   (int)U_TO_X(center.x) - renderCamera->x,
					   (int)U_TO_X(center.y) - renderCamera->y);
	}

	/// Draw a solid circle.
	void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis,
						 const b2Color &color) {
		DrawCircle(center, radius, color);
	}

	/// Draw a line segment.
	void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) {
		Render_SetcolorRGBA(F2B(color.r), F2B(color.g), F2B(color.b),
							F2B(color.a * 0.5f));
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
							F2B(color.a * 0.5f));
		Render_Pixel((int)U_TO_X(p.x), (int)U_TO_X(p.y));
	}
};
DebugDraw debug_draw;

std::unordered_set<b2Body *> bodiesMarkedToDestroy;

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

	/* After stepping, you should clear any forces you have applied to your
	 * bodies. This lets you take multiple sub-steps with the same force
	 * field. */
	world->ClearForces();

	if (!world->IsLocked()) {
		/* Remove marked bodies */
		for (b2Body *body : bodiesMarkedToDestroy) {
			world->DestroyBody(body);
			bodiesMarkedToDestroy.extract(body);
		}

		/* Remove bodies outside the law */
		for (b2Body *body = world->GetBodyList(); body != NULL;
			 body		  = body->GetNext()) {
			b2Vec2 pos = body->GetPosition();
			if (!B2_IS_IN_BOUNDS(pos.x, pos.y))
				world->DestroyBody(body);
		}
	}
}

void box2d_world_destroy(b2World *world) { delete world; }

void box2d_debug_draw_active(bool on) {
	if (!on) {
		/* Clear all flags */
		debug_draw.ClearFlags((uint32_t)-1);
	} else {
		/* Set all flags */
		debug_draw.SetFlags(
			b2Draw::e_shapeBit | b2Draw::e_jointBit
			/* | b2Draw::e_pairBit | b2Draw::e_centerOfMassBit | b2Draw::e_aabbBit*/);
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

void box2d_world_move_all_bodies(b2World *world, float u, float v) {
	for (b2Body *body = world->GetBodyList(); body != NULL;
		 body		  = body->GetNext()) {
		b2Vec2 pos = body->GetPosition();
		pos.x += u;
		pos.y += v;
		body->SetTransform(pos, body->GetAngle());
	}
}

/* =============================================================== */
/* Box2D Body functions */
b2Body *box2d_body_create(b2World *world, float u, float v, int body_type,
						  bool allowSleep, bool discrete_collision) {
	b2BodyDef bodyDef;
	bodyDef.type = static_cast<b2BodyType>(body_type);
	bodyDef.position.Set(u, v);
	bodyDef.bullet	   = !discrete_collision;
	bodyDef.allowSleep = allowSleep;
	return world->CreateBody(&bodyDef);
}

void box2d_body_change_type(b2Body *body, int body_type) {
	body->SetType(static_cast<b2BodyType>(body_type));
}

bool box2d_body_get_fixed_rotation(b2Body *body) {
	return body->IsFixedRotation();
}

void box2d_body_set_fixed_rotation(b2Body *body, bool fixed_rotation) {
	body->SetFixedRotation(fixed_rotation);
}

void box2d_body_set_position(b2Body *body, float u, float v) {
	body->SetTransform(b2Vec2(u, v), body->GetAngle());
}

float box2d_body_get_angle(b2Body *body) { return body->GetAngle(); }

void box2d_body_set_angle(b2Body *body, float angle) {
	b2Vec2 pos = body->GetPosition();
	body->SetTransform(pos, angle);
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
	if (!world)
		return;

	if (!world->IsLocked()) {
		world->DestroyBody(body);
	} else {
		bodiesMarkedToDestroy.insert(body);
	}
}

b2Shape *box2d_shape_box(float width, float height, float x, float y) {
	b2PolygonShape *shape = new b2PolygonShape;
	b2Vec2			centroid;
	centroid.Set(x, y);
	shape->SetAsBox(width, height, centroid, 0.0f);
	return shape;
}

b2Shape *box2d_shape_circle(float radius, float x, float y) {
	b2CircleShape *shape = new b2CircleShape;
	shape->m_radius		 = radius;
	shape->m_p.Set(x, y);
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

b2ChainShape *box2d_shape_loop(Point2D *points, unsigned int count) {
	if (count < 3)
		return NULL;

	b2ChainShape *shape = new b2ChainShape;

	constexpr float minDistanceSquared = b2_linearSlop * b2_linearSlop;

	std::vector<b2Vec2> vertices;
	vertices.emplace_back(points[0].x, points[0].y); /* Add the first vertex */

	for (size_t i = 1; i < count; i++) {
		b2Vec2 currentVertex(points[i].x, points[i].y);

		if (b2DistanceSquared(vertices.back(), currentVertex) >
			minDistanceSquared)
			vertices.push_back(currentVertex); /* Add valid vertices */
	}

	/* Close the loop by ensuring the last vertex connects to the first */
	if (b2DistanceSquared(vertices.back(), vertices.front()) <=
		minDistanceSquared) {
		/* Remove the last vertex if it's too close to the first one */
		vertices.pop_back();
	}

	if (vertices.size() < 3) {
		delete shape;
		return NULL;
	}

	shape->CreateLoop(vertices.data(), vertices.size());
	return shape;
}

b2Fixture *box2d_body_create_fixture(b2Body *body, b2Shape *shape,
									 float density, float friction) {
	b2FixtureDef *fixture = new b2FixtureDef;
	fixture->shape		  = shape;
	fixture->density	  = density;
	fixture->friction	  = friction;
	return body->CreateFixture(fixture);
}

/* =============================================================== */
/* Box2D Raycasts */
class RaycastCallback : public b2RayCastCallback {
  public:
	RaycastData data;
	b2Body	   *owner;

	RaycastCallback() {
		data.closestFraction = 1.0f;
		data.hit			 = false;
	}

	/* Called for each fixture the ray intersects */
	float ReportFixture(b2Fixture *fixture, const b2Vec2 &point,
						const b2Vec2 &normal, float fraction) override {
		if (fixture->GetBody() == owner)
			return -1.0f; /* Skip this fixture */

		/* Store the closest hit */
		if (fraction < data.closestFraction) {
			data.closestFraction = fraction;
			data.point_x		 = point.x;
			data.point_y		 = point.y;
			data.normal_x		 = normal.x;
			data.normal_y		 = normal.y;
			data.hit			 = true;
			data.other			 = fixture;
		}

		/* Continue raycasting (return fraction to find closest) */
		return fraction;
	}
};

void box2d_raycast(b2World *world, RaycastData *output, float u1, float v1,
				   float u2, float v2, b2Body *owner) {
	if (!output)
		return;

	/* Define the ray's start and end points */
	b2Vec2 rayStart = b2Vec2(u1, v1);
	b2Vec2 rayEnd	= b2Vec2(u2, v2);

	/* Perform the raycast */
	RaycastCallback raycast;
	raycast.owner = owner;
	world->RayCast(&raycast, rayStart, rayEnd);

	/* Store the results */
	output->closestFraction = raycast.data.closestFraction;
	output->point_x			= raycast.data.point_x;
	output->point_y			= raycast.data.point_y;
	output->normal_x		= raycast.data.normal_x;
	output->normal_y		= raycast.data.normal_y;
	output->hit				= raycast.data.hit;
	output->other			= raycast.data.other;
}

void box2d_body_raycast(b2Body *body, RaycastData *output, float u, float v) {
	b2World *world = body->GetWorld();

	float u1 = body->GetPosition().x;
	float v1 = body->GetPosition().y;
	float u2 = u1 + u;
	float v2 = v1 + v;

	box2d_raycast(world, output, u1, v1, u2, v2, body);
}

void box2d_sweep_raycast(b2Body *body, RaycastData *output, int numRays,
						 float sweepLength, float rayDirX, float rayDirY,
						 bool horizontalSweep) {
	b2World *world	  = body->GetWorld();
	b2Vec2	 position = body->GetPosition();

	/* Initialize the output to store the closest hit among all rays */
	output->closestFraction = 1.0f;
	output->hit				= false;

	float halfSweep = sweepLength / 2.0f;
	float delta = sweepLength / (float)(numRays - 1); /* Spacing between rays */

	for (int i = 0; i < numRays; i++) {
		float  offset = -halfSweep + i * delta; /* Offset for the current ray */
		b2Vec2 rayStart, rayEnd;

		if (horizontalSweep) {
			/* Horizontal sweeping: rays along the X-axis */
			rayStart = position + b2Vec2(offset, 0);
		} else {
			/* Vertical sweeping: rays along the Y-axis */
			rayStart = position + b2Vec2(0, offset);
		}

		/* Ray direction (defined by rayDirX, rayDirY) */
		b2Vec2 rayDelta = b2Vec2(rayDirX, rayDirY);
		rayEnd			= rayStart + rayDelta;

		/* Perform raycast for this ray */
		RaycastCallback raycast;
		raycast.owner = body;
		world->RayCast(&raycast, rayStart, rayEnd);

		/* Update the overall result with the closest hit */
		if (raycast.data.hit &&
			raycast.data.closestFraction < output->closestFraction) {
			*output = raycast.data; /* Copy the closest hit data */
		}
	}
}
