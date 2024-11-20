#include "engine.h"

#include "noise.h"

size_t WORLD_SEED = 0;
bool   DEBUG_ON	  = false;

byte gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

/**
 * The subchunk optimization is only for the drawing step,
 * as it is the most expensive step.
 * The update step is fluent with the current VSCREEN size.
 */
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

	size_t up_y		= y - 1;
	size_t down_y	= y + 1;
	size_t left_x	= x - left_or_right;
	size_t left_x2	= x - left_or_right - left_or_right;
	size_t right_x	= x + left_or_right;
	size_t right_x2 = x + left_or_right + left_or_right;

	byte *bottom = &gameboard[down_y][x];

	switch (type) {
	case GO_SAND: {
#define OBJECT GO_SAND
		byte *bottomleft  = &gameboard[down_y][left_x];
		byte *bottomright = &gameboard[down_y][right_x];

		if (IS_IN_BOUNDS(x, down_y)) {
			if (*bottom == GO_NONE) {
				*boardxy = GO_NONE;
				*bottom	 = GUPDATE(OBJECT);
				set_subchunk_world(1, x, down_y);
				set_subchunk_world(1, x, y);
				return true;
			}

			if (IS_IN_BOUNDS_H(right_x) && *bottomright == GO_NONE) {
				*boardxy	 = GO_NONE;
				*bottomright = GUPDATE(OBJECT);
				set_subchunk_world(1, right_x, down_y);
				set_subchunk_world(1, x, y);
				return true;
			}

			if (IS_IN_BOUNDS_H(left_x) && *bottomleft == GO_NONE) {
				*boardxy	= GO_NONE;
				*bottomleft = GUPDATE(OBJECT);
				set_subchunk_world(1, left_x, down_y);
				set_subchunk_world(1, x, y);
				return true;
			}
		}
#undef OBJECT
	} break;
	case GO_WATER: {
#define OBJECT GO_WATER
		byte *bottomleft  = &gameboard[down_y][left_x];
		byte *bottomright = &gameboard[down_y][right_x];
		byte *left		  = &gameboard[y][left_x];
		byte *right		  = &gameboard[y][right_x];

		if (IS_IN_BOUNDS_V(down_y) && *bottom == GO_NONE) {
			*boardxy = GO_NONE;
			*bottom	 = GUPDATE(OBJECT);
			set_subchunk_world(1, x, down_y);
			set_subchunk_world(1, x, y);
			return true;
		}

		if (IS_IN_BOUNDS_H(right_x)) {
			/* If there is a blocking fluid try to move it when its density is
			 * lower, this makes the water look more fluent */
			if (IS_IN_BOUNDS_H(right_x2) && *right == OBJECT &&
				gameboard[y][right_x2] < OBJECT) {
				if (update_object(right_x2, y))
					update_object(right_x, y);
			}

			if (*right == GO_NONE) {
				*boardxy = GO_NONE;
				*right	 = GUPDATE(OBJECT);
				set_subchunk_world(1, right_x, y);
				set_subchunk_world(1, x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS_H(left_x)) {
			/* If there is a blocking fluid try to move it when its density is
			 * lower, this makes the water look more fluent */
			if (IS_IN_BOUNDS_H(left_x2) && *left == OBJECT &&
				gameboard[y][left_x2] < OBJECT) {
				if (update_object(left_x2, y))
					update_object(left_x, y);
			}

			if (*left == GO_NONE) {
				*boardxy = GO_NONE;
				*left	 = GUPDATE(OBJECT);
				set_subchunk_world(1, left_x, y);
				set_subchunk_world(1, x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS(right_x, down_y) && *bottomright == GO_NONE) {
			*boardxy	 = GO_NONE;
			*bottomright = GUPDATE(OBJECT);
			set_subchunk_world(1, right_x, down_y);
			set_subchunk_world(1, x, y);
			return true;
		}

		if (IS_IN_BOUNDS(left_x, down_y) && *bottomleft == GO_NONE) {
			*boardxy	= GO_NONE;
			*bottomleft = GUPDATE(OBJECT);
			set_subchunk_world(1, left_x, down_y);
			set_subchunk_world(1, x, y);
			return true;
		}
#undef OBJECT
	} break;
	case GO_VAPOR: {
#define OBJECT GO_VAPOR
		byte *up	  = &gameboard[up_y][x];
		byte *upleft  = &gameboard[up_y][left_x];
		byte *upright = &gameboard[up_y][right_x];
		byte *left	  = &gameboard[y][left_x];
		byte *right	  = &gameboard[y][right_x];

		if (IS_IN_BOUNDS_V(up_y) && *up == GO_NONE) {
			*boardxy = GO_NONE;
			*up		 = GUPDATE(OBJECT);
			set_subchunk_world(1, x, up_y);
			set_subchunk_world(1, x, y);
			return true;
		}

		if (IS_IN_BOUNDS_H(right_x)) {
			/* If there is a blocking fluid try to move it when its density is
			 * lower, this makes the water look more fluent */
			if (IS_IN_BOUNDS_H(right_x2) && *right == OBJECT &&
				gameboard[y][right_x2] < OBJECT) {
				if (update_object(right_x2, y))
					update_object(right_x, y);
			}

			if (*right == GO_NONE) {
				*boardxy = GO_NONE;
				*right	 = GUPDATE(OBJECT);
				set_subchunk_world(1, right_x, y);
				set_subchunk_world(1, x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS_H(left_x)) {
			if (*left == GO_NONE) {
				/* If there is a blocking fluid try to move it when its density
				 * is lower, this makes the water look more fluent */
				if (IS_IN_BOUNDS_H(left_x2) && *left == OBJECT &&
					gameboard[y][left_x2] < OBJECT) {
					if (update_object(left_x2, y))
						update_object(left_x, y);
				}

				*boardxy = GO_NONE;
				*left	 = GUPDATE(OBJECT);
				set_subchunk_world(1, left_x, y);
				set_subchunk_world(1, x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS(right_x, up_y) && *upright == GO_NONE) {
			*boardxy = GO_NONE;
			*upright = GUPDATE(OBJECT);
			set_subchunk_world(1, right_x, up_y);
			set_subchunk_world(1, x, y);
			return true;
		}

		if (IS_IN_BOUNDS(left_x, up_y) && *upleft == GO_NONE) {
			*boardxy = GO_NONE;
			*upleft	 = GUPDATE(OBJECT);
			set_subchunk_world(1, left_x, up_y);
			set_subchunk_world(1, x, y);
			return true;
		}
#undef OBJECT
	} break;
	}

	/* Flow down in less dense fluids */
	if (GO_IS_FLUID(type) && IS_IN_BOUNDS_V(down_y) && GO_IS_FLUID(*bottom) &&
		*bottom < type) {
		SWAP(*bottom, *boardxy);
		*bottom |= BITMASK_GO_UPDATED;
		*boardxy |= BITMASK_GO_UPDATED;
		set_subchunk_world(1, x, y);
		set_subchunk_world(1, x, down_y);
		return true;
	}

	return false;
}

void update_gameboard() {
	static bool left_to_right = false;

	for (size_t j = VSCREEN_HEIGHT_M1;; --j) {
		if (left_to_right) {
			/* Horizontal loop has to be first odds and then
			 * evens, in order to save some bugs with fluids */
			for (size_t i = 0;; i += 2) {
				const byte pixel_ = gameboard[j][i];
				if (pixel_ != GO_NONE && !IS_GUPDATED(pixel_)) {
					update_object(i, j);
				}

				/* Next: even numbers */
				if (i == VSCREEN_WIDTH)
					i = -1;
				else if (i == VSCREEN_WIDTH_M1)
					break;
			}
		} else {
			/* Horizontal loop has to be first odds and then
			 * evens, in order to save some bugs with fluids */
			for (size_t i = VSCREEN_WIDTH_M1;; i -= 2) {
				const byte pixel_ = gameboard[j][i];
				if (pixel_ != GO_NONE && !IS_GUPDATED(pixel_)) {
					update_object(i, j);
				}
				/* Next: even numbers */
				if (i == -1)
					i = VSCREEN_WIDTH;
				else if (i == 0)
					break;
			}
		}

		if (j == 0)
			break;
	}

	left_to_right = !left_to_right;
}

void draw_subchunk_pos(size_t start_i, size_t end_i, size_t start_j,
					   size_t end_j, bool *p, const SDL_Rect *camera) {

	/* Draw block */
	for (size_t j = end_j - 1;; --j) {
		for (size_t i = end_i - 1;; --i) {
			/* Reset BITMASK_GO_UPDATED and track P */
			const bool u = IS_GUPDATED(gameboard[j][i]);
			if (u) {
				if (p)
					*p = true;
				gameboard[j][i] = GOBJECT(gameboard[j][i]);
			}

			/* If is in camera bounds, draw it */
			if (!camera ||
				((i >= camera->x - 1 && i <= camera->x + camera->w) &&
				 (j >= camera->y - 1 && j <= camera->y + camera->h))) {
				if (DEBUG_ON) {
					Color CUSTOM_COL;
					memcpy(&CUSTOM_COL, &GO_COLORS[gameboard[j][i]],
						   sizeof(Color));
					if (u) {
						CUSTOM_COL.r = 0xFF - CUSTOM_COL.r;
						CUSTOM_COL.g = 0xFF - CUSTOM_COL.g;
						CUSTOM_COL.b = 0xFF - CUSTOM_COL.b;
					}

					Render_Pixel_Color(i, j, CUSTOM_COL);
				} else {
					Render_Pixel_Color(i, j, GO_COLORS[gameboard[j][i]]);
				}
			}

			if (i == start_i)
				break;
		}

		if (j == start_j)
			break;
	}
}

void draw_subchunk(uint_fast8_t si, uint_fast8_t sj, bool *p,
				   const SDL_Rect *camera) {
	size_t start_i = si * SUBCHUNK_WIDTH;
	size_t end_i   = start_i + SUBCHUNK_WIDTH;
	size_t start_j = sj * SUBCHUNK_HEIGHT;
	size_t end_j   = start_j + SUBCHUNK_HEIGHT;

	/* Check if subchunk is out of bounds */
	if (start_i >= VSCREEN_WIDTH - 1 || (start_j >= VSCREEN_HEIGHT - 1))
		return;

	if (end_i > VSCREEN_WIDTH)
		end_i = VSCREEN_WIDTH;
	if (end_j > VSCREEN_HEIGHT)
		end_j = VSCREEN_HEIGHT;

	draw_subchunk_pos(start_i, end_i, start_j, end_j, p, camera);
}

void draw_gameboard_world(const SDL_Rect *camera) {

	/* Traverse all subchunks */
	for (uint_fast8_t sj = SUBCHUNK_SIZE - 1;; --sj) {
		size_t start_j = sj * SUBCHUNK_HEIGHT;
		size_t end_j   = start_j + SUBCHUNK_HEIGHT;
		if (end_j > VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT;

		for (uint_fast8_t si = SUBCHUNK_SIZE - 1;; --si) {

			/* Skip inactive subchunks */
			const bool is_active = is_subchunk_active(si, sj);
			if (!is_active) {
				if (si == 0)
					break;
				continue;
			}

			size_t start_i = si * SUBCHUNK_WIDTH;
			size_t end_i   = start_i + SUBCHUNK_WIDTH;
			if (end_i > VSCREEN_WIDTH)
				end_i = VSCREEN_WIDTH;

			/* Is there any object to update? */
			bool p = false;

			draw_subchunk_pos(start_i, end_i, start_j, end_j, &p, camera);

			/* Disable subchunk if no movement for next iteration */
			if (!p)
				set_subchunk(0, si, sj);

			if (si == 0)
				break;
		}

		if (sj == 0)
			break;
	}

	if (DEBUG_ON) {
		/* Draw chunk lines */
		Render_SetcolorRGBA(0, 127, 255, 64);
		for (size_t _j = 0; _j < VSCREEN_HEIGHT; _j += 2) {
			Render_Pixel(VIEWPORT_WIDTH, _j);
			Render_Pixel(VSCREEN_TWO_THIRDS_WIDTH, _j);
		}
		for (size_t _i = 0; _i < VSCREEN_WIDTH; _i += 2) {
			Render_Pixel(_i, VIEWPORT_HEIGHT);
			Render_Pixel(_i, VSCREEN_TWO_THIRDS_HEIGHT);
		}

		/* Draw test points */
		for (size_t _j = 0; _j < VSCREEN_HEIGHT; _j += SUBCHUNK_HEIGHT)
			for (size_t _i = 0; _i < VSCREEN_WIDTH; _i += SUBCHUNK_WIDTH)
				Render_Pixel_Color(
					_i, _j,
					((!is_subchunk_active_world(_i, _j)) ? C_RED : C_GREEN));
	}
}
