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

GO_ID register_gameobject(GO_Type type, float density, Color color,
						  GO_Draw draw) {

	go_table[go_table_size].type	= type;
	go_table[go_table_size].density = density;
	go_table[go_table_size].color	= color;
	go_table[go_table_size].draw	= draw;

	return (GO_ID){.raw = ++go_table_size};
}

static Color C_VAPOR = {0x7F, 0xFF, 0xFF, 0x69};

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

void init_gameobjects() {
	GO_VAPOR = register_gameobject(GO_GAS, 0.0f, C_VAPOR, NULL);
	GO_WATER = register_gameobject(GO_LIQUID, 1.0f, C_WATER, F_draw_water);
	GO_SAND	 = register_gameobject(GO_POWDER, 2.0f, C_SAND, F_draw_sand);
	GO_STONE = register_gameobject(GO_STATIC, 3.0f, C_STONE, F_draw_stone);
	GO_DIRT	 = register_gameobject(GO_POWDER, 2.0f, C_DIRT, F_draw_dirt);
	GO_OIL	 = register_gameobject(GO_LIQUID, 0.5f, C_OIL, F_draw_oil);
}
