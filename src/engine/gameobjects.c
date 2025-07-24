#include "gameobjects.h"

#include <math.h>

#include "../graphics/graphics.h"
#include "noise.h"

size_t	   go_table_size = 0;
GameObject go_table[MAX_GO_ID];

GO_ID GO_VAPOR;
GO_ID GO_WATER;
GO_ID GO_SAND;
GO_ID GO_STONE;
GO_ID GO_DIRT;
GO_ID GO_OIL;
GO_ID GO_ACID;

GO_ID register_gameobject(GO_Type type, float density, Color color,
						  GO_Draw draw, GO_Update update) {

	go_table[go_table_size].type	= type;
	go_table[go_table_size].density = density;
	go_table[go_table_size].color	= color;
	go_table[go_table_size].draw	= draw;
	go_table[go_table_size].update	= update;

	return (GO_ID){.raw = ++go_table_size};
}

static Color C_VAPOR = {0x7F, 0xFF, 0xFF, 0x69};
static Color C_ACID	 = {0x55, 0xFF, 0x55, 0xCD};

static bool F_dynamic_fluid_update(size_t x, size_t y, const bool ltr,
								   const size_t dispersion,
								   const size_t mixability) {
	ssize_t left_or_right = (ltr ? 1 : -1);

	GO_ID *boardxy	 = &gameboard[y][x];
	boardxy->updated = 1; /* IMPORTANT! Set updated bit */

	GO_ID	   gobj_id = *boardxy;
	GameObject gobj	   = GOBJECT(gobj_id);

	/* Move down if possible */
	size_t down_y = y + 1;
	GO_ID *bottom = &gameboard[down_y][x];
	if (IS_IN_BOUNDS_V(down_y) && bottom->raw == GO_NONE.raw) {
		bottom->id		= gobj_id.id;
		bottom->updated = 1;
		boardxy->raw	= GO_NONE.raw;
		subchunk_set_world(x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move left if possible (or right if ltr is false) */
	ssize_t left_x = x;
	for (size_t i = 0; i <= dispersion; i++) {
		const ssize_t new_left_x = left_x - left_or_right;
		if (!IS_IN_BOUNDS_H(new_left_x))
			break;

		GO_ID left = gameboard[y][new_left_x];
		if (left.updated)
			break;

		if (left.raw != GO_NONE.raw) {
			/* Try to push it if density is not greater */
			GameObject *go_left = &GOBJECT(left);
			if (!go_left->update || go_left->density > gobj.density)
				break;

			go_left->update(new_left_x, y, ltr);

			/* If it's still there, break */
			if (gameboard[y][new_left_x].raw != GO_NONE.raw)
				break;
		}

		left_x = new_left_x;
	}

	if (left_x != x) {
		GO_ID *left	  = &gameboard[y][left_x];
		left->id	  = gobj_id.id;
		left->updated = 1;
		boardxy->raw  = GO_NONE.raw;
		subchunk_set_world(left_x, y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Probability to mix with other objects */
	ssize_t left_x2 = x - left_or_right;
	GO_ID  *left	= &gameboard[y][left_x2];
	if (left->raw != GO_NONE.raw && left->updated &&
		(fast_rand() % mixability) == 0 &&
		GOBJECT(*left).density < gobj.density) {
		left->updated = 1;
		SWAP(left->raw, boardxy->raw);
		subchunk_set_world(left_x2, y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move right if possible (or left if ltr is false) */
	ssize_t right_x = x;
	for (size_t i = 0; i <= dispersion; i++) {
		const ssize_t new_right_x = right_x + left_or_right;
		if (!IS_IN_BOUNDS_H(new_right_x))
			break;

		GO_ID right = gameboard[y][new_right_x];
		if (right.updated)
			break;

		if (right.raw != GO_NONE.raw) {
			/* Try to push it if density is not greater */
			GameObject *go_right = &GOBJECT(right);
			if (!go_right->update || go_right->density > gobj.density)
				break;

			go_right->update(new_right_x, y, ltr);

			/* If it's still there, break */
			if (gameboard[y][new_right_x].raw != GO_NONE.raw)
				break;
		}

		right_x = new_right_x;
	}

	if (right_x != x) {
		GO_ID *right   = &gameboard[y][right_x];
		right->id	   = gobj_id.id;
		right->updated = 1;
		boardxy->raw   = GO_NONE.raw;
		subchunk_set_world(right_x, y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Probability to mix with other objects */
	ssize_t right_x2 = x + left_or_right;
	GO_ID  *right	 = &gameboard[y][right_x2];
	if (right->raw != GO_NONE.raw && right->updated &&
		(fast_rand() % mixability) == 0 &&
		GOBJECT(*right).density < gobj.density) {
		right->updated = 1;
		SWAP(right->raw, boardxy->raw);
		subchunk_set_world(right_x2, y);
		subchunk_set_world(x, y);
	}

	/* Move bottom-left if possible (or right if ltr is false) */
	GO_ID *bottomleft = &gameboard[down_y][left_x];
	if (IS_IN_BOUNDS(left_x, down_y) &&
		(bottomleft->raw == GO_NONE.raw ||
		 ((fast_rand() % mixability) == 0 &&
		  GOBJECT(*bottomleft).density < gobj.density))) {
		if (bottomleft->raw != GO_NONE.raw)
			bottomleft->updated = 1;
		SWAP(bottomleft->raw, boardxy->raw);
		subchunk_set_world(left_x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move bottom-right if possible (or left if ltr is false) */
	GO_ID *bottomright = &gameboard[down_y][right_x];
	if (IS_IN_BOUNDS(right_x, down_y) &&
		(bottomright->raw == GO_NONE.raw ||
		 ((fast_rand() % mixability) == 0 &&
		  GOBJECT(*bottomright).density < gobj.density))) {
		if (bottomright->raw != GO_NONE.raw)
			bottomright->updated = 1;
		SWAP(bottomright->raw, boardxy->raw);
		subchunk_set_world(right_x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	return false;
}

static bool F_dynamic_gas_update(size_t x, size_t y, const bool ltr,
								 const size_t dispersion,
								 const size_t mixability) {
	ssize_t left_or_right = (ltr ? 1 : -1);

	GO_ID *boardxy	 = &gameboard[y][x];
	boardxy->updated = 1; /* IMPORTANT! Set updated bit */

	GO_ID	   gobj_id = *boardxy;
	GameObject gobj	   = GOBJECT(gobj_id);

	/* Move down if possible */
	size_t up_y = y - 1;
	GO_ID *top	= &gameboard[up_y][x];
	if (IS_IN_BOUNDS_V(up_y) && top->raw == GO_NONE.raw) {
		top->id		 = gobj_id.id;
		top->updated = 1;
		boardxy->raw = GO_NONE.raw;
		subchunk_set_world(x, up_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move left if possible (or right if ltr is false) */
	ssize_t left_x = x;
	for (size_t i = 0; i <= dispersion; i++) {
		const ssize_t new_left_x = left_x - left_or_right;
		if (!IS_IN_BOUNDS_H(new_left_x))
			break;

		GO_ID left = gameboard[y][new_left_x];
		if (left.updated)
			break;

		if (left.raw != GO_NONE.raw) {
			/* Try to push it if density is not greater */
			GameObject *go_left = &GOBJECT(left);
			if (!go_left->update || go_left->density > gobj.density)
				break;

			go_left->update(new_left_x, y, ltr);

			/* If it's still there, break */
			if (gameboard[y][new_left_x].raw != GO_NONE.raw)
				break;
		}

		left_x = new_left_x;
	}

	if (left_x != x) {
		GO_ID *left	  = &gameboard[y][left_x];
		left->id	  = gobj_id.id;
		left->updated = 1;
		boardxy->raw  = GO_NONE.raw;
		subchunk_set_world(left_x, y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Probability to mix with other objects */
	ssize_t left_x2 = x - left_or_right;
	GO_ID  *left	= &gameboard[y][left_x2];
	if (left->raw != GO_NONE.raw && left->updated &&
		(fast_rand() % mixability) == 0 &&
		GOBJECT(*left).density < gobj.density) {
		left->updated = 1;
		SWAP(left->raw, boardxy->raw);
		subchunk_set_world(left_x2, y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move right if possible (or left if ltr is false) */
	ssize_t right_x = x;
	for (size_t i = 0; i <= dispersion; i++) {
		const ssize_t new_right_x = right_x + left_or_right;
		if (!IS_IN_BOUNDS_H(new_right_x))
			break;

		GO_ID right = gameboard[y][new_right_x];
		if (right.updated)
			break;

		if (right.raw != GO_NONE.raw) {
			/* Try to push it if density is not greater */
			GameObject *go_right = &GOBJECT(right);
			if (!go_right->update || go_right->density > gobj.density)
				break;

			go_right->update(new_right_x, y, ltr);

			/* If it's still there, break */
			if (gameboard[y][new_right_x].raw != GO_NONE.raw)
				break;
		}

		right_x = new_right_x;
	}

	if (right_x != x) {
		GO_ID *right   = &gameboard[y][right_x];
		right->id	   = gobj_id.id;
		right->updated = 1;
		boardxy->raw   = GO_NONE.raw;
		subchunk_set_world(right_x, y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Probability to mix with other objects */
	ssize_t right_x2 = x + left_or_right;
	GO_ID  *right	 = &gameboard[y][right_x2];
	if (right->raw != GO_NONE.raw && right->updated &&
		(fast_rand() % mixability) == 0 &&
		GOBJECT(*right).density < gobj.density) {
		right->updated = 1;
		SWAP(right->raw, boardxy->raw);
		subchunk_set_world(right_x2, y);
		subchunk_set_world(x, y);
	}

	/* Move bottom-left if possible (or right if ltr is false) */
	GO_ID *topleft = &gameboard[up_y][left_x];
	if (IS_IN_BOUNDS(left_x, up_y) &&
		(topleft->raw == GO_NONE.raw ||
		 ((fast_rand() % mixability) == 0 &&
		  GOBJECT(*topleft).density < gobj.density))) {
		if (topleft->raw != GO_NONE.raw)
			topleft->updated = 1;
		SWAP(topleft->raw, boardxy->raw);
		subchunk_set_world(left_x, up_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move bottom-right if possible (or left if ltr is false) */
	GO_ID *topright = &gameboard[up_y][right_x];
	if (IS_IN_BOUNDS(right_x, up_y) &&
		(topright->raw == GO_NONE.raw ||
		 ((fast_rand() % mixability) == 0 &&
		  GOBJECT(*topright).density < gobj.density))) {
		if (topright->raw != GO_NONE.raw)
			topright->updated = 1;
		SWAP(topright->raw, boardxy->raw);
		subchunk_set_world(right_x, up_y);
		subchunk_set_world(x, y);
		return true;
	}

	return false;
}

static Color C_SAND	 = {0xE2, 0xDB, 0xA4, 0xFF};
static Color C_SAND2 = {0xFB, 0xF4, 0xBD, 0xFF};
static Color C_SAND3 = {0xEA, 0xE3, 0xAD, 0xFF};
static Color C_SAND4 = {0xC1, 0xC4, 0x97, 0xFF};

/* Draw pattern for sand */
static void F_draw_sand(size_t wx, size_t wy, int vx, int vy) {
	/* Pseudo-random seed based only on world coordinates */
	const size_t seed = wy - (wx ^ wy);

	/* noise2 is way faster than perlin2d, and is very good for this case */
	size_t noise = noise2(wx, wy, seed);

	/* Map noise value to a specific color */
	const size_t idx = vscreen_idx(vx, vy);
	if (noise < 6)
		vscreen[idx] = C_LTGRAY;
	else if (noise < 12)
		vscreen[idx] = C_SAND4;
	else if (noise < 64)
		vscreen[idx] = C_SAND3;
	else if (noise < 128)
		vscreen[idx] = C_SAND2;
	else
		vscreen[idx] = C_SAND;
}

static bool F_update_sand(size_t x, size_t y, const bool ltr) {
	ssize_t left_or_right = (ltr ? 1 : -1);

	GO_ID *boardxy	 = &gameboard[y][x];
	boardxy->updated = 1; /* IMPORTANT! Set updated bit */

	GO_ID gobj_id = *boardxy;

	size_t down_y = y + 1;

	if (!IS_IN_BOUNDS(x, down_y))
		return false;

	/* Move down if possible */
	GO_ID *bottom = &gameboard[down_y][x];
	if (bottom->raw == GO_NONE.raw) {
		bottom->id		= gobj_id.id;
		bottom->updated = 1;
		boardxy->raw	= GO_NONE.raw;
		subchunk_set_world(x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* 50% chance to stop and don't move, note that this is for sink too */
	if ((fast_rand() % 2) == 0)
		return false;

	/* Move bottom-left if possible (or right if ltr is false) */
	size_t left_x	  = x - left_or_right;
	GO_ID *bottomleft = &gameboard[down_y][left_x];
	if (IS_IN_BOUNDS_H(left_x) && bottomleft->raw == GO_NONE.raw) {
		bottomleft->id		= gobj_id.id;
		bottomleft->updated = 1;
		boardxy->raw		= GO_NONE.raw;
		subchunk_set_world(left_x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move bottom-right if possible (or left if ltr is false) */
	size_t right_x	   = x + left_or_right;
	GO_ID *bottomright = &gameboard[down_y][right_x];
	if (IS_IN_BOUNDS_H(right_x) && bottomright->raw == GO_NONE.raw) {
		bottomright->id		 = gobj_id.id;
		bottomright->updated = 1;
		boardxy->raw		 = GO_NONE.raw;
		subchunk_set_world(right_x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Sink: Flow down in less dense fluids */
	GameObject *gobj	  = &GOBJECT(gobj_id);
	GameObject *go_bottom = &GOBJECT(*bottom);
	if (GO_IS_FLUID(go_bottom->type) && go_bottom->density < gobj->density) {
		bottom->updated = 1;
		SWAP(bottom->raw, boardxy->raw);
		subchunk_set_world(x, y);
		subchunk_set_world(x, down_y);
		return true;
	}

	return false;
}

static Color C_DIRT	 = {0x54, 0x43, 0x39, 0xFF};
static Color C_DIRT3 = {0x3B, 0x28, 0x1A, 0xFF};
static Color C_DIRT2 = {0x4D, 0x3C, 0x32, 0xFF};
static Color C_DIRT4 = {0x26, 0x14, 0x0A, 0xFF};
static Color C_DIRT5 = {0x80, 0x6A, 0x5F, 0xFF};

/* Draw pattern for dirt */
static void F_draw_dirt(size_t wx, size_t wy, int vx, int vy) {
	/* Pseudo-random seed based only on world coordinates */
	const size_t seed = wy - (wx ^ -wy);

	/* noise2 is way faster than perlin2d, and is very good for this case */
	size_t noise = noise2(wx, wy, seed);

	/* Map noise value to a specific color */
	const size_t idx = vscreen_idx(vx, vy);
	if (noise < 12)
		vscreen[idx] = C_DIRT5;
	else if (noise < 32)
		vscreen[idx] = C_DIRT4;
	else if (noise < 96)
		vscreen[idx] = C_DIRT3;
	else if (noise < 192)
		vscreen[idx] = C_DIRT2;
	else
		vscreen[idx] = C_DIRT;
}

static bool F_update_dirt(size_t x, size_t y, const bool ltr) {
	ssize_t left_or_right = (ltr ? 1 : -1);

	GO_ID *boardxy	 = &gameboard[y][x];
	boardxy->updated = 1; /* IMPORTANT! Set updated bit */

	GO_ID gobj_id = *boardxy;

	size_t down_y = y + 1;

	if (!IS_IN_BOUNDS(x, down_y))
		return false;

	/* Move down if possible */
	GO_ID *bottom = &gameboard[down_y][x];
	if (bottom->raw == GO_NONE.raw) {
		bottom->id		= gobj_id.id;
		bottom->updated = 1;
		boardxy->raw	= GO_NONE.raw;
		subchunk_set_world(x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move bottom-left if possible (or right if ltr is false) */
	size_t left_x	  = x - left_or_right;
	GO_ID *bottomleft = &gameboard[down_y][left_x];
	if (IS_IN_BOUNDS_H(left_x) && bottomleft->raw == GO_NONE.raw) {
		bottomleft->id		= gobj_id.id;
		bottomleft->updated = 1;
		boardxy->raw		= GO_NONE.raw;
		subchunk_set_world(left_x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* Move bottom-right if possible (or left if ltr is false) */
	size_t right_x	   = x + left_or_right;
	GO_ID *bottomright = &gameboard[down_y][right_x];
	if (IS_IN_BOUNDS_H(right_x) && bottomright->raw == GO_NONE.raw) {
		bottomright->id		 = gobj_id.id;
		bottomright->updated = 1;
		boardxy->raw		 = GO_NONE.raw;
		subchunk_set_world(right_x, down_y);
		subchunk_set_world(x, y);
		return true;
	}

	/* 30% chance to stop and don't sink */
	if ((fast_rand() % 3) != 0)
		return false;

	/* Sink: Flow down in less dense fluids */
	GameObject *gobj	  = &GOBJECT(gobj_id);
	GameObject *go_bottom = &GOBJECT(*bottom);
	if (GO_IS_FLUID(go_bottom->type) && go_bottom->density < gobj->density) {
		bottom->updated = 1;
		SWAP(bottom->raw, boardxy->raw);
		subchunk_set_world(x, y);
		subchunk_set_world(x, down_y);
		return true;
	}

	return false;
}

static Color C_STONE  = {0x79, 0x7B, 0x7A, 0xFF};
static Color C_STONE2 = {0x76, 0x78, 0x77, 0xFF};
static Color C_STONE4 = {0x7D, 0x7F, 0x7E, 0xFF};
static Color C_STONE3 = {0x73, 0x75, 0x74, 0xFF};

/* Draw pattern for stone */
static void F_draw_stone(size_t wx, size_t wy, int vx, int vy) {
	size_t noise = noise2(wx, wy, 0);

	/* Map noise value to a specific color */
	const size_t idx = vscreen_idx(vx, vy);
	if (noise < 24)
		vscreen[idx] = C_STONE4;
	else if (noise < 48)
		vscreen[idx] = C_STONE3;
	else if (noise < 128)
		vscreen[idx] = C_STONE2;
	else
		vscreen[idx] = C_STONE;
}

static Color C_WATER = {0x47, 0x77, 0xDC, 0xA4};

/* Draw pattern for water */
static void F_draw_water(size_t wx, size_t wy, int vx, int vy) {
#define WATER_SEED 0
	const double freq	= 0.02;
	const double freq2	= 0.005;
	const size_t depth	= 3;
	const double o_time = frame_cx >> 1;

	const double px = 49.0 * perlin2d(WATER_SEED, (double)(wx + o_time),
									  (double)(wy + o_time), freq2, 1);
	const double py = 47.0 * perlin2d(WATER_SEED, (double)(wx - o_time),
									  (double)(wy - o_time), freq2, 1);

	double noise_value =
		perlin2d(WATER_SEED, (double)wx + px, (double)wy + py, freq, depth);

	/* Normalize noise value to [-1, 1], make rings using absolute value [0,1],
	 * then increase contrast between ring line and it's center using pow^2, and
	 * scale to 32.0 */
	noise_value = 32.0 * pow((fabs(fmod(noise_value + 1.5, 2.0) - 1.0)), 2.0);

	uint8_t nv = (uint8_t)noise_value;

	Color color = C_WATER;
	color.r += nv;
	color.g += nv;
	color.b += nv;
	color.a += nv;

	vscreen[vscreen_idx(vx, vy)] = color;
}

static bool F_update_water(size_t x, size_t y, const bool ltr) {
	return F_dynamic_fluid_update(x, y, ltr, fast_rand() % 3 + 1, 4);
}

static Color C_OIL = {0x92, 0x32, 0x23, 0xCD};

/* Draw pattern for oil */
static void F_draw_oil(size_t wx, size_t wy, int vx, int vy) {
#define OIL_SEED 420
	const double freq  = 0.08;
	const double freq2 = 0.02;
	const size_t depth = 1;

	const size_t o_time = frame_cx >> 2;

	const double px = 49.0 * perlin2d(OIL_SEED, (double)(wx - o_time),
									  (double)(wy + o_time), freq2 / 2.0, 1);
	const double py = 23.0 * perlin2d(OIL_SEED, (double)(wx + o_time),
									  (double)(wy - o_time), freq2, 1);

	/* Generate a pseudo-random value based on Perlin noise */
	double noise_value =
		perlin2d(OIL_SEED, (double)wx + px, (double)wy + py, freq, depth);

	/* Normalize noise value to [-1, 1] and scale to 18.0 */
	noise_value = 18.0 * (fabs(fmod(noise_value + 1.5, 2.0) - 1.0));

	uint8_t nv = (uint8_t)noise_value;

	Color color = C_OIL;
	color.r += nv;
	color.g += nv >> 1;
	color.a -= nv;

	vscreen[vscreen_idx(vx, vy)] = color;
}

static bool F_update_oil(size_t x, size_t y, const bool ltr) {
	return F_dynamic_fluid_update(x, y, ltr, fast_rand() % 2, 16);
}

static bool F_update_vapor(size_t x, size_t y, const bool ltr) {
	return F_dynamic_gas_update(x, y, ltr, fast_rand() % 3 + 1, 4);
}

static bool F_update_acid(size_t x, size_t y, const bool ltr) {
	const size_t chance_percent = 50;

	/* Probability to eat through */
	for (;;) {
		if (fast_rand() % chance_percent != 0)
			break;

		GO_ID *boardxy = &gameboard[y][x];

		/* Choose random direction to eat */
		ssize_t ll = 0, rr = 0;
		while (ll == 0 && rr == 0) {
			ll = fast_rand() % 3 - 1;
			rr = fast_rand() % 3 - 1;
		}

		/* Destroy that direction */
		if (!IS_IN_BOUNDS(x + rr, y + ll))
			break;

		GO_ID *target = &gameboard[y + ll][x + rr];
		if (target->id == GO_NONE.id || target->id == GO_ACID.id)
			break; /* Move (exit for) */

		target->raw = GO_NONE.raw;
		subchunk_set_world(x + rr, y + ll);

		/* Self destroy */
		boardxy->raw = GO_NONE.raw;
		subchunk_set_world(x, y);
		return true;
	}

	/* If didn't eat, move normally */
	return F_dynamic_fluid_update(x, y, ltr, fast_rand() % 2 + 1, 2);
}

void init_gameobjects() {
	GO_VAPOR = register_gameobject(GO_GAS, 0.0f, C_VAPOR, NULL, F_update_vapor);
	GO_WATER = register_gameobject(GO_LIQUID, 1.0f, C_WATER, F_draw_water,
								   F_update_water);
	GO_SAND	 = register_gameobject(GO_POWDER, 2.0f, C_SAND, F_draw_sand,
								   F_update_sand);
	GO_STONE =
		register_gameobject(GO_STATIC, 3.0f, C_STONE, F_draw_stone, NULL);
	GO_DIRT = register_gameobject(GO_POWDER, 2.0f, C_DIRT, F_draw_dirt,
								  F_update_dirt);
	GO_OIL =
		register_gameobject(GO_LIQUID, 0.9f, C_OIL, F_draw_oil, F_update_oil);
	GO_ACID =
		register_gameobject(GO_LIQUID, 1.18f, C_ACID, NULL, F_update_acid);
}
