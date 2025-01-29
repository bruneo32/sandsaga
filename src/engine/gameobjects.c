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
						  void (*draw)(size_t wx, size_t wy, int vx, int vy)) {

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
	if (noise < 6)
		Render_Setcolor(C_LTGRAY);
	else if (noise < 12)
		Render_Setcolor(C_SAND4);
	else if (noise < 64)
		Render_Setcolor(C_SAND3);
	else if (noise < 128)
		Render_Setcolor(C_SAND2);
	else
		Render_Setcolor(C_SAND);

	Render_Pixel(vx, vy);
}

static Color C_STONE  = {0x79, 0x7B, 0x7A, 0xFF};
static Color C_STONE2 = {0x76, 0x78, 0x77, 0xFF};
static Color C_STONE4 = {0x7D, 0x7F, 0x7E, 0xFF};
static Color C_STONE3 = {0x73, 0x75, 0x74, 0xFF};

/* Draw pattern for stone */
static void F_draw_stone(size_t wx, size_t wy, int vx, int vy) {
	size_t noise = noise2(wx, wy, 0);

	/* Map noise value to a specific color */
	if (noise < 24)
		Render_Setcolor(C_STONE4);
	else if (noise < 48)
		Render_Setcolor(C_STONE3);
	else if (noise < 128)
		Render_Setcolor(C_STONE2);
	else
		Render_Setcolor(C_STONE);

	Render_Pixel(vx, vy);
}

#define C_WATER_DATA 0x7F, 0x9D, 0xFF, 0xAF
static Color C_WATER = {C_WATER_DATA};

/* Draw pattern for water */
static void F_draw_water(size_t wx, size_t wy, int vx, int vy) {
	Color color = {C_WATER_DATA};

	const double frequency = 0.03;
	const size_t depth	   = 1;

	/* Generate a pseudo-random value based on Perlin noise */
	double noise_value = perlin2d(0, (double)wx, (double)wy, frequency, depth);

	/* Normalize noise value to a range of 0-12 */
	noise_value = fmod(noise_value, 1.0);
	if (noise_value < 0)
		noise_value += 1.0;
	noise_value *= 12;

	uint8_t nv = (uint8_t)noise_value;
	color.g += nv;
	color.b -= nv / 2;

	Render_Setcolor(color);
	Render_Pixel(vx, vy);
}

void init_gameobjects() {
	GO_VAPOR = register_gameobject(GO_GAS, 0.0f, C_VAPOR, NULL);
	GO_WATER = register_gameobject(GO_LIQUID, 1.0f, C_WATER, F_draw_water);
	GO_SAND	 = register_gameobject(GO_POWDER, 2.0f, C_SAND, F_draw_sand);
	GO_STONE = register_gameobject(GO_STATIC, 3.0f, C_STONE, F_draw_stone);
}
