#include "engine.h"

#include "noise.h"

#include "../disk/worldctrl.h"

int DEBUG_LEVEL = e_dbgl_none;

size_t WORLD_SEED = 0;
size_t frame_cx	  = 0;

GO_ID gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

Color vscreen[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

b2World *b2_world = NULL;

/**
 * The subchunk optimization is only for the drawing step,
 * as it is the most expensive step.
 * The update step is fluent with the current VSCREEN size.
 */
subchunk_t subchunkopt[SUBCHUNK_SIZE];

SoilData soil_body[SUBCHUNK_SIZE][SUBCHUNK_SIZE];

const Chunk CHUNK_ID_VALID_MASK = ((Chunk){
	.x		  = CHUNK_MAX_X,
	.y		  = CHUNK_MAX_Y,
	.modified = 0,
	.reserved = 0,
});

Chunk vctable[3][3];

void generate_chunk(seed_t SEED, Chunk CHUNK, const size_t vx,
					const size_t vy) {
	/* Check world borders */
	if (CHUNK.y < GEN_SKY_Y ||
		(CHUNK.y <= GEN_TOP_LAYER_Y &&
		 (CHUNK.x < GEN_WATERSEA_OFFSET_X ||
		  CHUNK.x > CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X))) {
		/* Empty sky */
		for (uint_fast16_t y = vy; y < vy + CHUNK_SIZE; ++y)
			memset(&gameboard[y][vx], GO_NONE.raw, CHUNK_SIZE);
		return;
	} else if (CHUNK.y > CHUNK_MAX_X - GEN_BEDROCK_MARGIN_Y) {
		/* Bedrock */
		for (uint_fast16_t y = vy; y < vy + CHUNK_SIZE; ++y)
			memset(&gameboard[y][vx], GO_STONE.raw, CHUNK_SIZE);
		return;
	} else if (CHUNK.x < GEN_WATERSEA_OFFSET_X ||
			   CHUNK.x > CHUNK_MAX_X - GEN_WATERSEA_OFFSET_X) {
		/* Water sea */
		for (uint_fast16_t y = vy; y < vy + CHUNK_SIZE; ++y)
			memset(&gameboard[y][vx], GO_WATER.raw, CHUNK_SIZE);
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
				memset(&gameboard[y][vx], GO_SAND.raw, CHUNK_SIZE);
			return;
		}

		/* Empty base or sky */
		for (uint_fast16_t y = vy; y < vy_max; ++y)
			memset(&gameboard[y][vx], GO_NONE.raw, CHUNK_SIZE);

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
				gameboard[gby][gbx].raw = GO_SAND.raw;
			}
		}
		return;
	}

	/* Rock base */
	for (uint_fast16_t y = vy; y < vy_max; ++y)
		memset(&gameboard[y][vx], GO_STONE.raw, CHUNK_SIZE);

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
				gameboard[gby][gbx].raw = GO_WATER.raw;
			} else if (noise > 0.6) {
				gameboard[gby][gbx].raw = GO_SAND.raw;
			}
		}
	}
}

#define CHUNK_CACHE_SIZE	32
#define CHUNK_CACHE_SIZE_M1 (CHUNK_CACHE_SIZE - 1)

static CacheChunk m_cached_chunks[CHUNK_CACHE_SIZE];

static size_t m_cc_idx;

void cache_chunk_init() {
	memset(m_cached_chunks, INVALID_CACHE_CHUNK, sizeof(m_cached_chunks));
	m_cc_idx = 0;
}

void cache_chunk_flushall() {
	for (size_t i = 0; i < CHUNK_CACHE_SIZE; ++i) {
		Chunk cid = m_cached_chunks[i].chunk_id;
		if (cid.id != INVALID_CACHE_CHUNK) {
			save_chunk_to_disk(cid, m_cached_chunks[i].chunk_data);
		}
	}
}

