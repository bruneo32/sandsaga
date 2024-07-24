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

static volatile bool GAME_ON = true;

#define PLAYER_SPEED 4
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

	Render_Clearscreen_Color(C_BLUE);
	Render_Update;

	SDL_SetWindowSize(__window, VIEWPORT_WIDTH_M2, VIEWPORT_HEIGHT_M2);
	SDL_SetWindowPosition(__window, SDL_WINDOWPOS_CENTERED,
						  SDL_WINDOWPOS_CENTERED);

	/* =============================================================== */
	/* Load resources */
	player.sprite = loadIMG_from_mem(
		res_player_body_png, res_player_body_png_len, __window, __renderer);
	player_head = loadIMG_from_mem(res_player_head_png, res_player_head_png_len,
								   __window, __renderer);

	Font_SetCurrent(res_VGA_ROM_F08);

	/* =============================================================== */
	/* Init gameloop variables */
	SDL_Rect   window_viewport;
	SDL_FPoint window_scale;
	short	   block_size	  = 1 << 3;
	bool	   grid_mode	  = false;
	byte	   current_object = GO_STONE;

	SDL_RenderGetScale(__renderer, &window_scale.x, &window_scale.y);

	/* =============================================================== */
	/* Initialize data */
	seed_t SEED		= rand();
	player.chunk_id = (Chunk){.x = 3, .y = 3};
	SDL_Rect camera = {0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT};

	/* Generate world first instance*/
	chunk_axis_t chunk_start_x = player.chunk_id.x - 1;
	chunk_axis_t chunk_start_y = player.chunk_id.y - 1;
	for (chunk_axis_t j = chunk_start_y; j <= player.chunk_id.y + 1; ++j) {
		for (chunk_axis_t i = chunk_start_x; i <= player.chunk_id.x + 1; ++i) {
			Chunk chunk = (Chunk){.x = i, .y = j};
			generate_chunk(SEED, chunk, (i - chunk_start_x) * VIEWPORT_WIDTH,
						   (j - chunk_start_y) * VIEWPORT_HEIGHT);
		}
	}

	player.x = VIEWPORT_WIDTH + 32;
	player.y = VIEWPORT_HEIGHT + 32;

	camera.x = clamp(player.x - VIEWPORT_WIDTH_DIV_2 + 8, 0,
					 VSCREEN_WIDTH - VIEWPORT_WIDTH);

	camera.y = clamp(player.y - VIEWPORT_HEIGHT_DIV_2 + 12, 0,
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
		Uint32		 mouse_x, mouse_y;
		const Uint32 mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
		const Uint8 *keyboard	   = SDL_GetKeyboardState(NULL);

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
				case SDL_SCANCODE_G:
					grid_mode = !grid_mode;
					break;
				case SDL_SCANCODE_W:
					player.y =
						clamp(player.y - PLAYER_SPEED, 0, VSCREEN_HEIGHT);

					/* If went to top chunk.
					 * Move world to bottom and generate new chunks in top
					 */
					if (player.chunk_id.y > 1 && player.y < VIEWPORT_HEIGHT) {
						player.y =
							VIEWPORT_HEIGHT * 2 - (VIEWPORT_HEIGHT - player.y);
						--player.chunk_id.y;

						/* Move world to bottom */
						memmove(&gameboard[VIEWPORT_HEIGHT][0],
								&gameboard[0][0],
								VIEWPORT_HEIGHT * 2 * VSCREEN_WIDTH);

						/* Generate new world at top */
						chunk_axis_t start_x = player.chunk_id.x - 1;
						for (uint_fast8_t i = 0; i < 3; ++i) {
							Chunk chunk = {.x = start_x + i,
										   .y = player.chunk_id.y - 1};
							generate_chunk(SEED, chunk, i * VIEWPORT_WIDTH, 0);
						}
					}

					camera.y = clamp(player.y - VIEWPORT_HEIGHT_DIV_2 + 12, 0,
									 VSCREEN_HEIGHT - VIEWPORT_HEIGHT);
					break;
				case SDL_SCANCODE_S:
					player.y =
						clamp(player.y + PLAYER_SPEED, 0, VSCREEN_HEIGHT);

					/* If went to bottom chunk.
					 * Move world to top and generate new chunks in bottom
					 */
					if (player.chunk_id.y < CHUNK_MAX_Y - 1 &&
						player.y >= VIEWPORT_HEIGHT * 2) {
						player.y =
							VIEWPORT_HEIGHT + (player.y - VIEWPORT_HEIGHT * 2);
						++player.chunk_id.y;

						/* Move world to top */
						memmove(&gameboard[0][0],
								&gameboard[VIEWPORT_HEIGHT][0],
								VIEWPORT_HEIGHT * 2 * VSCREEN_WIDTH);

						/* Generate new world at bottom */
						chunk_axis_t start_x = player.chunk_id.x - 1;
						for (uint_fast8_t i = 0; i < 3; ++i) {
							Chunk chunk = {.x = start_x + i,
										   .y = player.chunk_id.y + 1};
							generate_chunk(SEED, chunk, i * VIEWPORT_WIDTH,
										   VIEWPORT_HEIGHT * 2);
						}
					}

					camera.y = clamp(player.y - VIEWPORT_HEIGHT_DIV_2 + 12, 0,
									 VSCREEN_HEIGHT - VIEWPORT_HEIGHT);
					break;
				case SDL_SCANCODE_A:
					player.x = clamp(player.x - PLAYER_SPEED, 0, VSCREEN_WIDTH);
					player.fliph = true;

					/* If went to left chunk.
					 * Move world to right and generate new chunks in left
					 */
					if (player.chunk_id.x > 1 && player.x < VIEWPORT_WIDTH) {
						player.x =
							VIEWPORT_WIDTH_M2 - (VIEWPORT_WIDTH - player.x);
						--player.chunk_id.x;

						/* Move world to right */
						for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
							memmove(&gameboard[j][VIEWPORT_WIDTH_M2],
									&gameboard[j][VIEWPORT_WIDTH],
									VIEWPORT_WIDTH);
							memmove(&gameboard[j][VIEWPORT_WIDTH],
									&gameboard[j][0], VIEWPORT_WIDTH);
						}

						/* Generate new world at left */
						chunk_axis_t start_j = player.chunk_id.y - 1;
						for (uint_fast8_t j = 0; j < 3; ++j) {
							Chunk chunk = {.x = player.chunk_id.x - 1,
										   .y = start_j + j};
							generate_chunk(SEED, chunk, 0, j * VIEWPORT_HEIGHT);
						}
					}
					camera.x = clamp(player.x - VIEWPORT_WIDTH_DIV_2 + 8, 0,
									 VSCREEN_WIDTH - VIEWPORT_WIDTH);
					break;
				case SDL_SCANCODE_D:
					player.x = clamp(player.x + PLAYER_SPEED, 0, VSCREEN_WIDTH);
					player.fliph = false;

					/* If went to right chunk.
					 * Move world to left and generate new chunks in right
					 */
					if (player.chunk_id.x < CHUNK_MAX_X - 1 &&
						player.x >= VIEWPORT_WIDTH_M2) {
						player.x =
							VIEWPORT_WIDTH + (player.x - VIEWPORT_WIDTH_M2);
						++player.chunk_id.x;

						/* Move world to right */
						for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
							memmove(&gameboard[j][0],
									&gameboard[j][VIEWPORT_WIDTH],
									VIEWPORT_WIDTH);
							memmove(&gameboard[j][VIEWPORT_WIDTH],
									&gameboard[j][VIEWPORT_WIDTH_M2],
									VIEWPORT_WIDTH);
						}

						/* Generate new world at right */
						chunk_axis_t start_j = player.chunk_id.y - 1;
						for (uint_fast8_t j = 0; j < 3; ++j) {
							Chunk chunk = {.x = player.chunk_id.x + 1,
										   .y = start_j + j};
							generate_chunk(SEED, chunk, VIEWPORT_WIDTH_M2,
										   j * VIEWPORT_HEIGHT);
						}
					}

					camera.x = clamp(player.x - VIEWPORT_WIDTH_DIV_2 + 8, 0,
									 VSCREEN_WIDTH - VIEWPORT_WIDTH);
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

		mouse_x = mouse_x / window_scale.x - window_viewport.x;
		mouse_y = mouse_y / window_scale.y - window_viewport.y;
		const uint32_t mouse_wold_x = mouse_x + camera.x;
		const uint32_t mouse_wold_y = mouse_y + camera.y;

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
			} else {
				uint_fast16_t bx =
					(grid_mode
						 ? GRIDALIGN(mouse_wold_x, block_size) + block_size / 2
						 : mouse_wold_x);
				uint_fast16_t by =
					(grid_mode
						 ? GRIDALIGN(mouse_wold_y, block_size) + block_size / 2
						 : mouse_wold_y);

				for (uint_fast16_t j =
						 clamp(by - block_size / 2, 0, VSCREEN_HEIGHT);
					 j < clamp(by + block_size / 2, 0, VSCREEN_HEIGHT); ++j) {
					for (uint_fast16_t i =
							 clamp(bx - block_size / 2, 0, VSCREEN_WIDTH);
						 i < clamp(bx + block_size / 2, 0, VSCREEN_WIDTH);
						 ++i) {
						gameboard[j][i] = current_object;
					}
				}
			}

			current_object = _object;
		}

		/* Update objects behaviour */
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

		/* =============================================================== */
		/* Draw game */
		Render_Clearscreen_Color(GO_COLORS[GO_NONE]);

		/* Draw player */
		const uint_fast16_t player_screen_x = player.x - camera.x;
		const uint_fast16_t player_screen_y = player.y - camera.y;

		Render_image_ext(player.sprite->texture, player_screen_x,
						 player_screen_y, 16, 24, 0, NULL, player.fliph);
		Render_image_ext(player_head->texture, player_screen_x, player_screen_y,
						 16, 8, 0, NULL, player.fliph);

		/* Draw gameboard */
		for (uint_fast16_t j = 0; j < camera.h; ++j) {
			const uint_fast16_t y = j + camera.y;

			for (uint_fast16_t i = 0; i < camera.w; ++i) {
				const uint_fast16_t x = i + camera.x;

				const byte pixel = GOBJECT(gameboard[y][x]);
				if (pixel != GO_NONE) {
					gameboard[y][x] = pixel; /* Reset updated bit */
					Render_Pixel_Color(i, j, GO_COLORS[pixel]);
				}
			}
		}

		/* Draw mouse pointer */
		Color color;
		memcpy(&color, &GO_COLORS[current_object], sizeof(color));
		color.a = 0xAF;

		if (block_size == 1) {
			Render_Pixel_Color(mouse_x, mouse_y, color);
		} else {
			uint_fast16_t bx =
				(grid_mode ? GRIDALIGN(mouse_x, block_size) + block_size / 2
						   : mouse_x);
			uint_fast16_t by =
				(grid_mode ? GRIDALIGN(mouse_y, block_size) + block_size / 2
						   : mouse_y);

			for (uint_fast16_t j = by - block_size / 2; j < by + block_size / 2;
				 ++j) {
				for (uint_fast16_t i = bx - block_size / 2;
					 i < bx + block_size / 2; ++i) {
					Render_Pixel_Color(i, j, color);
				}
			}
		}

		/* =============================================================== */
		/* Draw UI */
		char fps_str[4];
		sprintf(fps_str, "%2i", CALCULATE_FPS(delta));

		Render_Setcolor(C_WHITE);
		draw_string(fps_str, VIEWPORT_WIDTH - FSTR_WIDTH(fps_str), 0);

		/* =============================================================== */
		/* Render game */
		Render_Update;

		/* =============================================================== */
		/* Calculate ticks (adjust FPS) */
		frameTicks = SDL_GetTicks() - currentTicks;
		if (frameTicks < FRAME_DELAY_MS)
			SDL_Delay(FRAME_DELAY_MS - frameTicks);
	}

	return 0;
}
