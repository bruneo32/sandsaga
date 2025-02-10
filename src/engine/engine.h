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
extern size_t frame_cx;

enum e_dbgl /* : int */ {
	e_dbgl_none	   = 0x1,
	e_dbgl_ui	   = 0x1 << 1,
	e_dbgl_engine  = 0x1 << 2,
	e_dbgl_physics = 0x1 << 3,
};
#define DBGL(dbgl) ((DEBUG_LEVEL & dbgl) != 0)

extern int DEBUG_LEVEL;

extern GO_ID gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

extern Color vscreen[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];
#define vscreen_idx(x, y) (((y) * VIEWPORT_WIDTH) + (x))
#define vscreen_line_size (VIEWPORT_WIDTH * sizeof(Color))

/* 16-bit subchunk_t means 16x16 subchunks filling VSCREEN */
typedef uint16_t subchunk_t;

#define SUBCHUNK_SIZE	(8 * sizeof(subchunk_t))
#define SUBCHUNK_HEIGHT (VSCREEN_HEIGHT / SUBCHUNK_SIZE)
#define SUBCHUNK_WIDTH	(VSCREEN_WIDTH / SUBCHUNK_SIZE)

extern subchunk_t subchunkopt[SUBCHUNK_SIZE];
#define SUBCHUNK_ROW_COMPLETE ((subchunk_t)~0)

typedef struct _SoilData {
	b2Body *body;
} SoilData;
extern SoilData soil_body[SUBCHUNK_SIZE][SUBCHUNK_SIZE];

#define subchunk_set(_i, _j)   (subchunkopt[(_j)] |= BIT(_i))
#define subchunk_unset(_i, _j) (subchunkopt[(_j)] &= ~BIT(_i))

#define subchunk_set_world(_x, _y)                                             \
	subchunk_set((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)
#define subchunk_unset_world(_x, _y)                                           \
	subchunk_unset((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)

#define is_subchunk_active(_i, _j) (0 != (subchunkopt[(_j)] & BIT(_i)))
#define is_subchunk_active_world(_x, _y)                                       \
	is_subchunk_active((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)

#define ResetSubchunks                                                         \
	for (uint_fast8_t __j = 0; __j < SUBCHUNK_SIZE; ++__j) {                   \
		subchunkopt[__j] = SUBCHUNK_ROW_COMPLETE;                              \
		for (uint_fast8_t __i = 0; __i < SUBCHUNK_SIZE; ++__i)                 \
			deactivate_soil(__i, __j);                                         \
	}

/** Returns true if any object was updated, false otherwise */
void update_gameboard();
void draw_gameboard_world(const SDL_FRect *camera);

/* =============================================================== */
/* Chunks */
#define CHUNK_MAX_X	  (UINT16_MAX)
#define CHUNK_MAX_Y	  (UINT8_MAX)
#define CHUNK_MEMSIZE (CHUNK_SIZE * CHUNK_SIZE)

typedef uint32_t seed_t;
typedef uint16_t chunk_xaxis_t;
typedef uint8_t	 chunk_yaxis_t;

#pragma pack(push, 1)
typedef union chunk_u {
	seed_t id;
	struct {
		chunk_xaxis_t x;
		chunk_yaxis_t y;
		/* Chunk flags */
		bit reserved : 7; /* Padding to match `seed_t` size */
		bit modified : 1;
	} PACKED;
} Chunk;
#pragma pack(pop)

extern const Chunk CHUNK_ID_VALID_MASK;
/* Get chunk id without flags, only x and y */
#define CHUNK_ID(chunk_) (((chunk_).id) & (CHUNK_ID_VALID_MASK.id))

/** Virtual Chunk Table */
extern Chunk vctable[3][3];

#define INVALID_CACHE_CHUNK ((seed_t)~0)
typedef struct _CacheChunk {
	Chunk chunk_id;
	GO_ID chunk_data[CHUNK_MEMSIZE];
} CacheChunk;

void cache_chunk_init();
void cache_chunk_flushall();

/** Saves the chunk gameboard[vy][vx] in cache. The modified flag will tell if
 * the chunk is marked for disk storage when this cached is flushed out. */
void cache_chunk(Chunk chunk_id, const size_t vy, const size_t vx);

/** Returns the cached chunk data, or NULL if it's not in cache. Flags are
 * insensitive. */
GO_ID *cache_get_chunk(Chunk chunk_id);

#define GEN_WATERSEA_OFFSET_X 128
#define GEN_SKY_Y			  32
#define GEN_TOP_LAYER_Y		  48
#define GEN_BEDROCK_MARGIN_Y  (CHUNK_MAX_Y - 1)
void generate_chunk(seed_t SEED, Chunk CHUNK, const size_t vx, const size_t vy);

/* =============================================================== */
/* Box2D world */
extern b2World *b2_world;

void activate_soil(size_t si, size_t sj);
void deactivate_soil(size_t si, size_t sj);

#define activate_soil_world(_x, _y)                                            \
	activate_soil((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)
#define deactivate_soil_world(_x, _y)                                          \
	deactivate_soil((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT)
#define deactivate_soil_all                                                    \
	for (uint_fast8_t __j = 0; __j < SUBCHUNK_SIZE; ++__j) {                   \
		for (uint_fast8_t __i = 0; __i < SUBCHUNK_SIZE; ++__i) {               \
			deactivate_soil(__i, __j);                                         \
		}                                                                      \
	}
#define recalculate_soil(_si, _sj)                                             \
	if (soil_body[_sj][_si].body != NULL) {                                    \
		deactivate_soil(_si, _sj);                                             \
		activate_soil(_si, _sj);                                               \
	}
#define recalculate_soil_world(_x, _y)                                         \
	recalculate_soil((_x) / SUBCHUNK_WIDTH, (_y) / SUBCHUNK_HEIGHT);

#endif /* _ENGINE_H */
