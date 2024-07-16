#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>

#include "assets/assets.h"
#include "engine/engine.h"
#include "engine/gameobjects.h"
#include "engine/noise.h"
#include "graphics/color.h"
#include "graphics/graphics.h"
#include "util.h"

#define FPS			   60
#define FRAME_DELAY_MS (1000 / FPS)

#define VIEWPORT_WIDTH		  228
#define VIEWPORT_HEIGHT		  128
#define VIEWPORT_WIDTH_M1	  (VIEWPORT_WIDTH - 1)
#define VIEWPORT_HEIGHT_M1	  (VIEWPORT_HEIGHT - 1)
#define VIEWPORT_WIDTH_DIV_2  (VIEWPORT_WIDTH / 2)
#define VIEWPORT_HEIGHT_DIV_2 (VIEWPORT_HEIGHT / 2)

#define VSCREEN_WIDTH	  (VIEWPORT_WIDTH * 3)
#define VSCREEN_HEIGHT	  (VIEWPORT_HEIGHT)
#define VSCREEN_WIDTH_M1  (VSCREEN_WIDTH - 1)
#define VSCREEN_HEIGHT_M1 (VSCREEN_HEIGHT - 1)

#define RENDER_XFACTOR 4
#define RENDER_YFACTOR 4
static int WINDOW_WIDTH	 = VIEWPORT_WIDTH * RENDER_XFACTOR;
static int WINDOW_HEIGHT = VIEWPORT_HEIGHT * RENDER_YFACTOR;

#define IS_IN_BOUNDS_H(x)  ((x >= 0) && (x < VSCREEN_WIDTH))
#define IS_IN_BOUNDS_V(y)  ((y >= 0) && (y < VSCREEN_HEIGHT))
#define IS_IN_BOUNDS(x, y) (IS_IN_BOUNDS_H(x) && IS_IN_BOUNDS_V(y))

static volatile bool GAME_ON = true;

static byte gameboard[VSCREEN_HEIGHT][VSCREEN_WIDTH];

void update_object(int x, int y);
void generate_chunk(seed_t SEED, Chunk CHUNK, const int vx, const int vy);
void ResizeWindow(int newWidth, int newHeight, SDL_Rect viewport,
				  SDL_FPoint *new_scale);

/** Handle `CTRL + C` to quit the game */
void sigkillHandler(int signum) { GAME_ON = false; }

