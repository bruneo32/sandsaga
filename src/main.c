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

static int WINDOW_WIDTH	 = VIEWPORT_WIDTH * RENDER_XFACTOR;
static int WINDOW_HEIGHT = VIEWPORT_HEIGHT * RENDER_YFACTOR;

#define PLAYER_SPEED 4
static Player  player;
static Sprite *player_head;

void ResizeWindow(int newWidth, int newHeight, SDL_Rect viewport,
				  SDL_FPoint *new_scale);

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
	Render_init("Falling sand sandbox game", WINDOW_WIDTH, WINDOW_HEIGHT,
				VSCREEN_WIDTH, VSCREEN_HEIGHT);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "0");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	SDL_RenderSetScale(__renderer, RENDER_XFACTOR, RENDER_YFACTOR);

	Render_Clearscreen_Color(C_BLUE);
	Render_Update;

	/* =============================================================== */
	/* Load resources */
	player.sprite = loadIMG_from_mem(
		res_player_body_png, res_player_body_png_len, __window, __renderer);
	player_head = loadIMG_from_mem(res_player_head_png, res_player_head_png_len,
								   __window, __renderer);

	Font_SetCurrent(res_VGA_ROM_F08);

	/* =============================================================== */
	/* Init gameloop variables */
	SDL_Rect   viewport = {-VIEWPORT_WIDTH, -VIEWPORT_HEIGHT, VSCREEN_WIDTH,
						   VSCREEN_HEIGHT};
	SDL_FPoint __renderer_scale;
	short	   block_size	  = 1 << 3;
	bool	   grid_mode	  = false;
	byte	   current_object = GO_STONE;

	SDL_RenderGetScale(__renderer, &__renderer_scale.x, &__renderer_scale.y);

	/* =============================================================== */
	/* Initialize data */
	seed_t SEED		= rand();
	player.chunk_id = (Chunk){.x = CHUNK_MAX_X / 2, .y = 3};

	/* Generate world first instance*/
	chunk_axis_t chunk_start_x = player.chunk_id.x - 1;
	chunk_axis_t chunk_start_y = player.chunk_id.y - 1;
	for (chunk_axis_t j = chunk_start_y; j <= player.chunk_id.y + 1; ++j) {
		for (chunk_axis_t i = chunk_start_x; i <= player.chunk_id.x + 1; ++i) {
			Chunk chunk = (Chunk){.x = i, .y = chunk_start_y};
			generate_chunk(SEED, chunk, (i - chunk_start_x) * VIEWPORT_WIDTH,
						   (j - chunk_start_y) * VIEWPORT_HEIGHT);
		}
	}

	player.x   = VIEWPORT_WIDTH + 32;
	player.y   = VIEWPORT_HEIGHT + 32;
	viewport.x = clamp(-player.x + VIEWPORT_WIDTH_DIV_2,
					   -VSCREEN_WIDTH + VIEWPORT_WIDTH, 0);
	viewport.y = clamp(-player.y + VIEWPORT_HEIGHT_DIV_2,
					   -VSCREEN_HEIGHT + VIEWPORT_HEIGHT, 0);
	SDL_RenderSetViewport(__renderer, &viewport);

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
		const Uint8 *keyboard	   = SDL_GetKeyboardState(NULL);

		mouse_x = mouse_x / __renderer_scale.x - viewport.x;
		mouse_y = mouse_y / __renderer_scale.y - viewport.y;

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

					viewport.y = clamp(-player.y + VIEWPORT_HEIGHT_DIV_2,
									   -VSCREEN_HEIGHT + VIEWPORT_HEIGHT, 0);
					SDL_RenderSetViewport(__renderer, &viewport);
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

					viewport.y = clamp(-player.y + VIEWPORT_HEIGHT_DIV_2,
									   -VSCREEN_HEIGHT + VIEWPORT_HEIGHT, 0);
					SDL_RenderSetViewport(__renderer, &viewport);
					break;
				case SDL_SCANCODE_A:
					player.x = clamp(player.x - PLAYER_SPEED, 0, VSCREEN_WIDTH);
					player.fliph = true;

					/* If went to left chunk.
					 * Move world to right and generate new chunks in left
					 */
					if (player.chunk_id.x > 1 && player.x < VIEWPORT_WIDTH) {
						player.x =
							VIEWPORT_WIDTH * 2 - (VIEWPORT_WIDTH - player.x);
						--player.chunk_id.x;

						/* Move world to right */
						for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
							memmove(&gameboard[j][VIEWPORT_WIDTH * 2],
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

					viewport.x = clamp(-player.x + VIEWPORT_WIDTH_DIV_2,
									   -VSCREEN_WIDTH + VIEWPORT_WIDTH, 0);
					SDL_RenderSetViewport(__renderer, &viewport);
					break;
				case SDL_SCANCODE_D:
					player.x = clamp(player.x + PLAYER_SPEED, 0, VSCREEN_WIDTH);
					player.fliph = false;

					/* If went to right chunk.
					 * Move world to left and generate new chunks in right
					 */
					if (player.chunk_id.x < CHUNK_MAX_X - 1 &&
						player.x >= VIEWPORT_WIDTH * 2) {
						player.x =
							VIEWPORT_WIDTH + (player.x - VIEWPORT_WIDTH * 2);
						++player.chunk_id.x;

						/* Move world to right */
						for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
							memmove(&gameboard[j][0],
									&gameboard[j][VIEWPORT_WIDTH],
									VIEWPORT_WIDTH);
							memmove(&gameboard[j][VIEWPORT_WIDTH],
									&gameboard[j][VIEWPORT_WIDTH * 2],
									VIEWPORT_WIDTH);
						}

						/* Generate new world at right */
						chunk_axis_t start_j = player.chunk_id.y - 1;
						for (uint_fast8_t j = 0; j < 3; ++j) {
							Chunk chunk = {.x = player.chunk_id.x + 1,
										   .y = start_j + j};
							generate_chunk(SEED, chunk, VIEWPORT_WIDTH * 2,
										   j * VIEWPORT_HEIGHT);
						}
					}

					viewport.x = clamp(-player.x + VIEWPORT_WIDTH_DIV_2,
									   -VSCREEN_WIDTH + VIEWPORT_WIDTH, 0);
					SDL_RenderSetViewport(__renderer, &viewport);
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
					ResizeWindow(_event.window.data1, _event.window.data2,
								 viewport, &__renderer_scale);
					break;
				}
				break;
			}
		}

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
				gameboard[mouse_y][mouse_x] = current_object;
			} else {
				uint_fast16_t bx =
					(grid_mode ? GRIDALIGN(mouse_x, block_size) + block_size / 2
							   : mouse_x);
				uint_fast16_t by =
					(grid_mode ? GRIDALIGN(mouse_y, block_size) + block_size / 2
							   : mouse_y);

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
		for (uint_fast16_t j = VSCREEN_HEIGHT_M1; j > -1; --j) {
			/* Horizontal loop has to be first evens and then odds, in order to
			 * save some bugs with fluids */
			for (uint_fast16_t i = 0; i < VSCREEN_WIDTH; i += 2) {
				if (gameboard[j][i] != GO_NONE &&
					!IS_GUPDATED(gameboard[j][i])) {
					update_object(i, j);
				}

				/* Next: odd numbers */
				if (i == VSCREEN_WIDTH_M1 - 1)
					i = -1;
			}
		}

		/* =============================================================== */
		/* Draw game */
		Render_Clearscreen_Color(GO_COLORS[GO_NONE]);

		Render_image_ext(player.sprite->texture, player.x, player.y, 16, 24, 0,
						 (&(SDL_Point){8, 12}), player.fliph);
		Render_image_ext(player_head->texture, player.x, player.y, 16, 8, 0,
						 (&(SDL_Point){8, 4}), player.fliph);

		for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
			for (uint_fast16_t i = 0; i < VSCREEN_WIDTH; ++i) {
				if (gameboard[j][i] != GO_NONE) {
					gameboard[j][i] =
						GOBJECT(gameboard[j][i]); /* Reset updated bit */
					Render_Pixel_Color(i, j, GO_COLORS[gameboard[j][i]]);
				}
			}
		}

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
		draw_string(fps_str, -viewport.x + VIEWPORT_WIDTH - FSTR_WIDTH(fps_str),
					-viewport.y);

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

void ResizeWindow(int newWidth, int newHeight, SDL_Rect viewport,
				  SDL_FPoint *new_scale) {

	float scaleX = ((float)newWidth / (float)WINDOW_WIDTH);
	float scaleY = ((float)newHeight / (float)WINDOW_HEIGHT);

	float scale = fminf(scaleX, scaleY);

	/* Calculate centered positioning offset */
	int offsetX = (newWidth - scale) / scale / 2;
	int offsetY = (newHeight - scale) / scale / 2;

	new_scale->x = RENDER_XFACTOR * scale;
	new_scale->y = RENDER_YFACTOR * scale;
	SDL_RenderSetScale(__renderer, new_scale->x, new_scale->y);

	viewport.x = clamp(viewport.x, -VSCREEN_WIDTH + VIEWPORT_WIDTH, 0);
	viewport.y = clamp(viewport.y, -VSCREEN_HEIGHT + VIEWPORT_HEIGHT, 0);
	SDL_RenderSetViewport(__renderer, &viewport);
}
