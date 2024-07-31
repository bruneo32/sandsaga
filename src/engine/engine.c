#include "engine.h"

#include "noise.h"

byte	   gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];
subchunk_t subchunkopt[SUBCHUNK_SIZE];

void set_subchunk(bool on, uint_fast8_t i, uint_fast8_t j) {
	if (on)
		subchunkopt[j] = (subchunkopt[j] | BIT(i));
	else
		subchunkopt[j] = (subchunkopt[j] & ~BIT(i));
}

void generate_chunk(seed_t SEED, Chunk CHUNK, const size_t vx,
					const size_t vy) {
	if (CHUNK.y < GEN_SKY_Y ||
		(CHUNK.y == GEN_SKY_Y &&
		 (CHUNK.x < GEN_WATERSEA_OFFSET_X ||
		  CHUNK.x > CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X))) {
		/* Empty sky */
		for (uint_fast16_t y = vy; y < vy + VIEWPORT_HEIGHT; ++y)
			memset(&gameboard[y][vx], GO_NONE, VIEWPORT_WIDTH);
		return;
	} else if (CHUNK.y > CHUNK_MAX_X - GEN_BEDROCK_MARGIN_Y) {
		/* Bedrock */
		for (uint_fast16_t y = vy; y < vy + VIEWPORT_HEIGHT; ++y)
			memset(&gameboard[y][vx], GO_STONE, VIEWPORT_WIDTH);
		return;
	} else if (CHUNK.x < GEN_WATERSEA_OFFSET_X ||
			   CHUNK.x > CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X) {
		/* Water sea */
		for (uint_fast16_t y = vy; y < vy + VIEWPORT_HEIGHT; ++y)
			memset(&gameboard[y][vx], GO_WATER, VIEWPORT_WIDTH);
		return;
	}

	// const uint_fast16_t vx_max = clamp_high(vx + VIEWPORT_WIDTH,
	// VSCREEN_WIDTH);
	const uint_fast16_t vy_max =
		clamp_high(vy + VIEWPORT_HEIGHT, VSCREEN_HEIGHT);

	const uint_fast64_t world_x0 = CHUNK.x * VIEWPORT_WIDTH;
	const uint_fast64_t world_y0 = CHUNK.y * VIEWPORT_HEIGHT;

	/* Rock base */
	for (uint_fast16_t y = vy; y < vy_max; ++y)
		memset(&gameboard[y][vx], GO_STONE, VIEWPORT_WIDTH);

	/* GENERATE */
	for (uint_fast16_t x = 0; x < VIEWPORT_WIDTH; ++x) {
		const uint_fast64_t world_x = world_x0 + x;
		const uint_fast16_t gbx		= vx + x;

		const uint_fast64_t ground_height =
			(CHUNK.y < GEN_TOP_LAYER_Y)
				? fabs(perlin2d(SEED, world_x, 0, 0.001, 2) *
					   (GEN_TOP_LAYER_Y - GEN_SKY_Y))
				: 0;

		for (uint_fast16_t y = 0; y < VIEWPORT_HEIGHT; ++y) {
			const uint_fast64_t world_y = world_y0 + y;
			const uint_fast16_t gby		= vy + y;

			if (CHUNK.y < GEN_TOP_LAYER_Y) {
				const uint_fast64_t ground =
					(GEN_SKY_Y * VIEWPORT_HEIGHT) + ground_height;

				if (world_y < ground) {
					gameboard[gby][gbx] = GO_NONE;
					continue;
				}
			}

			const double noise = perlin2d(SEED, world_x, world_y, 0.007, 4);
			if (noise > 0.88) {
				gameboard[gby][gbx] = GO_NONE;
			} else if (noise > 0.75) {
				gameboard[gby][gbx] = GO_WATER;
			} else if (noise > 0.6) {
				gameboard[gby][gbx] = GO_SAND;
			}
		}
	}
}

