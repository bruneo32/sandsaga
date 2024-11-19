#ifndef _ENGINE_H
#define _ENGINE_H

#include <stdint.h>

#include "../graphics/graphics.h"
#include "../util.h"
#include "gameobjects.h"

extern bool DEBUG_ON;

extern byte gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

/* 32-bit subchunk_t means 32x32 subchunks filling VSCREEN */
typedef uint32_t subchunk_t;

#define SUBCHUNK_SIZE	(8 * sizeof(subchunk_t))
#define SUBCHUNK_HEIGHT (VSCREEN_HEIGHT / SUBCHUNK_SIZE)
#define SUBCHUNK_WIDTH	(VSCREEN_WIDTH / SUBCHUNK_SIZE)

extern subchunk_t subchunkopt[SUBCHUNK_SIZE];

void set_subchunk(bool on, uint_fast8_t i, uint_fast8_t j);

#define ResetSubchunks                                                         \
	for (uint_fast8_t __j = 0; __j < SUBCHUNK_SIZE; ++__j)                     \
		subchunkopt[__j] = -1;
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

#endif /* _ENGINE_H */
