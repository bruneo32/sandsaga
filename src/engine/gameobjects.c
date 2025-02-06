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

static Color C_WATER = {0x7F, 0x8D, 0xFF, 0xAF};

/* Draw pattern for water */
static void F_draw_water(size_t wx, size_t wy, int vx, int vy) {
	const double frequency = 0.03;
	const size_t depth	   = 1;

	const size_t mov = frame_cx / 4;

	/* Generate a pseudo-random value based on Perlin noise */
	double noise_value =
		perlin2d(0, (double)wx + mov, (double)wy + mov, frequency, depth);

	/* Normalize noise value to a range of 0-12 */
	noise_value = fmod(noise_value, 1.0);
	if (noise_value < 0)
		noise_value += 1.0;
	noise_value *= 24.0;

	uint8_t nv = (uint8_t)noise_value;

	Color color = C_WATER;
	color.g += nv;
	color.b -= nv;

	vscreen[vscreen_idx(vx, vy)] = color;
}

void init_gameobjects() {
	GO_VAPOR = register_gameobject(GO_GAS, 0.0f, C_VAPOR, NULL);
	GO_WATER = register_gameobject(GO_LIQUID, 1.0f, C_WATER, F_draw_water);
	GO_SAND	 = register_gameobject(GO_POWDER, 2.0f, C_SAND, F_draw_sand);
	GO_STONE = register_gameobject(GO_STATIC, 3.0f, C_STONE, F_draw_stone);
}