void cache_chunk(Chunk chunk_id, const size_t vy, const size_t vx) {
	/* Get cached chunk ready to store */
	CacheChunk *cache_chunk = &m_cached_chunks[m_cc_idx];

	/* If cache is not full, iterate until m_cc_idx */
	const uint_fast8_t max_cache_idx =
		(m_cached_chunks[CHUNK_CACHE_SIZE_M1].chunk_id.id ==
		 INVALID_CACHE_CHUNK)
			? m_cc_idx
			: CHUNK_CACHE_SIZE;

	/* Search for already cached chunk to overwrite instead */
	for (size_t i = 0; i < max_cache_idx; ++i) {
		if (CHUNK_ID(m_cached_chunks[i].chunk_id) == CHUNK_ID(chunk_id)) {
			cache_chunk = &m_cached_chunks[i];
			break;
		}
	}

	/* Save to disk the previous cached chunk data */
	if (cache_chunk->chunk_id.id != INVALID_CACHE_CHUNK) {
		save_chunk_to_disk(cache_chunk->chunk_id, cache_chunk->chunk_data);
	}

	/* Update cached chunk with new data line by line */
	cache_chunk->chunk_id = chunk_id;
	for (size_t k = 0; k < CHUNK_SIZE; ++k) {
		/* Sanitize flags before copying */
		for (size_t l = 0; l < CHUNK_SIZE; ++l)
			gameboard[vy + k][vx + l].updated = 0;

		memcpy(cache_chunk->chunk_data + (k * CHUNK_SIZE),
			   &gameboard[vy + k][vx], CHUNK_SIZE);
	}

	/* Update index if it's a new cache chunk */
	if (cache_chunk == &m_cached_chunks[m_cc_idx]) {
		if (++m_cc_idx == CHUNK_CACHE_SIZE)
			m_cc_idx = 0;
	}
}

GO_ID *cache_get_chunk(Chunk chunk_id) {
	/* If cache is not full, iterate until m_cc_idx */
	const uint_fast8_t max_cache_idx =
		(m_cached_chunks[CHUNK_CACHE_SIZE_M1].chunk_id.id ==
		 INVALID_CACHE_CHUNK)
			? m_cc_idx
			: CHUNK_CACHE_SIZE;

	for (size_t i = 0; i < max_cache_idx; ++i) {
		if (CHUNK_ID(m_cached_chunks[i].chunk_id) == CHUNK_ID(chunk_id)) {
			return m_cached_chunks[i].chunk_data;
		}
	}

	return NULL;
}

