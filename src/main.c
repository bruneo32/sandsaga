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
#include "assets/res/player_body.png.h"

#include "physics/physics.h"

#include "assets/assets.h"
#include "engine/engine.h"
#include "engine/entities.h"
#include "engine/gameobjects.h"
#include "engine/noise.h"
#include "graphics/color.h"
#include "graphics/font/font.h"
#include "graphics/graphics.h"
#include "util.h"

static size_t		 fps_	 = FPS;
static volatile bool GAME_ON = true;

static Player player;

/** Handle `CTRL + C` to quit the game */
void sigkillHandler(int signum) { GAME_ON = false; }

int main(int argc, char *argv[]) {
	signal(SIGINT, sigkillHandler);

	/* Set random seeds */
	const time_t _st = time(NULL);
	srand(_st);
	sfrand(_st);

	/* =============================================================== */
	/* Init SDL */
	Render_init("Sandsaga", VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "0");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

#ifdef _WIN32
	/* In windows, opengl could be suboptimal, d3d11 is the best balance
	 * between performance and compatibility */
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#endif

	SDL_RenderSetLogicalSize(__renderer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	SDL_SetWindowSize(__window, VIEWPORT_WIDTH_M2, VIEWPORT_HEIGHT_M2);
	SDL_SetWindowPosition(__window, SDL_WINDOWPOS_CENTERED,
						  SDL_WINDOWPOS_CENTERED);

	Render_Clearscreen_Color(C_BLUE);
	Render_Update;

	/* =============================================================== */
	/* Load resources */
	Font_SetCurrent(res_VGA_ROM_F08);

	player.sprite = loadIMG_from_mem(
		res_player_body_png, res_player_body_png_len, __window, __renderer);

	/* =============================================================== */
	/* Init gameloop variables */
	SDL_Rect   window_viewport;
	SDL_FPoint window_scale;
	size_t	   frame_cx = 0;
	char	   fps_str[4];
	snprintf(fps_str, sizeof(fps_str), "%2zu", fps_);
	short block_size	 = 1 << 3;
	bool  grid_mode		 = false;
	byte  current_object = GO_STONE;

	SDL_RenderGetViewport(__renderer, &window_viewport);
	SDL_RenderGetScale(__renderer, &window_scale.x, &window_scale.y);

	/* =============================================================== */
	/* Initialize data */
	WORLD_SEED		= rand();
	player.chunk_id = (Chunk){.x = CHUNK_MAX_X / 2, .y = GEN_SKY_Y - 1};

	/* Generate world first instance*/
	chunk_axis_t chunk_start_x = player.chunk_id.x - 1;
	chunk_axis_t chunk_start_y = player.chunk_id.y - 1;
	for (chunk_axis_t j = chunk_start_y; j <= player.chunk_id.y + 1; ++j) {
		for (chunk_axis_t i = chunk_start_x; i <= player.chunk_id.x + 1; ++i) {
			Chunk chunk = (Chunk){.x = i, .y = j};
			generate_chunk(WORLD_SEED, chunk, (i - chunk_start_x) * CHUNK_SIZE,
						   (j - chunk_start_y) * CHUNK_SIZE);
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

					current_object =
						clamp(current_object + 1, GO_FIRST, GO_LAST);
				} else {
					if (LCTRL) {
						if (block_size > 1)
							block_size >>= 1;
						break;
					}

					current_object =
						clamp(current_object - 1, GO_FIRST, GO_LAST);
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
					GAME_ON = false;
					continue;
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
				case SDL_SCANCODE_Q:
					const bool fx = box2d_body_get_fixed_rotation(player.body);

					if (!fx)
						box2d_body_set_angle(player.body, 0);

					box2d_body_set_fixed_rotation(player.body, !fx);
					break;
				case SDL_SCANCODE_KP_MINUS:
					if (block_size > 1)
						block_size >>= 1;
					break;
				case SDL_SCANCODE_KP_PLUS:
					if (block_size < 256)
						block_size <<= 1;
					break;
				case SDL_SCANCODE_RIGHTBRACKET:
					current_object =
						clamp(current_object + 1, GO_FIRST, GO_LAST);
					break;
				case SDL_SCANCODE_LEFTBRACKET:
					current_object =
						clamp(current_object - 1, GO_FIRST, GO_LAST);
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
		if ((mouse_buttons & (SDL_BUTTON(SDL_BUTTON_LEFT) |
							  SDL_BUTTON(SDL_BUTTON_RIGHT))) != 0) {
			byte _object = current_object;
			if (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
				current_object = GO_NONE;
			}

			if (block_size == 1) {
				gameboard[mouse_wold_y][mouse_wold_x] = current_object;
				set_subchunk_world(1, mouse_wold_x, mouse_wold_y);
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
						set_subchunk_world(1, i, j);
					}
				}
			}

			current_object = _object;
		}

		/* Move player before world_step */
		move_player(&player, SDL_GetKeyboardState(NULL));
		/* Recalculate soil for active subchunks, not every frame since it is
		 * expensive */
		if (frame_cx % 4 == 0)
			for (uint_fast8_t sj = 0; sj < SUBCHUNK_SIZE; ++sj)
				for (uint_fast8_t si = 0; si < SUBCHUNK_SIZE; ++si)
					if (is_subchunk_active(si, sj))
						recalculate_soil(si, sj);

		box2d_world_step(b2_world, FPS_DELTA, 10, 8);
		move_camera(&player, &camera); /* After world_step */

		/* Update gameboard, entities and physics after all */
		update_gameboard();

		/* =============================================================== */
		/* Step animations */
		step_animation(player.animation, dt);

		/* =============================================================== */
		/* Draw game */
		Render_Clearscreen_Color(C_DKGRAY);

		/* Draw player */
		draw_player(&player, &camera);

		/* Draw gameboard */
		SDL_SetRenderTarget(__renderer, vscreen_);
		SDL_SetRenderDrawBlendMode(__renderer, SDL_BLENDMODE_NONE);
		draw_gameboard_world(&camera);

		/* Draw gameboard on screen */
		SDL_SetRenderTarget(__renderer, NULL);
		SDL_SetRenderDrawBlendMode(__renderer, SDL_BLENDMODE_BLEND);
		SDL_Rect icamera = {(int)camera.x, (int)camera.y, (int)camera.w,
							(int)camera.h};
		SDL_RenderCopy(__renderer, vscreen_, &icamera, NULL);

		if (DBGL(e_dbgl_physics)) {
			box2d_debug_draw(b2_world, (Rect *)&icamera);
		}

		/* Draw mouse pointer */
		Color color;
		memcpy(&color, &GO_COLORS[current_object], sizeof(color));
		color.a = 0xAF;

		if (block_size == 1) {
			Render_Pixel_Color(mouse_x, mouse_y, color);
		} else {
			int_fast16_t bx =
				(grid_mode
					 ? (GRIDALIGN(mouse_wold_x, block_size) - (int)camera.x) +
						   block_size / 2
					 : mouse_x);
			int_fast16_t by =
				(grid_mode
					 ? (GRIDALIGN(mouse_wold_y, block_size) - (int)camera.y) +
						   block_size / 2
					 : mouse_y);

			for (int_fast16_t j = clamp_low((by - block_size / 2), 0);
				 j < clamp_high(by + block_size / 2, VIEWPORT_HEIGHT); ++j) {
				for (int_fast16_t i = clamp_low((bx - block_size / 2), 0);
					 i < clamp_high(bx + block_size / 2, VIEWPORT_WIDTH); ++i) {
					Render_Pixel_Color(i, j, color);
				}
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

	delete (player.sprite);

	if (player.body)
		box2d_body_destroy(player.body);

	if (b2_world)
		box2d_world_destroy(b2_world);

	return 0;
}
