#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>

#include "assets/res/VGA-ROM.F08.h"
#include "assets/res/player_body.png.h"
#include "assets/res/player_head.png.h"

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

static Player  player;
static Sprite *player_head;

/** Handle `CTRL + C` to quit the game */
void sigkillHandler(int signum) { GAME_ON = false; }

int main(int argc, char *argv[]) {
	signal(SIGINT, sigkillHandler);
	signal(SIGKILL, sigkillHandler);

	/* Set random seed */
	srand(time(NULL));
	// srand(0);

	/* =============================================================== */
	/* Init SDL */
	Render_init("Falling sand sandbox game", VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "0");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

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
	player_head = loadIMG_from_mem(res_player_head_png, res_player_head_png_len,
								   __window, __renderer);

	/* =============================================================== */
	/* Init gameloop variables */
	SDL_Rect   window_viewport;
	SDL_FPoint window_scale;
	size_t	   frame_cx = 0;
	char	   fps_str[4];
	snprintf(fps_str, sizeof(fps_str), "%2li", fps_);
	short block_size	 = 1 << 3;
	bool  grid_mode		 = false;
	byte  current_object = GO_STONE;

	SDL_RenderGetViewport(__renderer, &window_viewport);
	SDL_RenderGetScale(__renderer, &window_scale.x, &window_scale.y);

	/* =============================================================== */
	/* Initialize data */
	WORLD_SEED = rand();
	player.chunk_id =
		(Chunk){.x = GEN_WATERSEA_OFFSET_X + 1, .y = GEN_SKY_Y + 1};
	SDL_Rect camera = {0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT};

	/* Generate world first instance*/
	chunk_axis_t chunk_start_x = player.chunk_id.x - 1;
	chunk_axis_t chunk_start_y = player.chunk_id.y - 1;
	for (chunk_axis_t j = chunk_start_y; j <= player.chunk_id.y + 1; ++j) {
		for (chunk_axis_t i = chunk_start_x; i <= player.chunk_id.x + 1; ++i) {
			Chunk chunk = (Chunk){.x = i, .y = j};
			generate_chunk(WORLD_SEED, chunk,
						   (i - chunk_start_x) * VIEWPORT_WIDTH,
						   (j - chunk_start_y) * VIEWPORT_HEIGHT);
		}
	}
	ResetSubchunks;

	player.flying = true;
	player.width  = 16;
	player.height = 24;
	player.x	  = VIEWPORT_WIDTH + VIEWPORT_WIDTH_DIV_2;
	player.y	  = VIEWPORT_HEIGHT + VIEWPORT_HEIGHT_DIV_2;

	camera.x = clamp(player.x - VIEWPORT_WIDTH_DIV_2, 0,
					 VSCREEN_WIDTH - VIEWPORT_WIDTH);

	camera.y = clamp(player.y - VIEWPORT_HEIGHT_DIV_2, 0,
					 VSCREEN_HEIGHT - VIEWPORT_HEIGHT);

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
		prevTicks			= currentTicks;

		/* =============================================================== */
		/* Get inputs */
		int			 mouse_x, mouse_y;
		const Uint32 mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

		SDL_Event _event;
		while (GAME_ON && (SDL_PollEvent(&_event))) {
			switch (_event.type) {
			case SDL_MOUSEWHEEL:
				if (_event.wheel.y > 0) {
					current_object =
						clamp(current_object + 1, GO_FIRST, GO_LAST);
				} else {
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
					break;
				case SDL_SCANCODE_F3:
					DEBUG_ON = !DEBUG_ON;
					if (!DEBUG_ON)
						ResetSubchunks;
					break;
				case SDL_SCANCODE_G:
					grid_mode = !grid_mode;
					break;
				case SDL_SCANCODE_KP_MINUS:
					if (block_size > 1)
						block_size >>= 1;
					break;
				case SDL_SCANCODE_KP_PLUS:
					if (block_size < 256)
						block_size <<= 1;
					break;
				}
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
			clamp((mouse_x + camera.x), 0, VSCREEN_WIDTH);
		const uint32_t mouse_wold_y =
			clamp((mouse_y + camera.y), 0, VSCREEN_HEIGHT);

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

		/* Update objects behaviour */
		update_gameboard();
		move_player(&player, &camera, SDL_GetKeyboardState(NULL));

		/* =============================================================== */
		/* Draw game */
		Render_Clearscreen_Color(C_DKGRAY);

		/* Draw player */
		const uint_fast16_t player_screen_x = player.x - camera.x;
		const uint_fast16_t player_screen_y = player.y - camera.y;

		Render_image_ext(player.sprite->texture,
						 player_screen_x - (player.width / 2),
						 player_screen_y - (player.height / 2), player.width,
						 player.height, 0, NULL, player.fliph);
		Render_image_ext(player_head->texture,
						 player_screen_x - (player.width / 2),
						 player_screen_y - (player.height / 2), 16, 8, 0, NULL,
						 player.fliph);

		if (DEBUG_ON) {
			const short player_height_2 = player.height / 2;
			const short player_width_4	= player.width / 4;
			const short player_screen_top =
				player_screen_y - player_height_2 + 1;
			const short player_screen_bottom =
				player_screen_y + player_height_2 - 1;
			const short player_screen_left =
				player_screen_x - player_width_4 + 1;
			const short player_screen_right =
				player_screen_x + player_width_4 - 1;

			const size_t forward = (!player.fliph) ? 1 : -1;

			Render_Pixel_RGBA(player_screen_x, player_screen_y, 255, 0, 0, 255);

			Render_Pixel_RGBA(player_screen_left, player_screen_top, 0, 255,
							  255, 255);
			Render_Pixel_RGBA(player_screen_right, player_screen_top, 0, 255,
							  255, 255);

			Render_Pixel_RGBA(player_screen_left, player_screen_bottom, 0, 255,
							  0, 255);
			Render_Pixel_RGBA(player_screen_right, player_screen_bottom, 0, 255,
							  0, 255);

			Render_Pixel_RGBA(player_screen_left, player_screen_bottom - 4, 255,
							  255, 0, 255);
			Render_Pixel_RGBA(player_screen_right, player_screen_bottom - 4,
							  255, 255, 0, 255);

			Render_Pixel_RGBA(player_screen_x +
								  (forward * (player_width_4 - 1)),
							  player_screen_y, 255, 0, 255, 255);
		}

		/* Draw gameboard */
		SDL_SetRenderTarget(__renderer, vscreen_);
		draw_gameboard_world(&camera);
		SDL_SetRenderTarget(__renderer, NULL);

		SDL_SetRenderDrawBlendMode(__renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderCopy(__renderer, vscreen_, &camera, NULL);
		SDL_SetRenderDrawBlendMode(__renderer, SDL_BLENDMODE_NONE);

		/* Draw mouse pointer */
		Color color;
		memcpy(&color, &GO_COLORS[current_object], sizeof(color));
		color.a = 0xAF;

		if (block_size == 1) {
			Render_Pixel_Color(mouse_x, mouse_y, color);
		} else {
			int_fast16_t bx =
				(grid_mode ? (GRIDALIGN(mouse_wold_x, block_size) - camera.x) +
								 block_size / 2
						   : mouse_x);
			int_fast16_t by =
				(grid_mode ? (GRIDALIGN(mouse_wold_y, block_size) - camera.y) +
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
		if (DEBUG_ON) {
			Render_Setcolor(C_WHITE);
			snprintf(fps_str, sizeof(fps_str), "%2li", fps_);
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
	delete (player_head);

	return 0;
}
