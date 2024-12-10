#ifndef _ENGINE_H
#define _ENGINE_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../physics/physics.h"

#include "../graphics/graphics.h"
#include "../util.h"
#include "gameobjects.h"

extern size_t WORLD_SEED;
extern bool	  DEBUG_ON;

extern byte gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

/* 16-bit subchunk_t means 16x16 subchunks filling VSCREEN */
typedef uint16_t subchunk_t;

#define SUBCHUNK_SIZE	(8 * sizeof(subchunk_t))
#define SUBCHUNK_HEIGHT (VSCREEN_HEIGHT / SUBCHUNK_SIZE)
#define SUBCHUNK_WIDTH	(VSCREEN_WIDTH / SUBCHUNK_SIZE)

extern subchunk_t subchunkopt[SUBCHUNK_SIZE];
extern b2Body	 *soil_body[SUBCHUNK_SIZE][SUBCHUNK_SIZE];

void set_subchunk(bool on, uint_fast8_t i, uint_fast8_t j);

#define ResetSubchunks                                                         \
	for (uint_fast8_t __j = 0; __j < SUBCHUNK_SIZE; ++__j) {                   \
		subchunkopt[__j] = -1;                                                 \
		for (uint_fast8_t __i = 0; __i < SUBCHUNK_SIZE; ++__i)                 \
			soil_body[__j][__i] = NULL;                                        \
	}
#define set_subchunk_world(_on, _x, _y)                                        \
	set_subchunk(_on, (_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)
#define is_subchunk_active(_i, _j) (0 != (subchunkopt[(_j)] & BIT(_i)))
#define is_subchunk_active_world(_x, _y)                                       \
	is_subchunk_active((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)

/** Returns true if any object was updated, false otherwise */
bool update_object(const size_t x, const size_t y);
void update_gameboard();
void draw_gameboard_world(const SDL_Rect *camera);

/* =============================================================== */
/* Chunks */
#define CHUNK_MAX_X UINT16_MAX
#define CHUNK_MAX_Y UINT16_MAX

typedef uint32_t seed_t;
typedef uint16_t chunk_axis_t;
typedef union chunk_u {
	seed_t id;
	struct {
		chunk_axis_t y;
		chunk_axis_t x;
	} PACKED;
} Chunk;

#define GEN_WATERSEA_OFFSET_X 128
#define GEN_SKY_Y			  512
#define GEN_TOP_LAYER_Y		  1024
#define GEN_BEDROCK_MARGIN_Y  512
void generate_chunk(seed_t SEED, Chunk CHUNK, const size_t vx, const size_t vy);

/* =============================================================== */
/* Box2D world */
extern b2World *b2_world;

void activate_soil(size_t si, size_t sj, bool force);
void deactivate_soil(size_t si, size_t sj);
#define activate_soil_world(_x, _y, _f)                                        \
	activate_soil((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT, _f)
#define deactivate_soil_world(_x, _y)                                          \
	deactivate_soil((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)

#endif /* _ENGINE_H */
