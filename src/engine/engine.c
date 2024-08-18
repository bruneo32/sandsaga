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

bool update_object(const size_t x, const size_t y) {
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
				return true;
			}

			else if (IS_IN_BOUNDS_H(left_x) && *bottomleft == GO_NONE) {
				*boardxy	= GO_NONE;
				*bottomleft = GUPDATE(GO_SAND);
				set_subchunk_world(1, left_x, down_y);
				set_subchunk_world(1, x, y);
				return true;
			} else if (IS_IN_BOUNDS_H(right_x) && *bottomright == GO_NONE) {
				*boardxy	 = GO_NONE;
				*bottomright = GUPDATE(GO_SAND);
				set_subchunk_world(1, right_x, down_y);
				set_subchunk_world(1, x, y);
				return true;
			}

			/* Flow down in less dense fluids */
			else if (*bottom < *boardxy && GO_IS_FLUID(*bottom)) {
				SWAP(*bottom, *boardxy);
				*bottom |= BITMASK_GO_UPDATED;
				*boardxy |= BITMASK_GO_UPDATED;
				set_subchunk_world(1, x, y);
				set_subchunk_world(1, x, down_y);
				return true;
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
			return true;
		}

		else if (IS_IN_BOUNDS(left_x, down_y) && *bottomleft == GO_NONE) {
			*boardxy	= GO_NONE;
			*bottomleft = GUPDATE(GO_WATER);
			set_subchunk_world(1, left_x, down_y);
			set_subchunk_world(1, x, y);
			return true;
		} else if (IS_IN_BOUNDS(right_x, down_y) && *bottomright == GO_NONE) {
			*boardxy	 = GO_NONE;
			*bottomright = GUPDATE(GO_WATER);
			set_subchunk_world(1, right_x, down_y);
			set_subchunk_world(1, x, y);
			return true;
		}

		else if (IS_IN_BOUNDS(left_x, y) && *left == GO_NONE) {
			*boardxy = GO_NONE;
			*left	 = GUPDATE(GO_WATER);
			set_subchunk_world(1, left_x, y);
			set_subchunk_world(1, x, y);
			return true;
		} else if (IS_IN_BOUNDS(right_x, y) && *right == GO_NONE) {
			*boardxy = GO_NONE;
			*right	 = GUPDATE(GO_WATER);
			set_subchunk_world(1, right_x, y);
			set_subchunk_world(1, x, y);
			return true;
		}
	} break;
	}

	return false;
}

/* TODO: Allow extra processed subchunks as Queue */
void update_gameboard() {

	/* Traverse all subchunks */
	for (uint_fast8_t sj = SUBCHUNK_SIZE - 1;; --sj) {
		uint_fast16_t start_j = sj * SUBCHUNK_HEIGHT;
		uint_fast16_t end_j	  = start_j + SUBCHUNK_HEIGHT;

		if (end_j > VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT;

		for (uint_fast8_t si = SUBCHUNK_SIZE - 1;; --si) {
			const bool is_active = is_subchunk_active(si, sj);

			/* Skip inactive subchunks */
			if (!is_active) {
				if (si == 0)
					break;
				continue;
			}

			uint_fast16_t start_i = si * SUBCHUNK_WIDTH;
			uint_fast16_t end_i	  = start_i + SUBCHUNK_WIDTH - 1;

			/* Check if subchunk is out of bounds */
			if (end_i > VSCREEN_WIDTH)
				end_i = VSCREEN_WIDTH;

			/* Is there any object to update? */
			bool p = false;

			/* Update block */
			for (uint_fast16_t j = end_j - 1;; --j) {
				/* Horizontal loop has to be first odds and then
				 * evens, in order to save some bugs with fluids */
				for (uint_fast16_t i = start_i + 1; i <= end_i; i += 2) {
					const byte pixel_ = gameboard[j][i];

					if (pixel_ != GO_NONE && !IS_GUPDATED(pixel_)) {
						bool u = update_object(i, j);
						if (!p && u)
							p = true;
					}

					/* Next: even numbers */
					if (i == end_i)
						i = start_i - 2;
				}

				if (j == start_j)
					break;
			}

			/* Activate top subchunk for gravity purposes
			 * (only if this subchunk has movement) */
			if (sj > 0 && p) {
				set_subchunk(1, si, sj - 1);
				set_subchunk(1, si - 1, sj);
				set_subchunk(1, si + 1, sj);
			}

			if (si == 0)
				break;
		}

		if (sj == 0)
			break;
	}
}

void draw_gameboard_world(const SDL_Rect *camera) {

	/* Traverse all subchunks */
	for (uint_fast8_t sj = SUBCHUNK_SIZE - 1;; --sj) {
		uint_fast16_t start_j = sj * SUBCHUNK_HEIGHT;
		uint_fast16_t end_j	  = start_j + SUBCHUNK_HEIGHT;
		if (end_j > VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT;

		for (uint_fast8_t si = 0; si < SUBCHUNK_SIZE; ++si) {
			const bool is_active = is_subchunk_active(si, sj);

			/* Skip inactive subchunks */
			if (!is_active)
				continue;

			uint_fast16_t start_i = si * SUBCHUNK_WIDTH;
			uint_fast16_t end_i	  = start_i + SUBCHUNK_WIDTH;

			if (end_i > VSCREEN_WIDTH)
				end_i = VSCREEN_WIDTH;

			/* Is there any object to update? */
			bool p = false;

			/* Draw block */
			for (uint_fast16_t j = end_j - 1;; --j) {
				for (uint_fast16_t i = start_i; i < end_i; ++i) {
					/* Reset BITMASK_GO_UPDATED and track P */
					if (IS_GUPDATED(gameboard[j][i])) {
						if (!p)
							p = true;
						gameboard[j][i] = GOBJECT(gameboard[j][i]);
					}

					/* If is in camera bounds, draw it */
					if (!camera ||
						((i >= camera->x - 1 && i <= camera->x + camera->w) &&
						 (j >= camera->y - 1 && j <= camera->y + camera->h))) {
						Render_Pixel_Color(i, j, GO_COLORS[gameboard[j][i]]);
					}
				}

				if (j == start_j)
					break;
			}

			/* Disable subchunk if no movement for next iteration */
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
}
