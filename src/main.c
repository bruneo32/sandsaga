#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "assets/res/VGA-ROM.F08.h"
#include "assets/res/_player_body.png.h"

#include "physics/physics.h"

#include "assets/assets.h"
#include "disk/disk.h"
#include "disk/resources.h"
#include "engine/engine.h"
#include "engine/entities.h"
#include "engine/gameobjects.h"
#include "engine/noise.h"
#include "graphics/color.h"
#include "graphics/font/font.h"
#include "graphics/graphics.h"
#include "log/log.h"
#include "ui/ui.h"
#include "util.h"

static size_t		 fps_	 = FPS;
static volatile bool GAME_ON = true;
static volatile bool PAUSED	 = false;

static Player player;

/* Save game when the game is interrupte by any reason */
void F_PANIC_SAVE() {
	/* Cache modified chunks in vscreen to save later */
	for (uint_fast8_t j = 0; j < 3; ++j) {
		for (uint_fast8_t i = 0; i < 3; ++i) {
			const Chunk x = vctable[j][i];
			if (x.modified) {
				cache_chunk(x, j * CHUNK_SIZE, i * CHUNK_SIZE);
			}
		}
	}

	/* Save cached chunks */
	cache_chunk_flushall();
}

void F_QUITGAME() { GAME_ON = false; }
void F_RESUME() {
	PAUSED = false;
	ui_set_cursor(0); /* Default cursor */
}

/** Handle `CTRL + C` to quit the game */
void sigkillHandler(int signum) { GAME_ON = false; }