bool update_object(const size_t x, const size_t y, const bool ltr) {
	size_t left_or_right = (ltr ? 1 : -1);

	size_t up_y		= y - 1;
	size_t down_y	= y + 1;
	size_t left_x	= x - left_or_right;
	size_t left_x2	= x - left_or_right - left_or_right;
	size_t right_x	= x + left_or_right;
	size_t right_x2 = x + left_or_right + left_or_right;

	GO_ID *boardxy = &gameboard[y][x];
	GO_ID *bottom  = &gameboard[down_y][x];

	GO_ID		gobjr = *boardxy;
	GameObject *gobj  = &GOBJECT(gobjr);
	GO_Type		type  = gobj->type;

	switch (type) {
	case GO_POWDER: {
		GO_ID *bottomleft  = &gameboard[down_y][left_x];
		GO_ID *bottomright = &gameboard[down_y][right_x];

		if (IS_IN_BOUNDS(x, down_y)) {
			if ((*bottom).raw == GO_NONE.raw) {
				(*bottom).id	  = gobjr.id;
				(*bottom).updated = 1;
				(*boardxy).raw	  = GO_NONE.raw;
				subchunk_set_world(x, down_y);
				subchunk_set_world(x, y);
				return true;
			}

			if (IS_IN_BOUNDS_H(left_x) && (*bottomleft).raw == GO_NONE.raw) {
				(*bottomleft).id	  = gobjr.id;
				(*bottomleft).updated = 1;
				(*boardxy).raw		  = GO_NONE.raw;
				subchunk_set_world(left_x, down_y);
				subchunk_set_world(x, y);
				return true;
			}

			if (IS_IN_BOUNDS_H(right_x) && (*bottomright).raw == GO_NONE.raw) {
				(*bottomright).id	   = gobjr.id;
				(*bottomright).updated = 1;
				(*boardxy).raw		   = GO_NONE.raw;
				subchunk_set_world(right_x, down_y);
				subchunk_set_world(x, y);
				return true;
			}
		}
	} break;
	case GO_LIQUID: {
		GO_ID *bottomleft  = &gameboard[down_y][left_x];
		GO_ID *bottomright = &gameboard[down_y][right_x];
		GO_ID *left		   = &gameboard[y][left_x];
		GO_ID *right	   = &gameboard[y][right_x];

		if (IS_IN_BOUNDS_V(down_y) && (*bottom).raw == GO_NONE.raw) {
			(*bottom).id	  = gobjr.id;
			(*bottom).updated = 1;
			(*boardxy).raw	  = GO_NONE.raw;
			subchunk_set_world(x, down_y);
			subchunk_set_world(x, y);
			return true;
		}

		if (IS_IN_BOUNDS_H(left_x)) {
			/* If there is a blocking fluid try to move it when its density
			 * is lower, this makes the water look more fluent */
			if (IS_IN_BOUNDS_H(left_x2) && (*left).raw == gobjr.raw &&
				(gameboard[y][left_x2]).raw < gobjr.raw) {
				if (update_object(left_x2, y, ltr))
					update_object(left_x, y, ltr);
			}

			if ((*left).raw == GO_NONE.raw) {
				(*left).id		= gobjr.id;
				(*left).updated = 1;
				(*boardxy).raw	= GO_NONE.raw;
				subchunk_set_world(left_x, y);
				subchunk_set_world(x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS_H(right_x)) {
			/* If there is a blocking fluid try to move it when its density
			 * is lower, this makes the water look more fluent */
			if (IS_IN_BOUNDS_H(right_x2) && (*right).raw == gobjr.raw &&
				(gameboard[y][right_x2]).raw < gobjr.raw) {
				if (update_object(right_x2, y, ltr))
					update_object(right_x, y, ltr);
			}

			if ((*right).raw == GO_NONE.raw) {
				(*right).id		 = gobjr.id;
				(*right).updated = 1;
				(*boardxy).raw	 = GO_NONE.raw;
				subchunk_set_world(right_x, y);
				subchunk_set_world(x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS(left_x, down_y) && (*bottomleft).raw == GO_NONE.raw) {
			(*bottomleft).id	  = gobjr.id;
			(*bottomleft).updated = 1;
			(*boardxy).raw		  = GO_NONE.raw;
			subchunk_set_world(left_x, down_y);
			subchunk_set_world(x, y);
			return true;
		}

		if (IS_IN_BOUNDS(right_x, down_y) &&
			(*bottomright).raw == GO_NONE.raw) {
			(*bottomright).id	   = gobjr.id;
			(*bottomright).updated = 1;
			(*boardxy).raw		   = GO_NONE.raw;
			subchunk_set_world(right_x, down_y);
			subchunk_set_world(x, y);
			return true;
		}
	} break;
	case GO_GAS: {
		GO_ID *up	   = &gameboard[up_y][x];
		GO_ID *upleft  = &gameboard[up_y][left_x];
		GO_ID *upright = &gameboard[up_y][right_x];
		GO_ID *left	   = &gameboard[y][left_x];
		GO_ID *right   = &gameboard[y][right_x];

		if (IS_IN_BOUNDS_V(up_y) && (*up).raw == GO_NONE.raw) {
			(*up).id	   = gobjr.id;
			(*up).updated  = 1;
			(*boardxy).raw = GO_NONE.raw;
			subchunk_set_world(x, up_y);
			subchunk_set_world(x, y);
			return true;
		}

		if (IS_IN_BOUNDS_H(left_x)) {
			/* If there is a blocking fluid try to move it when its density
			 * is lower, this makes the water look more fluent */
			if (IS_IN_BOUNDS_H(left_x2) && (*left).raw == gobjr.raw &&
				(gameboard[y][left_x2]).raw < gobjr.raw) {
				if (update_object(left_x2, y, ltr))
					update_object(left_x, y, ltr);
			}

			if ((*left).raw == GO_NONE.raw) {
				(*left).id		= gobjr.id;
				(*left).updated = 1;
				(*boardxy).raw	= GO_NONE.raw;
				subchunk_set_world(left_x, y);
				subchunk_set_world(x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS_H(right_x)) {
			/* If there is a blocking fluid try to move it when its density
			 * is lower, this makes the water look more fluent */
			if (IS_IN_BOUNDS_H(right_x2) && (*right).raw == gobjr.raw &&
				(gameboard[y][right_x2]).raw < gobjr.raw) {
				if (update_object(right_x2, y, ltr))
					update_object(right_x, y, ltr);
			}

			if ((*right).raw == GO_NONE.raw) {
				(*right).id		 = gobjr.id;
				(*right).updated = 1;
				(*boardxy).raw	 = GO_NONE.raw;
				subchunk_set_world(right_x, y);
				subchunk_set_world(x, y);
				return true;
			}
		}

		if (IS_IN_BOUNDS(left_x, up_y) && (*upleft).raw == GO_NONE.raw) {
			(*upleft).id	  = gobjr.id;
			(*upleft).updated = 1;
			(*boardxy).raw	  = GO_NONE.raw;
			subchunk_set_world(left_x, up_y);
			subchunk_set_world(x, y);
			return true;
		}

		if (IS_IN_BOUNDS(right_x, up_y) && (*upright).raw == GO_NONE.raw) {
			(*upright).id	   = gobjr.id;
			(*upright).updated = 1;
			(*boardxy).raw	   = GO_NONE.raw;
			subchunk_set_world(right_x, up_y);
			subchunk_set_world(x, y);
			return true;
		}
	} break;
	}

	/* Flow down in less dense fluids */
	const GO_ID bot		  = *bottom;
	GameObject *go_bottom = &GOBJECT(bot);
	if (IS_IN_BOUNDS_V(down_y) && bot.raw && !bot.updated &&
		GO_IS_FLUID(type) && GO_IS_FLUID(go_bottom->type) &&
		go_bottom->density < gobj->density) {
		SWAP((*bottom).raw, (*boardxy).raw);
		(*bottom).updated  = 1;
		(*boardxy).updated = 1;
		subchunk_set_world(x, y);
		subchunk_set_world(x, down_y);
		return true;
	}

	return false;
}

/* ==== Update gameboard subchunked ==== */

void update_gameboard() {
	static bool left_to_right = false;

	for (ssize_t sj = SUBCHUNK_SIZE - 1; sj >= 0; --sj) {
		/* If the whole line of subchunks is inactive, there's nothing to do */
		if (subchunkopt[sj] == 0)
			continue;

		ssize_t start_j = sj * SUBCHUNK_HEIGHT;
		ssize_t end_j	= start_j + SUBCHUNK_HEIGHT;
		end_j			= clamp_high(end_j, VSCREEN_HEIGHT);

		bool odds = sj & 1;

		/* First active subchunk i */
		ssize_t fsi = -1;

		/* Process subchunks, but like they were a whole block. In effect, when
		 * reaching the edge of a j loop, don't go back to the same subchunk, go
		 * to the next and this will be revisited. This is weird to say but
		 * important to make fluids like water to flow consistently. */
		for (ssize_t j = end_j - 1; j >= start_j; --j) {
			bool ltr = fast_rand() & 1;
			/* First odds, then evens (or viceversa) */
			repeat(2) {
				if (left_to_right) {
					for (ssize_t si = (fsi == -1) ? 0 : fsi; si < SUBCHUNK_SIZE;
						 ++si) {
						/* Skip inactive subchunks */
						if (!is_subchunk_active(si, sj))
							continue;

						if (fsi == -1)
							fsi = si;

						ssize_t start_i = si * SUBCHUNK_WIDTH;
						ssize_t end_i	= start_i + SUBCHUNK_WIDTH;

						bool p = false;
						for (ssize_t i = start_i + odds; i < end_i; i += 2) {
							const GO_ID pixel = gameboard[j][i];
							if (pixel.raw == GO_NONE.raw || pixel.updated)
								continue;
							if (update_object(i, j, ltr))
								p = true;
						}
						/* Activate top subchunk for gravity */
						if (p) {
							subchunk_set(si, sj - 1);
							subchunk_set(si - 1, sj);
							subchunk_set(si + 1, sj);
						}
					}
				} else {
					for (ssize_t si = (fsi == -1) ? SUBCHUNK_SIZE - 1 : fsi;
						 (si >= 0); --si) {
						/* Skip inactive subchunks */
						if (!is_subchunk_active(si, sj))
							continue;

						if (fsi == -1)
							fsi = si;

						ssize_t start_i = si * SUBCHUNK_WIDTH;
						ssize_t end_i	= start_i + SUBCHUNK_WIDTH;

						bool p = false;
						for (ssize_t i = end_i - 1 - odds; i >= start_i;
							 i -= 2) {
							const GO_ID pixel = gameboard[j][i];
							if (pixel.raw == GO_NONE.raw || pixel.updated)
								continue;
							if (update_object(i, j, ltr))
								p = true;
						}
						/* Activate top subchunk for gravity */
						if (p) {
							subchunk_set(si, sj - 1);
							subchunk_set(si - 1, sj);
							subchunk_set(si + 1, sj);
						}
					}
				}

				/* Alternate odds and evens, no worries since the repeat loop is
				 * even, odd will result in the same state it started. */
				odds = !odds;
			}
		}
	}

	/* Reset gameobjects updated bit of this frame,
	 * and select which subchunks will keep alive in the next frame. */
	for (size_t sj = 0; sj < SUBCHUNK_SIZE; ++sj) {
		/* If the whole line of subchunks is inactive, there's nothing to do */
		if (subchunkopt[sj] == 0)
			continue;

		size_t start_j = sj * SUBCHUNK_HEIGHT;
		size_t end_j   = start_j + SUBCHUNK_HEIGHT;
		if (end_j >= VSCREEN_HEIGHT)
			end_j = VSCREEN_HEIGHT;

		for (size_t si = 0; si < SUBCHUNK_SIZE; ++si) {
			/* Skip inactive subchunks */
			if (!is_subchunk_active(si, sj))
				continue;

			size_t start_i = si * SUBCHUNK_WIDTH;
			size_t end_i   = start_i + SUBCHUNK_WIDTH;
			if (end_i >= VSCREEN_WIDTH)
				end_i = VSCREEN_WIDTH;

			/* Initial assumption: subchunk will be inactive */
			subchunk_unset(si, sj);
			for (size_t j = start_j; j < end_j; ++j) {
				for (size_t i = start_i; i < end_i; ++i) {
					const GO_ID go = gameboard[j][i];
					if (go.updated) {
						/* Set subchunk active for the next frame */
						subchunk_set(si, sj);
						/* Reset gameobject updated bit */
						gameboard[j][i].updated = 0;
					}
				}
			}
		}
	}

	left_to_right = !left_to_right;
}

void draw_gameboard_world(const SDL_FRect *camera) {
	size_t cam_x = (size_t)(camera->x);
	size_t cam_y = (size_t)(camera->y);

	/* Plot pixels inside camera to buffer */
	for (size_t j = 0; j < VIEWPORT_HEIGHT; ++j) {
		for (size_t i = 0; i < VIEWPORT_WIDTH; ++i) {
			const size_t x = i + cam_x;
			const size_t y = j + cam_y;

			const GO_ID go = gameboard[y][x];

			if (go.raw == GO_NONE.raw) {
				vscreen[vscreen_idx(i, j)] = (Color){0x00, 0x00, 0x00, 0x00};
				continue;
			}

			const GameObject *gobj = &GOBJECT(go);
			if (gobj->draw == NULL) {
				vscreen[vscreen_idx(i, j)] = gobj->color;
			} else {
				/* Get world coordinates */
				const size_t wx = vctable[0][0].x * CHUNK_SIZE + x;
				const size_t wy = vctable[0][0].y * CHUNK_SIZE + y;
				gobj->draw(wx, wy, (int)i, (int)j);
			}
		}
	}

	/* Render pixels to vscreen and copy to renderer */
	SDL_UpdateTexture(__vscreen, NULL, vscreen, vscreen_line_size);
	SDL_RenderCopy(__renderer, __vscreen, NULL, NULL);

	/* Debug draw */
	if (DBGL(e_dbgl_ui)) {
		Color c_debug1 = {0, 127, 255, 64};
		Color c_debug2 = {255, 127, 0, 64};

		/* Draw chunk lines */
		if (cam_x < CHUNK_SIZE) {
			for (size_t _j = 0; _j < VIEWPORT_HEIGHT; _j += 2)
				Render_Pixel_Color(CHUNK_SIZE + 1 - cam_x, _j, c_debug1);
		} else if (cam_x >= CHUNK_SIZE) {
			for (size_t _j = 0; _j < VIEWPORT_HEIGHT; _j += 2)
				Render_Pixel_Color(CHUNK_SIZE_M2 + 1 - cam_x, _j, c_debug2);
		}

		if (cam_y < CHUNK_SIZE) {
			for (size_t _i = 0; _i < VIEWPORT_WIDTH; _i += 2)
				Render_Pixel_Color(_i, CHUNK_SIZE + 1 - cam_y, c_debug1);
		} else if (cam_y >= CHUNK_SIZE) {
			for (size_t _i = 0; _i < VIEWPORT_WIDTH; _i += 2)
				Render_Pixel_Color(_i, CHUNK_SIZE_M2 + 1 - cam_y, c_debug2);
		}

		/* Draw subchunk active points */
		for (size_t _j = (cam_y / SUBCHUNK_HEIGHT) * SUBCHUNK_HEIGHT;
			 _j < cam_y + VSCREEN_HEIGHT; _j += SUBCHUNK_HEIGHT) {
			for (size_t _i = (cam_x / SUBCHUNK_WIDTH) * SUBCHUNK_WIDTH;
				 _i < cam_x + VSCREEN_WIDTH; _i += SUBCHUNK_WIDTH) {
				Render_Pixel_Color(
					_i - cam_x, _j - cam_y,
					((!is_subchunk_active_world(_i, _j)) ? C_RED : C_GREEN));
			}
		}
	}
}

bool F_IS_FLOOR(ssize_t y, ssize_t x) {
	return gameboard[y][x].raw != GO_NONE.raw &&
		   ((&GOBJECT(gameboard[y][x]))->type == GO_STATIC ||
			(&GOBJECT(gameboard[y][x]))->type == GO_POWDER);
}

void deactivate_soil(size_t si, size_t sj) {
	/* Check if soil is invalid */
	if ((((size_t)soil_body[sj][si].body) & 0x0000FFFFFFFFFFFF) == 0)
		return;

	box2d_body_destroy(soil_body[sj][si].body);
	soil_body[sj][si].body = NULL;
}

void activate_soil(size_t si, size_t sj) {
	if (soil_body[sj][si].body != NULL)
		return;

	const size_t start_i = clamp(si * SUBCHUNK_WIDTH, 0, VSCREEN_WIDTH);
	const size_t start_j = clamp(sj * SUBCHUNK_HEIGHT, 0, VSCREEN_HEIGHT);
	/* +1 to overlap the next subchunk edge, this prevents most of the ghost
	 * collision with the player. */
	const size_t end_i = clamp(start_i + SUBCHUNK_WIDTH + 1, 0, VSCREEN_WIDTH);
	const size_t end_j =
		clamp(start_j + SUBCHUNK_HEIGHT + 1, 0, VSCREEN_HEIGHT);

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
				box2d_body_create_fixture(body, (b2Shape *)loopchain, 5.0f,
										  0.8f, 0.0f);

			free(mesh->points);
			free(mesh);
		}

		free(chains->data);
		free(chains);

		soil_body[sj][si].body = body;
	}
}
