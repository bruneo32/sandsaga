#include "engine.h"

#include "noise.h"

byte	 gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];
uint32_t subchunkopt[SUBCHUNK_VMAX];

void set_subchunk(bool on, uint_fast8_t i, uint_fast8_t j) {
	if (on)
		subchunkopt[j] = (subchunkopt[j] | BIT(i));
	else
		subchunkopt[j] = (subchunkopt[j] & ~BIT(i));
}

void generate_chunk(seed_t SEED, Chunk CHUNK, const size_t vx,
					const size_t vy) {
	const size_t vx_max = vx + VIEWPORT_WIDTH;
	const size_t vy_max = vy + VIEWPORT_HEIGHT;

	const uint64_t world_x0 = CHUNK.x * VIEWPORT_WIDTH;
	const uint64_t world_y0 = CHUNK.y * VIEWPORT_HEIGHT;

	for (size_t y = vy; y < vy_max; ++y) {
		for (size_t x = vx; x < vx_max; ++x) {
			gameboard[y][x] = GO_STONE;
		}
	}

	const size_t ground_height = 128;
	for (size_t x = vx; x < vx_max; ++x) {
		const size_t ground =
			fabs(perlin2d(SEED, world_x0 + x, 0, 0.005, 1) * ground_height);
		for (size_t y = vy; y < vy_max; ++y) {
			if (y < ground) {
				gameboard[y][x] = GO_NONE;
				continue;
			}

			const double noise =
				perlin2d(SEED, world_x0 + x, world_y0 + y, 0.01, 4);
			if (noise > 0.9) {
				gameboard[y][x] = GO_NONE;
			} else if (noise > 0.75) {
				gameboard[y][x] = GO_WATER;
			} else if (noise > 0.6) {
				gameboard[y][x] = GO_SAND;
			}
		}
	}

	ResetSubchunks;
}

void update_object(const size_t x, const size_t y) {
	byte *boardxy = &gameboard[y][x];
	byte  type	  = *boardxy;

	/* Update object behaviour */
	size_t left_or_right = (((rand() % 2) == 0) ? 1 : -1);

	size_t up_y	   = y - 1;
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
		}
	} break;
	case GO_WATER: {
		byte *top		  = &gameboard[up_y][x];
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

		/* Flow up in more dense fluids */
		else if (IS_IN_BOUNDS(x, up_y) && *top > *boardxy &&
				 GO_IS_FLUID(*top)) {
			SWAP((*top), (*boardxy));
			*top |= BITMASK_GO_UPDATED;
			*boardxy |= BITMASK_GO_UPDATED;
			set_subchunk_world(1, x, y);
			set_subchunk_world(1, x, up_y);
		}
	} break;
	}
}

void update_gameboard() {
	for (uint_fast16_t j = VSCREEN_HEIGHT_M1;; --j) {
		/* Horizontal loop has to be first evens and then odds, in order to
		 * save some bugs with fluids */
		for (uint_fast16_t i = 0; i < VSCREEN_WIDTH; i += 2) {
			const byte pixel = gameboard[j][i];
			if (pixel != GO_NONE && !IS_GUPDATED(pixel))
				update_object(i, j);

			/* Next: odd numbers */
			if (i == VSCREEN_WIDTH_M1 - 1)
				i = -1;
		}

		if (j == 0)
			break;
	}

	/* Reset update bit */
	for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; j++) {
		for (uint_fast16_t i = 0; i < VSCREEN_WIDTH; i++) {
			gameboard[j][i] = GOBJECT(gameboard[j][i]);
		}
	}
}

void draw_gameboard_world() {
#define _draw_block(_sx, _sy, _ex, _ey)                                        \
	for (uint_fast16_t __j = _sy; __j < _ey; ++__j)                            \
		for (uint_fast16_t __i = _sx; __i < _ex; ++__i)                        \
			Render_Pixel_Color(__i, __j, GO_COLORS[gameboard[__j][__i]]);

	for (uint_fast8_t sj = 0; sj < SUBCHUNK_VMAX; ++sj) {
		uint_fast16_t start_j = sj * SUBCHUNK_SIZE;
		uint_fast16_t end_j	  = start_j + SUBCHUNK_SIZE;
		if (end_j > VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT;

		if (subchunkopt[sj] == -1) {
			/* Draw whole row */
			_draw_block(0, start_j, VSCREEN_WIDTH, end_j);
		} else
			for (uint_fast8_t si = 0; si < SUBCHUNK_SIZE; ++si) {
				if (!is_subchunk_active(si, sj))
					continue;

				uint_fast16_t start_i = si * SUBCHUNK_SIZE;
				uint_fast16_t end_i	  = start_i + SUBCHUNK_SIZE;

				if (end_i > VSCREEN_WIDTH)
					end_i = VSCREEN_WIDTH;

				_draw_block(start_i, start_j, end_i, end_j);
			}
	}

	/* Draw test lines */
	Render_Setcolor(C_RED);
	for (size_t _j = 0; _j < VSCREEN_HEIGHT; _j += SUBCHUNK_SIZE) {
		for (size_t _i = 0; _i < VSCREEN_WIDTH; _i += SUBCHUNK_SIZE) {
			Render_Pixel_Color(
				_i, _j,
				((!is_subchunk_active(_i / 32, _j / 32)) ? C_RED : C_GREEN));
			set_subchunk_world(0,_i,_j);
		}
	}

#undef _draw_block
}
