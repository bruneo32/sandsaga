#include "engine.h"

#include "noise.h"
int DEBUG_LEVEL = e_dbgl_none;

size_t WORLD_SEED = 0;

byte gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

b2World *b2_world = NULL;

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

SoilData soil_body[SUBCHUNK_SIZE][SUBCHUNK_SIZE];

void generate_chunk(seed_t SEED, Chunk CHUNK, const size_t vx,
					const size_t vy) {
	/* Check world borders */
	if (CHUNK.y < GEN_SKY_Y ||
		(CHUNK.y <= GEN_TOP_LAYER_Y &&
		 (CHUNK.x < GEN_WATERSEA_OFFSET_X ||
		  CHUNK.x > CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X))) {
		/* Empty sky */
		for (uint_fast16_t y = vy; y < vy + CHUNK_SIZE; ++y)
			memset(&gameboard[y][vx], GO_NONE, CHUNK_SIZE);
		return;
	} else if (CHUNK.y > CHUNK_MAX_X - GEN_BEDROCK_MARGIN_Y) {
		/* Bedrock */
		for (uint_fast16_t y = vy; y < vy + CHUNK_SIZE; ++y)
			memset(&gameboard[y][vx], GO_STONE, CHUNK_SIZE);
		return;
	} else if (CHUNK.x < GEN_WATERSEA_OFFSET_X ||
			   CHUNK.x > CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X) {
		/* Water sea */
		for (uint_fast16_t y = vy; y < vy + CHUNK_SIZE; ++y)
			memset(&gameboard[y][vx], GO_WATER, CHUNK_SIZE);
		return;
	}

	// const uint_fast16_t vx_max = clamp_high(vx + CHUNK_SIZE, VSCREEN_WIDTH);
	const uint_fast16_t vy_max = clamp_high(vy + CHUNK_SIZE, VSCREEN_HEIGHT);

	const uint_fast64_t world_x0 = CHUNK.x * CHUNK_SIZE;
	const uint_fast64_t world_y0 = CHUNK.y * CHUNK_SIZE;

	/* Generate shore */
	const bool is_right_shore =
		CHUNK.x >= CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X - 2;
	if (CHUNK.y <= GEN_TOP_LAYER_Y &&
		(CHUNK.x <= GEN_WATERSEA_OFFSET_X + 2 || is_right_shore)) {
		const uint_fast16_t chunk_x0_to_water =
			is_right_shore ? CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X - CHUNK.x
						   : CHUNK.x - GEN_WATERSEA_OFFSET_X;
		const uint_fast16_t chunk_y0_to_water = GEN_TOP_LAYER_Y - CHUNK.y;

		const uint_fast16_t alternate = (chunk_x0_to_water % 2 != 0);

		const uint_fast16_t chunk_vvalid =
			alternate ? chunk_y0_to_water == (chunk_x0_to_water - 1) / 2
					  : chunk_y0_to_water == chunk_x0_to_water / 2;
		const uint_fast16_t chunk_full_sand =
			alternate ? chunk_y0_to_water < (chunk_x0_to_water - 1) / 2
					  : chunk_y0_to_water < chunk_x0_to_water / 2;

		if (chunk_full_sand) {
			/* Fill with sand */
			for (uint_fast16_t y = vy; y < vy_max; ++y)
				memset(&gameboard[y][vx], GO_SAND, CHUNK_SIZE);
			return;
		}

		/* Empty base or sky */
		for (uint_fast16_t y = vy; y < vy_max; ++y)
			memset(&gameboard[y][vx], GO_NONE, CHUNK_SIZE);

		if (!chunk_vvalid)
			/* Not a shore, it's the sky, exit */
			return;

		uint_fast16_t y0 = alternate ? 0 : CHUNK_SIZE_DIV_2;
		uint_fast16_t cx = is_right_shore ? 0 : CHUNK_SIZE_DIV_2;
		for (uint_fast16_t x = 0; x < CHUNK_SIZE; ++x) {
			if (x % 2 == 0) {
				if (is_right_shore)
					++cx;
				else
					--cx;
			}

			const uint_fast16_t gbx = vx + x;

			for (uint_fast16_t y = y0 + cx; y < CHUNK_SIZE; ++y) {
				const uint_fast16_t gby = vy + y;
				gameboard[gby][gbx]		= GO_SAND;
			}
		}
		return;
	}

	/* Rock base */
	for (uint_fast16_t y = vy; y < vy_max; ++y)
		memset(&gameboard[y][vx], GO_STONE, CHUNK_SIZE);

	/* GENERATE */
	for (uint_fast16_t x = 0; x < CHUNK_SIZE; ++x) {
		const uint_fast64_t world_x = world_x0 + x;
		const uint_fast16_t gbx		= vx + x;

		const uint_fast64_t ground_height =
			(CHUNK.y < GEN_TOP_LAYER_Y)
				? fabs(perlin2d(SEED, world_x, 0, 0.0005, 2)) *
					  ((GEN_TOP_LAYER_Y - GEN_SKY_Y) * CHUNK_SIZE)
				: 0;

		for (uint_fast16_t y = 0; y < CHUNK_SIZE; ++y) {
			const uint_fast64_t world_y = world_y0 + y;
			const uint_fast16_t gby		= vy + y;

			if (CHUNK.y < GEN_TOP_LAYER_Y) {
				const uint_fast64_t ground =
					(GEN_SKY_Y * CHUNK_SIZE) + ground_height;

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
			/* If there is a blocking fluid try to move it when its density
			 * is lower, this makes the water look more fluent */
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
			/* If there is a blocking fluid try to move it when its density
			 * is lower, this makes the water look more fluent */
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
			/* If there is a blocking fluid try to move it when its density
			 * is lower, this makes the water look more fluent */
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
				/* If there is a blocking fluid try to move it when its
				 * density is lower, this makes the water look more fluent
				 */
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
					   size_t end_j, const SDL_Rect *camera) {

	/* Draw block */
	for (size_t j = end_j - 1;; --j) {
		for (size_t i = end_i - 1;; --i) {
			/* Reset BITMASK_GO_UPDATED and track P */
			const bool u = IS_GUPDATED(gameboard[j][i]);
			if (u) {
				set_subchunk_world(1, i, j);
				gameboard[j][i] = GOBJECT(gameboard[j][i]);
			}

			/* If is in camera bounds, draw it */
			if (!camera ||
				((i >= camera->x - 1 && i <= camera->x + camera->w) &&
				 (j >= camera->y - 1 && j <= camera->y + camera->h))) {
				if (DBGL(e_dbgl_engine)) {
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

void draw_gameboard_world(const SDL_Rect *camera) {

	/* Traverse all subchunks */
	for (uint_fast8_t sj = SUBCHUNK_SIZE - 1;; --sj) {
		size_t start_j = sj * SUBCHUNK_HEIGHT;
		size_t end_j   = start_j + SUBCHUNK_HEIGHT;
		if (end_j >= VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT - 1;

		if (subchunkopt[sj] == SUBCHUNK_ROW_COMPLETE) {
			/* The whole line of subchunks is active */
			subchunkopt[sj] = 0;
			draw_subchunk_pos(0, VSCREEN_WIDTH_M1, start_j, end_j, camera);
			if (sj == 0)
				break;
			continue;
		}

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
			if (end_i >= VSCREEN_WIDTH)
				end_i = VSCREEN_WIDTH - 1;

			set_subchunk(0, si, sj);
			draw_subchunk_pos(start_i, end_i, start_j, end_j, camera);

			if (si == 0)
				break;
		}

		if (sj == 0)
			break;
	}

	if (DBGL(e_dbgl_ui)) {
		/* Draw chunk lines */
		for (size_t _j = 0; _j < VSCREEN_HEIGHT; _j += 2) {
			Render_Pixel_RGBA(CHUNK_SIZE, _j, 0, 127, 255, 64);
			Render_Pixel_RGBA(CHUNK_SIZE_M2, _j, 255, 127, 0, 64);
		}
		for (size_t _i = 0; _i < VSCREEN_WIDTH; _i += 2) {
			Render_Pixel_RGBA(_i, CHUNK_SIZE, 0, 127, 255, 64);
			Render_Pixel_RGBA(_i, CHUNK_SIZE_M2, 255, 127, 0, 64);
		}

		/* Draw test points */
		for (size_t _j = 0; _j < VSCREEN_HEIGHT; _j += SUBCHUNK_HEIGHT)
			for (size_t _i = 0; _i < VSCREEN_WIDTH; _i += SUBCHUNK_WIDTH)
				Render_Pixel_Color(
					_i, _j,
					((!is_subchunk_active_world(_i, _j)) ? C_RED : C_GREEN));
	}
}

bool F_IS_FLOOR(size_t y, size_t x) {
	return GO_IS_SOIL(GOBJECT(gameboard[y][x]));
}

void deactivate_soil(size_t si, size_t sj) {
	if (!soil_body[sj][si].body)
		return;

	box2d_body_destroy(soil_body[sj][si].body);
	soil_body[sj][si].body = NULL;
}

void activate_soil(size_t si, size_t sj) {
	if (soil_body[sj][si].body != NULL)
		return;

	const size_t start_i = clamp(si * SUBCHUNK_WIDTH, 0, VSCREEN_WIDTH);
	const size_t end_i	 = clamp(start_i + SUBCHUNK_WIDTH, 0, VSCREEN_WIDTH);
	const size_t start_j = clamp(sj * SUBCHUNK_HEIGHT, 0, VSCREEN_HEIGHT);
	const size_t end_j	 = clamp(start_j + SUBCHUNK_HEIGHT, 0, VSCREEN_HEIGHT);

	CList *chains =
		loopchain_from_contour(start_i, end_i, start_j, end_j, F_IS_FLOOR);

	if (chains != NULL && chains->count > 0) {
		double cx = SUBCHUNK_WIDTH / 2.0;
		double cy = SUBCHUNK_HEIGHT / 2.0;

		b2Body *body = box2d_body_create(
			b2_world, X_TO_U(start_i + SUBCHUNK_WIDTH / 2.0),
			X_TO_U(start_j + SUBCHUNK_HEIGHT / 2.0), b2_staticBody, true, true);

		for (size_t i = 0; i < chains->count; ++i) {
			PointList *mesh = (PointList *)chains->data[i];
			if (!mesh || mesh->count == 0 || !mesh->points)
				continue;
			convert_pointlist_to_box2d_units(mesh, &cx, &cy);

			b2ChainShape *loopchain =
				box2d_shape_loop(mesh->points, mesh->count);

			if (loopchain)
				box2d_body_create_fixture(body, (b2Shape *)loopchain, 1.0f,
										  1.0f);

			free(mesh->points);
			free(mesh);
		}

		free(chains->data);
		free(chains);

		soil_body[sj][si].body = body;
	}
}