void update_object(const size_t x, const size_t y) {
	byte *boardxy = &gameboard[y][x];
	byte  type	  = *boardxy;

	/* Update object behaviour */
	size_t left_or_right = (((rand() % 2) == 0) ? 1 : -1);

	// size_t up_y	   = y - 1;
	size_t down_y  = y + 1;
	size_t left_x  = x - left_or_right;
	size_t right_x = x + left_or_right;

	switch (type) {
	case GO_SAND: {
		byte *bottom	  = &gameboard[down_y][x];
		byte *bottomleft  = &gameboard[down_y][left_x];
		byte *bottomright = &gameboard[down_y][right_x];

		if (IS_IN_BOUNDS(x, down_y)) {
			if (*bottom == GO_NONE) {
				*boardxy = GO_NONE;
				*bottom	 = GUPDATE(GO_SAND);
				set_subchunk_world(1, x, down_y);
				set_subchunk_world(1, x, y);
			}

			else if (IS_IN_BOUNDS_H(left_x) && *bottomleft == GO_NONE) {
				*boardxy	= GO_NONE;
				*bottomleft = GUPDATE(GO_SAND);
				set_subchunk_world(1, left_x, down_y);
				set_subchunk_world(1, x, y);
			} else if (IS_IN_BOUNDS_H(right_x) && *bottomright == GO_NONE) {
				*boardxy	 = GO_NONE;
				*bottomright = GUPDATE(GO_SAND);
				set_subchunk_world(1, right_x, down_y);
				set_subchunk_world(1, x, y);
			}

			/* Flow down in less dense fluids */
			else if (*bottom < *boardxy && GO_IS_FLUID(*bottom)) {
				SWAP(*bottom, *boardxy);
				*bottom |= BITMASK_GO_UPDATED;
				*boardxy |= BITMASK_GO_UPDATED;
				set_subchunk_world(1, x, y);
				set_subchunk_world(1, x, down_y);
			}
		}
	} break;
	case GO_WATER: {
		byte *bottom	  = &gameboard[down_y][x];
		byte *bottomleft  = &gameboard[down_y][left_x];
		byte *bottomright = &gameboard[down_y][right_x];
		byte *left		  = &gameboard[y][left_x];
		byte *right		  = &gameboard[y][right_x];

		if (IS_IN_BOUNDS(x, down_y) && *bottom == GO_NONE) {
			*boardxy = GO_NONE;
			*bottom	 = GUPDATE(GO_WATER);
			set_subchunk_world(1, x, down_y);
			set_subchunk_world(1, x, y);
		}

		else if (IS_IN_BOUNDS(left_x, down_y) && *bottomleft == GO_NONE) {
			*boardxy	= GO_NONE;
			*bottomleft = GUPDATE(GO_WATER);
			set_subchunk_world(1, left_x, down_y);
			set_subchunk_world(1, x, y);
		} else if (IS_IN_BOUNDS(right_x, down_y) && *bottomright == GO_NONE) {
			*boardxy	 = GO_NONE;
			*bottomright = GUPDATE(GO_WATER);
			set_subchunk_world(1, right_x, down_y);
			set_subchunk_world(1, x, y);
		}

		else if (IS_IN_BOUNDS(left_x, y) && *left == GO_NONE) {
			*boardxy = GO_NONE;
			*left	 = GUPDATE(GO_WATER);
			set_subchunk_world(1, left_x, y);
			set_subchunk_world(1, x, y);
		} else if (IS_IN_BOUNDS(right_x, y) && *right == GO_NONE) {
			*boardxy = GO_NONE;
			*right	 = GUPDATE(GO_WATER);
			set_subchunk_world(1, right_x, y);
			set_subchunk_world(1, x, y);
		}
	} break;
	}
}

