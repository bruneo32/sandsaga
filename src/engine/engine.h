#ifndef _ENGINE_H
#define _ENGINE_H

#include <stdint.h>

#include "../graphics/graphics.h"
#include "../util.h"
#include "gameobjects.h"

extern byte gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

void update_object(const size_t x, const size_t y);

/* =============================================================== */
/* Chunks */
#define CHUNK_MAX_X UINT16_MAX
#define CHUNK_MAX_Y UINT16_MAX

#define GENERATE_BLOCK(_x, _y, _block_size, _block)                            \
	for (size_t __j = _y; __j < _y + _block_size; ++__j) {                        \
		for (size_t __i = _x; __i < _x + _block_size; ++__i) {                    \
			gameboard[__i][__j] = _block;                                      \
		}                                                                      \
	}

typedef uint32_t seed_t;
typedef uint16_t chunk_axis_t;
typedef union chunk_u {
	seed_t id;
	struct {
		chunk_axis_t y;
		chunk_axis_t x;
	} PACKED;
} Chunk;

void generate_chunk(seed_t SEED, Chunk CHUNK, const size_t vx, const size_t vy);

#endif /* _ENGINE_H */
