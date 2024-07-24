#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <SDL.h>
#include <math.h>
#include <stdbool.h>

#include "../assets/assets.h"
#include "color.h"

#define FPS			   60
#define FRAME_DELAY_MS (1000 / FPS)
#define CALCULATE_FPS(delta)                                                   \
	(clamp(((int)(1.f / ((float)delta / 1000.f))), 0, FPS))

#define VIEWPORT_WIDTH		  454
#define VIEWPORT_HEIGHT		  256
#define VIEWPORT_WIDTH_M1	  (VIEWPORT_WIDTH - 1)
#define VIEWPORT_HEIGHT_M1	  (VIEWPORT_HEIGHT - 1)
#define VIEWPORT_WIDTH_DIV_2  (VIEWPORT_WIDTH / 2)
#define VIEWPORT_HEIGHT_DIV_2 (VIEWPORT_HEIGHT / 2)
#define VIEWPORT_WIDTH_M2	  (VIEWPORT_WIDTH * 2)
#define VIEWPORT_HEIGHT_M2	  (VIEWPORT_HEIGHT * 2)

#define VSCREEN_WIDTH	  (VIEWPORT_WIDTH * 3)
#define VSCREEN_HEIGHT	  (VIEWPORT_HEIGHT * 3)
#define VSCREEN_WIDTH_M1  (VSCREEN_WIDTH - 1)
#define VSCREEN_HEIGHT_M1 (VSCREEN_HEIGHT - 1)

#define IS_IN_BOUNDS_H(__x)	   ((__x >= 0) && (__x < VSCREEN_WIDTH))
#define IS_IN_BOUNDS_V(__y)	   ((__y >= 0) && (__y < VSCREEN_HEIGHT))
#define IS_IN_BOUNDS(__x, __y) (IS_IN_BOUNDS_H(__x) && IS_IN_BOUNDS_V(__y))

/* =============================================================== */
/* Rendering */

extern SDL_Window	*__window;
extern SDL_Renderer *__renderer;

/** Create a window and a renderer */
void Render_init(const char *WINDOW_TITLE, uint32_t WINDOW_WIDTH,
				 uint32_t WINDOW_HEIGHT);
#define Render_end                                                             \
	{                                                                          \
		SDL_DestroyRenderer(__renderer);                                       \
		SDL_DestroyWindow(__window);                                           \
		SDL_Quit();                                                            \
	}

void Render_Rescale(float scalex, float scaley);
void Render_SetPosition(int x, int y);
void Render_SetPositionAndScale(int x, int y, float scalex, float scaley);

/** Resize the viewport to the center, keeping the aspect ratio of the original
 * size */
void Render_ResizeWindow(int newWidth, int newHeight, bool keepAspectRatio);

#define Render_TogleFullscreen                                                 \
	SDL_SetWindowFullscreen(__window, SDL_GetWindowFlags(__window) ^           \
										  SDL_WINDOW_FULLSCREEN_DESKTOP)

/** Paint the screen screen */
#define Render_Update SDL_RenderPresent(__renderer)

/*================================================================== */

#define Render_SetcolorRGBA(r, g, b, a)                                        \
	SDL_SetRenderDrawColor(__renderer, r, g, b, a)
#define Render_Setcolor(c) Render_SetcolorRGBA(c.r, c.g, c.b, c.a)

#define Render_Clearscreen SDL_RenderClear(__renderer);
#define Render_Clearscreen_RGBA(r, g, b, a)                                    \
	{                                                                          \
		Render_SetcolorRGBA(r, g, b, a);                                       \
		Render_Clearscreen;                                                    \
	}
#define Render_Clearscreen_Color(c) Render_Clearscreen_RGBA(c.r, c.g, c.b, c.a)

#define Render_Pixel(x, y) SDL_RenderDrawPoint(__renderer, x, y);
#define Render_Pixel_RGBA(x, y, r, g, b, a)                                    \
	{                                                                          \
		Render_SetcolorRGBA(r, g, b, a);                                       \
		Render_Pixel(x, y);                                                    \
	}
#define Render_Pixel_Color(x, y, c) Render_Pixel_RGBA(x, y, c.r, c.g, c.b, c.a)

#define Render_image_ext(texture, x, y, w, h, angle, center, flip)             \
	SDL_RenderCopyEx(__renderer, texture, NULL, &(SDL_Rect){x, y, w, h},       \
					 angle, center, flip)

void Render_subimage_ext(SDL_Texture *texture, int image_x, int image_y, int w,
						 int h, int renderX, int renderY, const double angle,
						 const SDL_Point *center, const SDL_RendererFlip flip);

#define Render_subimage(texture, image_x, image_y, w, h, renderX, renderY)     \
	Render_subimage_ext(texture, image_x, image_y, w, h, renderX, renderY, 0,  \
						NULL, SDL_FLIP_NONE)

#endif // _GRAPHICS_H