void update_gameboard() {
#define _update_block(_sx, _sy, _ex, _ey)                                      \
	for (uint_fast16_t __j = _ey - 1;; --__j) {                                \
		/* Horizontal loop has to be first evens and then odds, in order to    \
		 * save some bugs with fluids */                                       \
		for (uint_fast16_t __i = _sx; __i < _ex; __i += 2) {                   \
			const byte pixel_ = gameboard[__j][__i];                           \
			if (pixel_ != GO_NONE && !IS_GUPDATED(pixel_))                     \
				update_object(__i, __j);                                       \
			/* Next: odd numbers */                                            \
			if (__i == _ex - 2)                                                \
				__i = -1;                                                      \
		}                                                                      \
		if (__j == _sy)                                                        \
			break;                                                             \
	}

	for (uint_fast8_t sj = SUBCHUNK_SIZE - 1;; --sj) {
		uint_fast16_t start_j = sj * SUBCHUNK_HEIGHT;
		uint_fast16_t end_j	  = start_j + SUBCHUNK_HEIGHT;
		if (end_j > VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT;

		if (subchunkopt[sj] == -1) {
			/* Draw whole row */
			_update_block(0, start_j, VSCREEN_WIDTH, end_j);
		} else
			for (uint_fast8_t si = 0; si < SUBCHUNK_SIZE; ++si) {
				const bool is_active = is_subchunk_active(si, sj);
				if (!is_active)
					continue;

				/* Activate top subchunk for gravity purposes */
				if (sj > 0)
					set_subchunk(1, si, sj - 1);

				uint_fast16_t start_i = si * SUBCHUNK_WIDTH;
				uint_fast16_t end_i	  = start_i + SUBCHUNK_WIDTH;

				if (end_i > VSCREEN_WIDTH)
					end_i = VSCREEN_WIDTH;

				_update_block(start_i, start_j, end_i, end_j);
			}

		if (sj == 0)
			break;
	}

#undef _update_block
}

void draw_gameboard_world(const SDL_Rect *camera) {
#define _draw_block(_sx, _sy, _ex, _ey)                                        \
	for (uint_fast16_t __j = _ey - 1;; --__j) {                                \
		for (uint_fast16_t __i = _sx; __i < _ex; ++__i) {                      \
			if (IS_GUPDATED(gameboard[__j][__i])) {                            \
				if (!p)                                                        \
					p = true;                                                  \
				/* Reset BITMASK_GO_UPDATED */                                 \
				gameboard[__j][__i] = GOBJECT(gameboard[__j][__i]);            \
			}                                                                  \
			if (camera != NULL &&                                              \
				((__i >= camera->x && __i < camera->x + camera->w) &&          \
				 (__j >= camera->y && __j < camera->y + camera->h))) {         \
				Render_Pixel_Color(__i, __j, GO_COLORS[gameboard[__j][__i]]);  \
			}                                                                  \
		}                                                                      \
		if (__j == _sy)                                                        \
			break;                                                             \
	}

	for (uint_fast8_t sj = SUBCHUNK_SIZE - 1;; --sj) {
		uint_fast16_t start_j = sj * SUBCHUNK_HEIGHT;
		uint_fast16_t end_j	  = start_j + SUBCHUNK_HEIGHT;
		if (end_j > VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT;

		for (uint_fast8_t si = 0; si < SUBCHUNK_SIZE; ++si) {
			if (!is_subchunk_active(si, sj))
				continue;

			uint_fast16_t start_i = si * SUBCHUNK_WIDTH;
			uint_fast16_t end_i	  = start_i + SUBCHUNK_WIDTH;

			if (end_i > VSCREEN_WIDTH)
				end_i = VSCREEN_WIDTH;

			bool p = false;

			_draw_block(start_i, start_j, end_i, end_j);

			if (!p)
				set_subchunk(0, si, sj);
		}

		if (sj == 0)
			break;
	}

	/* Draw test points */
	for (size_t _j = 0; _j < VSCREEN_HEIGHT; _j += SUBCHUNK_HEIGHT)
		for (size_t _i = 0; _i < VSCREEN_WIDTH; _i += SUBCHUNK_WIDTH)
			Render_Pixel_Color(
				_i, _j,
				((!is_subchunk_active_world(_i, _j)) ? C_RED : C_GREEN));

#undef _draw_block
}
