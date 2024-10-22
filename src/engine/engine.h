#ifndef _ENGINE_H
#define _ENGINE_H

#include "../util.h"
#include <stdint.h>

#define CHUNK_MAX_X UINT16_MAX
#define CHUNK_MAX_Y UINT16_MAX

#define GENERATE_BLOCK(_x, _y, _block_size, _block)                            \
	for (int __j = _y; __j < _y + _block_size; ++__j) {                        \
		for (int __i = _x; __i < _x + _block_size; ++__i) {                    \
			gameboard[__i][__j] = _block;                                      \
		}                                                                      \
	}

typedef uint32_t seed_t;
typedef union chunk_u {
	seed_t id;
	struct {
		uint16_t y;
		uint16_t x;
	} PACKED;
} Chunk;

#endif /* _ENGINE_H */