int main(int argc, char *argv[]) {
	signal(SIGINT, sigkillHandler);
	signal(SIGKILL, sigkillHandler);

	/* Set random seed */
	// srand(time(NULL));
	srand(0);

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
	/* Init gameloop variables */
	SDL_Rect   viewport = {-VIEWPORT_WIDTH, 0, VSCREEN_WIDTH, VIEWPORT_HEIGHT};
	SDL_FPoint __renderer_scale;
	short	   block_size	  = 1 << 3;
	bool	   grid_mode	  = false;
	byte	   current_object = GO_STONE;

	SDL_RenderGetScale(__renderer, &__renderer_scale.x, &__renderer_scale.y);

	/* =============================================================== */
	/* Initialize data */
	seed_t SEED		 = 0;
	Chunk  chunk_idx = {.x = CHUNK_MAX_X / 2, .y = 3};

	/* Generate world */
	generate_chunk(SEED, chunk_idx, 0, 0);
	generate_chunk(SEED, (Chunk)(chunk_idx.id + 1), VIEWPORT_WIDTH, 0);
	generate_chunk(SEED, (Chunk)(chunk_idx.id + 2), VIEWPORT_WIDTH * 2, 0);

	SDL_Point player = {32 + VIEWPORT_WIDTH, 32};
	viewport.x		 = clamp(-player.x + VIEWPORT_WIDTH_DIV_2,
							 -VSCREEN_WIDTH + VIEWPORT_WIDTH, 0);
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
				case SDL_SCANCODE_A:
					player.x = clamp(player.x - 2, 0, VSCREEN_WIDTH);

					/* If went to left chunk.
					 * Move world to right and generate new chunks in left
					 */
					if (player.x < VIEWPORT_WIDTH) {
						player.x =
							VIEWPORT_WIDTH * 2 - (VIEWPORT_WIDTH - player.x);
						--chunk_idx.x;
						for (int j = 0; j < VSCREEN_HEIGHT; ++j) {
							memmove(&gameboard[j][VIEWPORT_WIDTH * 2],
									&gameboard[j][VIEWPORT_WIDTH],
									VIEWPORT_WIDTH);
							memmove(&gameboard[j][VIEWPORT_WIDTH],
									&gameboard[j][0], VIEWPORT_WIDTH);
						}
						generate_chunk(SEED, (Chunk)(chunk_idx.id - 1), 0, 0);
					}

					viewport.x = clamp(-player.x + VIEWPORT_WIDTH_DIV_2,
									   -VSCREEN_WIDTH + VIEWPORT_WIDTH, 0);
					SDL_RenderSetViewport(__renderer, &viewport);
					break;
				case SDL_SCANCODE_D:
					player.x = clamp(player.x + 2, 0, VSCREEN_WIDTH);

					/* If went to right chunk.
					 * Move world to left and generate new chunks in right
					 */
					if (player.x >= VIEWPORT_WIDTH * 2) {
						player.x =
							VIEWPORT_WIDTH + (player.x - VIEWPORT_WIDTH * 2);
						++chunk_idx.x;
						for (int j = 0; j < VSCREEN_HEIGHT; ++j) {
							memmove(&gameboard[j][0],
									&gameboard[j][VIEWPORT_WIDTH],
									VIEWPORT_WIDTH);
							memmove(&gameboard[j][VIEWPORT_WIDTH],
									&gameboard[j][VIEWPORT_WIDTH * 2],
									VIEWPORT_WIDTH);
						}
						generate_chunk(SEED, (Chunk)(chunk_idx.id + 1),
									   VIEWPORT_WIDTH * 2, 0);
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
				int bx =
					(grid_mode ? GRIDALIGN(mouse_x, block_size) + block_size / 2
							   : mouse_x);
				int by =
					(grid_mode ? GRIDALIGN(mouse_y, block_size) + block_size / 2
							   : mouse_y);

				for (int j = clamp(by - block_size / 2, 0, VSCREEN_HEIGHT);
					 j < clamp(by + block_size / 2, 0, VSCREEN_HEIGHT); ++j) {
					for (int i = clamp(bx - block_size / 2, 0, VSCREEN_WIDTH);
						 i < clamp(bx + block_size / 2, 0, VSCREEN_WIDTH);
						 ++i) {
						gameboard[j][i] = current_object;
					}
				}
			}

			current_object = _object;
		}

		/* Update objects behaviour */
		for (int j = VSCREEN_HEIGHT_M1; j >= 0; --j) {
			/* Horizontal loop has to be first evens and then odds, in order to
			 * save some bugs with fluids */
			for (int i = 0; i < VSCREEN_WIDTH; i += 2) {
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
		// for (int j = SCREEN_HEIGHT_M1; j >= 0; --j) {
		for (int j = 0; j < VSCREEN_HEIGHT; ++j) {
			// for (int i = SCREEN_WIDTH_M1; i >= 0; --i) {
			for (int i = 0; i < VSCREEN_WIDTH; ++i) {
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
			int bx =
				(grid_mode ? GRIDALIGN(mouse_x, block_size) + block_size / 2
						   : mouse_x);
			int by =
				(grid_mode ? GRIDALIGN(mouse_y, block_size) + block_size / 2
						   : mouse_y);

			for (int j = by - block_size / 2; j < by + block_size / 2; ++j) {
				for (int i = bx - block_size / 2; i < bx + block_size / 2;
					 ++i) {
					Render_Pixel_Color(i, j, color);
				}
			}
		}

		Render_Pixel_Color(player.x, player.y, C_RED);

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

void update_object(int x, int y) {
	byte *boardxy = &gameboard[y][x];
	byte  type	  = GOBJECT(*boardxy);

	/* Update object behaviour */
	int left_or_right = (((rand() % 2) == 0) ? 1 : -1);

	int up_y	= y - 1;
	int down_y	= y + 1;
	int left_x	= x - left_or_right;
	int right_x = x + left_or_right;

	switch (type) {
	case GO_SAND: {
		byte *bottom	  = &gameboard[down_y][x];
		byte *bottomleft  = &gameboard[down_y][left_x];
		byte *bottomright = &gameboard[down_y][right_x];

		if (IS_IN_BOUNDS(x, down_y)) {
			if (*bottom == GO_NONE) {
				*boardxy = GO_NONE;
				*bottom	 = GUPDATE(GO_SAND);
			}

			else if (IS_IN_BOUNDS_H(left_x) && *bottomleft == GO_NONE) {
				*boardxy	= GO_NONE;
				*bottomleft = GUPDATE(GO_SAND);
			} else if (IS_IN_BOUNDS_H(right_x) && *bottomright == GO_NONE) {
				*boardxy	 = GO_NONE;
				*bottomright = GUPDATE(GO_SAND);
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
		}

		else if (IS_IN_BOUNDS(left_x, down_y) && *bottomleft == GO_NONE) {
			*boardxy	= GO_NONE;
			*bottomleft = GUPDATE(GO_WATER);
		} else if (IS_IN_BOUNDS(right_x, down_y) && *bottomright == GO_NONE) {
			*boardxy	 = GO_NONE;
			*bottomright = GUPDATE(GO_WATER);
		}

		else if (IS_IN_BOUNDS(left_x, y) && *left == GO_NONE) {
			*boardxy = GO_NONE;
			*left	 = GUPDATE(GO_WATER);
		} else if (IS_IN_BOUNDS(right_x, y) && *right == GO_NONE) {
			*boardxy = GO_NONE;
			*right	 = GUPDATE(GO_WATER);
		}

		/* Flow up in more dense fluids */
		else if (IS_IN_BOUNDS(x, up_y) && !IS_GUPDATED(*top) &&
				 *top > *boardxy && GO_IS_FLUID(*top)) {
			*boardxy |= BITMASK_GO_UPDATED;
			*top |= BITMASK_GO_UPDATED;
			SWAP((*top), (*boardxy));
		}
	} break;
	}
}

void generate_chunk(seed_t SEED, Chunk CHUNK, const int vx, const int vy) {
	int vx_max = vx + VIEWPORT_WIDTH;
	int vy_max = vy + VIEWPORT_HEIGHT;

	int world_x0 = CHUNK.x * VIEWPORT_WIDTH;
	int world_y0 = CHUNK.y * VIEWPORT_HEIGHT;

	for (int y = vy; y < vy_max; ++y) {
		for (int x = vx; x < vx_max; ++x) {
			gameboard[y][x] = GO_STONE;
		}
	}

	const int ground_height = 128;
	for (int x = vx; x < vx_max; ++x) {
		const int ground =
			fabs(perlin2d(SEED, world_x0 + x, 0, 0.005, 1) * ground_height);
		for (int y = vy; y < vy_max; ++y) {
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
	viewport.y = clamp(viewport.y, -VSCREEN_WIDTH + VIEWPORT_WIDTH, 0);
	SDL_RenderSetViewport(__renderer, &viewport);
}