void draw_sky(SDL_FRect *camera) {
	const size_t y1 = GEN_SKY_Y * 384;
	const size_t y2 = GEN_TOP_LAYER_Y * 384;
	const size_t y3 = (GEN_TOP_LAYER_Y + 4) * 384;
	const size_t y4 = CHUNK_MAX_Y * 384;

	const size_t y1_clouds_dense = (GEN_SKY_Y + 4) * 384;

	const Color color0 = C_WHITE;
	const Color color1 = {000, 191, 255}; /* Sky top */
	const Color color2 = {135, 206, 250}; /* Sky bottom */
	const Color color3 = {105, 126, 140}; /* Rock top */
	const Color color4 = C_BLACK;

	const Chunk tlchunk = vctable[0][0];

	const size_t cam_wy = tlchunk.y * CHUNK_SIZE + ((size_t)camera->y);
	const size_t cam_wx = tlchunk.x * CHUNK_SIZE + ((size_t)camera->x);

	/* Draw color gradient */
	for (size_t y = 0; y < VIEWPORT_HEIGHT; ++y) {
		const size_t world_y = cam_wy + y;

		/* Calculate color line depending on the camera y */
		Color color;
		color.a = 0xFF;

		float t;

		if (world_y <= y1) {
			t		= (float)world_y / (float)y1;
			color.r = (uint8_t)(color0.r + t * (color1.r - color0.r));
			color.g = (uint8_t)(color0.g + t * (color1.g - color0.g));
			color.b = (uint8_t)(color0.b + t * (color1.b - color0.b));
		} else if (world_y <= y2) {
			t		= (float)(world_y - y1) / (float)(y2 - y1);
			color.r = (uint8_t)(color1.r + t * (color2.r - color1.r));
			color.g = (uint8_t)(color1.g + t * (color2.g - color1.g));
			color.b = (uint8_t)(color1.b + t * (color2.b - color1.b));
		} else if (world_y <= y3) {
			t		= (float)(world_y - y2) / (float)(y3 - y2);
			color.r = (uint8_t)(color2.r + t * (color3.r - color2.r));
			color.g = (uint8_t)(color2.g + t * (color3.g - color2.g));
			color.b = (uint8_t)(color2.b + t * (color3.b - color2.b));
		} else {
			t		= (float)(world_y - y3) / (float)(y4 - y3);
			color.r = (uint8_t)(color3.r + t * (color4.r - color3.r));
			color.g = (uint8_t)(color3.g + t * (color4.g - color3.g));
			color.b = (uint8_t)(color3.b + t * (color4.b - color3.b));
		}

		/* Render line */
		for (size_t x = 0; x < VIEWPORT_WIDTH; ++x)
			vscreen[vscreen_idx(x, y)] = color;
	}

	/* Draw clouds */
	for (size_t y = 0; y < VIEWPORT_HEIGHT; ++y) {
		const size_t world_y = cam_wy + y;

		/* Do not calculate for invalid heights */
		if (world_y < y1 || world_y > y2)
			continue;

		for (size_t x = 0; x < VIEWPORT_WIDTH; ++x) {
			const size_t world_x = cam_wx + x;

			double nv = perlin2d(WORLD_SEED, (double)(world_x + frame_cx),
								 (double)(world_y + frame_cx), 0.01, 2);

			/* Make clouds smaller with height */
			if (world_y > y1_clouds_dense)
				nv -= ((double)world_y - y1_clouds_dense) /
					  (double)(y3 - y1_clouds_dense);

			/* Threshold to contrast clouds with sky */
			if (nv < 0.5)
				continue;
			else if (nv < 0.7) {
				/* Create halo effect so clouds don't look like bricks */
				/* Linearly map nv from [0.5, 0.7] to [0, 0.7] */
				nv = (nv - 0.5) * 3.5;
			}

			Color cloud_point_color = {0xFF, 0xFF, 0xFF, 0x00};
			cloud_point_color.a += (uint8_t)(nv * 0xBC);
			/* Blend cloud point */
			vscreen[vscreen_idx(x, y)] =
				Color_blend(cloud_point_color, vscreen[vscreen_idx(x, y)]);
		}
	}

	/* Render pixels to vscreen and copy to renderer */
	SDL_UpdateTexture(__vscreen, NULL, vscreen, vscreen_line_size);
	SDL_RenderCopy(__renderer, __vscreen, NULL, NULL);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, sigkillHandler);

	/* Set random seeds */
	const time_t _st = time(NULL);
	srand(_st);
	sfrand(_st);

	/* =============================================================== */
	/* Init window */
	Render_init("Sandsaga", VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	SDL_RenderSetLogicalSize(__renderer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	SDL_SetWindowSize(__window, VIEWPORT_WIDTH2, VIEWPORT_HEIGHT2);
	SDL_SetWindowPosition(__window, SDL_WINDOWPOS_CENTERED,
						  SDL_WINDOWPOS_CENTERED);

	Font_SetCurrent(res_VGA_ROM_F08);

	Render_Clearscreen_Color(C_BLUE);
	Render_Update;

	/* =============================================================== */
	/* Init stuff */
	disk_init();
	cache_chunk_init();
	atexit(F_PANIC_SAVE);
	init_gameobjects();

	/* Initialize soil */
	for (uint_fast8_t __j = 0; __j < SUBCHUNK_SIZE; ++__j) {
		for (uint_fast8_t __i = 0; __i < SUBCHUNK_SIZE; ++__i) {
			soil_body[__j][__i].body = NULL;
		}
	}

	/* =============================================================== */
	/* Load resources */
	load_player_sprite(&player, res__player_body_png, res__player_body_png_len);

	/* =============================================================== */
	/* Init gameloop variables */
	SDL_Rect   window_viewport;
	SDL_FPoint window_scale;
	char	   fps_str[4];
	snprintf(fps_str, sizeof(fps_str), "%2zu", fps_);
	short block_size	 = 1 << 3;
	bool  grid_mode		 = false;
	GO_ID current_object = GO_FIRST;

	SDL_RenderGetViewport(__renderer, &window_viewport);
	SDL_RenderGetScale(__renderer, &window_scale.x, &window_scale.y);

	/* =============================================================== */
	/* Init UI */
	UIButton *btn_resume = UIButton_new(
		"Resume", VIEWPORT_WIDTH_DIV_2,
		VIEWPORT_HEIGHT_DIV_2 - BITFONT_CHAR_HEIGHT, UI_ALIGN_CENTER);

	UIButton *btn_quit = UIButton_new(
		"Quit game", VIEWPORT_WIDTH_DIV_2,
		VIEWPORT_HEIGHT_DIV_2 + BITFONT_CHAR_HEIGHT, UI_ALIGN_CENTER);

	btn_resume->onClick = F_RESUME;
	btn_quit->onClick	= F_QUITGAME;

	UICanvas pause_canvas = {2, {btn_resume, btn_quit}};

	/* =============================================================== */
	/* Initialize data */
	WORLD_SEED		= world_control->seed;
	player.chunk_id = (Chunk){
		.x		  = CHUNK_MAX_X / 2,
		.y		  = GEN_SKY_Y - 1,
		.modified = 0,
	};

	/* Initialize world chunks and vctable */
	chunk_xaxis_t chunk_start_x = player.chunk_id.x - 1;
	chunk_yaxis_t chunk_start_y = player.chunk_id.y - 1;
	for (chunk_yaxis_t j = chunk_start_y; j <= player.chunk_id.y + 1; ++j) {
		for (chunk_xaxis_t i = chunk_start_x; i <= player.chunk_id.x + 1; ++i) {
			Chunk chunk = (Chunk){
				.x		  = i,
				.y		  = j,
				.modified = 0,
			};
			vctable[j - chunk_start_y][i - chunk_start_x].id = chunk.id;

			const size_t vx = (i - chunk_start_x) * CHUNK_SIZE;
			const size_t vy = (j - chunk_start_y) * CHUNK_SIZE;

			/* Load chunk from disk or generate it */
			GO_ID chunk_data_disk[CHUNK_MEMSIZE];
			if (load_chunk_from_disk(chunk, chunk_data_disk)) {
				for (size_t __k = 0; __k < CHUNK_SIZE; ++__k) {
					memcpy(&gameboard[(vy + __k)][vx],
						   (chunk_data_disk) + (__k * CHUNK_SIZE), CHUNK_SIZE);
				}
			} else {
				generate_chunk(WORLD_SEED, chunk, vx, vy);
			}
		}
	}
	ResetSubchunks;

	player.flying = false;
	player.width  = 12;
	player.height = 24;
	player.x	  = CHUNK_SIZE + CHUNK_SIZE_DIV_2;
	player.y	  = CHUNK_SIZE + CHUNK_SIZE_DIV_2;

	SDL_FRect camera = {0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT};

	camera.x = clamp(player.x - VIEWPORT_WIDTH_DIV_2, 0,
					 VSCREEN_WIDTH - VIEWPORT_WIDTH);
	camera.y = clamp(player.y - VIEWPORT_HEIGHT_DIV_2, 0,
					 VSCREEN_HEIGHT - VIEWPORT_HEIGHT);

	b2_world = box2d_world_create(0, 9.8f);
	create_player_body(&player);

	/* =============================================================== */
	/* Calculate ticks */
	Uint32 prevTicks  = SDL_GetTicks();
	Uint32 frameTicks = 0;

	/* =============================================================== */
	/* GAME LOOP */
	while (GAME_ON) {
		/* Calculate ticks */
		Uint32 currentTicks = SDL_GetTicks();
		Uint32 delta		= currentTicks - prevTicks;
		float  dt			= (float)delta / 1000.0;
		prevTicks			= currentTicks;

		/* =============================================================== */
		/* Get inputs */
		static bool	 LCTRL;
		int			 mouse_x, mouse_y;
		const Uint32 mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

		SDL_Event _event;
		while (GAME_ON && (SDL_PollEvent(&_event))) {
			switch (_event.type) {
			case SDL_MOUSEWHEEL:
				if (_event.wheel.y > 0) {
					if (LCTRL) {
						if (block_size < 256)
							block_size <<= 1;
						break;
					}

					current_object.raw = clamp(current_object.raw + 1,
											   GO_FIRST.raw, GO_LAST.raw);
				} else {
					if (LCTRL) {
						if (block_size > 1)
							block_size >>= 1;
						break;
					}

					current_object.raw = clamp(current_object.raw - 1,
											   GO_FIRST.raw, GO_LAST.raw);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				switch (_event.button.button) {
				case SDL_BUTTON_X1:
					if (block_size > 1)
						block_size >>= 1;
					break;
				case SDL_BUTTON_X2:
					if (block_size < 256)
						block_size <<= 1;
					break;
				}
				break;
			/* Handle events keys */
			case SDL_KEYDOWN: {
				switch (_event.key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					if (!PAUSED)
						PAUSED = true;
					else
						F_RESUME();
					break;
				case SDL_SCANCODE_F4:
					Render_TogleFullscreen;
					ResetSubchunks; /* Redraw */
					break;
				case SDL_SCANCODE_F9:
					/* Toggle debug level UI */
					if (DBGL(e_dbgl_ui)) {
						DEBUG_LEVEL &= ~e_dbgl_ui;
						ResetSubchunks; /* Clear debug artifacts */
					} else {
						DEBUG_LEVEL |= e_dbgl_ui;
					}
					break;
				case SDL_SCANCODE_F10:
					/* Toggle debug level Engine */
					if (DBGL(e_dbgl_engine)) {
						DEBUG_LEVEL &= ~e_dbgl_engine;
						ResetSubchunks; /* Clear debug artifacts */
					} else {
						DEBUG_LEVEL |= e_dbgl_engine;
					}
					break;
				case SDL_SCANCODE_F11:
					/* Toggle debug level Engine */
					if (DBGL(e_dbgl_physics)) {
						DEBUG_LEVEL &= ~e_dbgl_physics;
						box2d_debug_draw_active(false);
						ResetSubchunks; /* Clear debug artifacts */
					} else {
						DEBUG_LEVEL |= e_dbgl_physics;
						box2d_debug_draw_active(true);
					}
					break;
				case SDL_SCANCODE_SPACE:
					player.flying = !player.flying;
					box2d_body_change_type(player.body, !player.flying
															? b2_dynamicBody
															: b2_kinematicBody);
					break;
				case SDL_SCANCODE_LCTRL:
					LCTRL = true;
					break;
				case SDL_SCANCODE_G:
					grid_mode = !grid_mode;
					break;
				case SDL_SCANCODE_Q: {
					const bool fx = box2d_body_get_fixed_rotation(player.body);

					if (!fx)
						box2d_body_set_angle(player.body, 0);

					box2d_body_set_fixed_rotation(player.body, !fx);
				} break;
				case SDL_SCANCODE_KP_MINUS:
					if (block_size > 1)
						block_size >>= 1;
					break;
				case SDL_SCANCODE_KP_PLUS:
					if (block_size < 256)
						block_size <<= 1;
					break;
				case SDL_SCANCODE_RIGHTBRACKET:
					current_object.raw = clamp(current_object.raw + 1,
											   GO_FIRST.raw, GO_LAST.raw);
					break;
				case SDL_SCANCODE_LEFTBRACKET:
					current_object.raw = clamp(current_object.raw - 1,
											   GO_FIRST.raw, GO_LAST.raw);
					break;
				}
			} break;
			case SDL_KEYUP: {
			case SDL_SCANCODE_LCTRL:
				LCTRL = false;
				break;
			} break;

			/* Handle SDL events */
			case SDL_QUIT:
				GAME_ON = false;
				continue;
			case SDL_WINDOWEVENT:
				switch (_event.window.event) {
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					Render_ResizeWindow(_event.window.data1,
										_event.window.data2, true);
					SDL_RenderGetViewport(__renderer, &window_viewport);
					SDL_RenderGetScale(__renderer, &window_scale.x,
									   &window_scale.y);
					break;
				}
				break;
			}
		}

		mouse_x = clamp((mouse_x / window_scale.x - window_viewport.x), 0,
						VIEWPORT_WIDTH);
		mouse_y = clamp((mouse_y / window_scale.y - window_viewport.y), 0,
						VIEWPORT_HEIGHT);
		const uint32_t mouse_wold_x =
			clamp((mouse_x + (int)camera.x), 0, VSCREEN_WIDTH);
		const uint32_t mouse_wold_y =
			clamp((mouse_y + (int)camera.y), 0, VSCREEN_HEIGHT);

		/* =============================================================== */
		/* Update game */

		/* Place/remove object at mouse pencil */
		if (!PAUSED && (mouse_buttons & (SDL_BUTTON(SDL_BUTTON_LEFT) |
										 SDL_BUTTON(SDL_BUTTON_RIGHT))) != 0) {
			GO_ID _object = current_object;
			if (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
				current_object = GO_NONE;
			}

			if (block_size == 1) {
				gameboard[mouse_wold_y][mouse_wold_x] = current_object;
				subchunk_set_world(mouse_wold_x, mouse_wold_y);
				/* Mark Chunk at mouse position as modified */
				vctable[(mouse_wold_x / CHUNK_SIZE)]
					   [(mouse_wold_y / CHUNK_SIZE)]
						   .modified = 1;
			} else {
				int_fast16_t bx =
					(grid_mode
						 ? GRIDALIGN(mouse_wold_x, block_size) + block_size / 2
						 : mouse_wold_x);
				int_fast16_t by =
					(grid_mode
						 ? GRIDALIGN(mouse_wold_y, block_size) + block_size / 2
						 : mouse_wold_y);

				for (int_fast16_t j = clamp_low(by - block_size / 2, 0);
					 j < clamp_high(by + block_size / 2, VSCREEN_HEIGHT); ++j) {
					for (int_fast16_t i = clamp_low(bx - block_size / 2, 0);
						 i < clamp_high(bx + block_size / 2, VSCREEN_WIDTH);
						 ++i) {
						gameboard[j][i] = current_object;
						subchunk_set_world(i, j);
						/* Mark Chunk at brush size position as modified */
						vctable[(i / CHUNK_SIZE)][(j / CHUNK_SIZE)].modified =
							1;
					}
				}
			}

			current_object = _object;
		}

		/* Move player before world_step */
		if (!PAUSED)
			move_player(&player, SDL_GetKeyboardState(NULL));
		else
			canvas_process(&pause_canvas, mouse_buttons, mouse_x, mouse_y);

		box2d_world_step(b2_world, FPS_DELTA, 10, 8);

		/* Calculate soil collisions around player */
		const size_t si = (size_t)(player.x) / SUBCHUNK_WIDTH;
		const size_t sj = (size_t)(player.y) / SUBCHUNK_HEIGHT;
		for (size_t j = sj - 2; j <= sj + 2; ++j) {
			for (size_t i = si - 2; i <= si + 2; ++i) {
				if (i >= si - 1 && i <= si + 1 && j >= sj - 1 && j <= sj + 1)
					activate_soil(i, j);
				else
					deactivate_soil(i, j);
			}
		}

		/* Recalculate soil for active subchunks, not every frame since it is
		 * expensive */
		if (frame_cx % 4 == 0)
			for (uint_fast8_t sj = 0; sj < SUBCHUNK_SIZE; ++sj)
				for (uint_fast8_t si = 0; si < SUBCHUNK_SIZE; ++si)
					if (is_subchunk_active(si, sj))
						recalculate_soil(si, sj);

		move_camera(&player, &camera); /* After world_step */

		/* Update gameboard, entities and physics after all */
		update_gameboard();

		/* =============================================================== */
		/* Step animations */
		step_animation(player.animation, dt);

		/* =============================================================== */
		/* Draw game */
		draw_sky(&camera);

		/* Draw player */
		draw_player(&player, &camera);

		/* Draw gameboard */
		draw_gameboard_world(&camera);

		/* Debug draw */
		if (DBGL(e_dbgl_physics)) {
			SDL_Rect icamera = {(int)camera.x, (int)camera.y, (int)camera.w,
								(int)camera.h};
			box2d_debug_draw(b2_world, (Rect *)&icamera);
		}

		/* Draw mouse pointer */
		if (!PAUSED) {
			Color color;
			memcpy(&color, &(GOBJECT(current_object).color), sizeof(color));
			color.a = 0xAF;

			Render_Setcolor(color);

			if (block_size == 1) {
				Render_Pixel(mouse_x, mouse_y);
			} else {
				int_fast16_t bx =
					(grid_mode ? (GRIDALIGN(mouse_wold_x, block_size) -
								  (int)camera.x) +
									 block_size / 2
							   : mouse_x);
				int_fast16_t by =
					(grid_mode ? (GRIDALIGN(mouse_wold_y, block_size) -
								  (int)camera.y) +
									 block_size / 2
							   : mouse_y);
				int y_from = clamp_low((by - block_size / 2), 0);
				int x_from = clamp_low((bx - block_size / 2), 0);

				Render_FillRect(x_from, y_from, block_size, block_size);
			}
		}

		/* =============================================================== */
		/* Draw UI */
		if (DBGL(e_dbgl_ui)) {
			Render_Setcolor(C_WHITE);
			snprintf(fps_str, sizeof(fps_str), "%2zu", fps_);
			draw_string(fps_str, VIEWPORT_WIDTH - FSTR_WIDTH(fps_str), 0);

			char str_xy[14];
			snprintf(str_xy, sizeof(str_xy), "%i,%i", player.chunk_id.x,
					 player.chunk_id.y);
			draw_string(str_xy, 0, 0);
		}

		if (PAUSED) {
			/* Draw transparent black background */
			Render_Setcolor(C_DARK2);
			Render_FillRect(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

			canvas_draw(&pause_canvas);
		}

		/* =============================================================== */
		/* Render game */
		Render_Update;

		/* =============================================================== */
		/* Calculate ticks (adjust FPS) */
		frameTicks = SDL_GetTicks() - currentTicks;
		if (frameTicks < FRAME_DELAY_MS)
			SDL_Delay(FRAME_DELAY_MS - frameTicks);

		++frame_cx;
		if (frame_cx % 4 == 0) {
			fps_ = CALCULATE_FPS(delta);
			if (fps_ > FPS - 5)
				fps_ = FPS;
		}
	}

	canvas_delete(&pause_canvas);

	delete (player.sprite);

	if (player.body)
		box2d_body_destroy(player.body);

	if (b2_world)
		box2d_world_destroy(b2_world);

	return 0;
}
