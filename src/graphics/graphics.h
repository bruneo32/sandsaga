#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL.h>
#include <math.h>
#include <stdbool.h>

#include "../util.h"
#include "color.h"

#define FPS			   60
#define FPS_DELTA	   (1.0f / FPS)
#define FRAME_DELAY_MS (1000 / FPS)
#define CALCULATE_FPS(delta)                                                   \
	(clamp(((int)(1.f / ((float)delta / 1000.f))), 0, FPS))

#define VIEWPORT_WIDTH		  384
#define VIEWPORT_HEIGHT		  216
#define VIEWPORT_WIDTH_DIV_2  (VIEWPORT_WIDTH / 2)
#define VIEWPORT_HEIGHT_DIV_2 (VIEWPORT_HEIGHT / 2)
#define VIEWPORT_WIDTH_M2	  (VIEWPORT_WIDTH * 2)
#define VIEWPORT_HEIGHT_M2	  (VIEWPORT_HEIGHT * 2)

#define CHUNK_SIZE		  VIEWPORT_WIDTH
#define CHUNK_SIZE_M2	  VIEWPORT_WIDTH_M2
#define CHUNK_SIZE_DIV_2  VIEWPORT_WIDTH_DIV_2
#define VSCREEN_WIDTH	  (CHUNK_SIZE * 3)
#define VSCREEN_HEIGHT	  (CHUNK_SIZE * 3)
#define VSCREEN_WIDTH_M1  (VSCREEN_WIDTH - 1)
#define VSCREEN_HEIGHT_M1 (VSCREEN_HEIGHT - 1)

#define IS_IN_BOUNDS_H(__x)	   ((__x >= 0) && (__x < VSCREEN_WIDTH))
#define IS_IN_BOUNDS_V(__y)	   ((__y >= 0) && (__y < VSCREEN_HEIGHT))
#define IS_IN_BOUNDS(__x, __y) (IS_IN_BOUNDS_H(__x) && IS_IN_BOUNDS_V(__y))

/* =============================================================== */
/* Rendering */

extern SDL_Window	*__window;
extern SDL_Renderer *__renderer;
extern SDL_Texture	*__vscreen;

/** Create a window and a renderer */
void Render_init(const char *WINDOW_TITLE, uint32_t WINDOW_WIDTH,
				 uint32_t WINDOW_HEIGHT);

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

#define Render_Line(x1, y1, x2, y2)                                            \
	SDL_RenderDrawLine(__renderer, x1, y1, x2, y2);
#define Render_Line_RGBA(x1, y1, x2, y2, r, g, b, a)                           \
	{                                                                          \
		Render_SetcolorRGBA(r, g, b, a);                                       \
		Render_Line(x1, y1, x2, y2);                                           \
	}
#define Render_Line_Color(x1, y1, x2, y2, c)                                   \
	Render_Line_RGBA(x1, y1, x2, y2, c.r, c.g, c.b, c.a)

#define Render_Rect(x_, y_, w_, h_)                                            \
	SDL_RenderDrawRect(                                                        \
		__renderer, &(SDL_Rect){.x = (x_), .y = (y_), .w = (w_), .h = (h_)});
#define Render_FillRect(x_, y_, w_, h_)                                        \
	SDL_RenderFillRect(                                                        \
		__renderer, &(SDL_Rect){.x = (x_), .y = (y_), .w = (w_), .h = (h_)});

/**
 * Draw an ellipse.
 * @param rx horizontal radius
 * @param ry vertical radius
 * @param xc x center
 * @param yc y center
 */
void Render_Ellipse(int rx, int ry, int xc, int yc);
#define Render_Ellipse_RGBA(rx, ry, xc, yc, r, g, b, a)                        \
	{                                                                          \
		Render_SetcolorRGBA(r, g, b, a);                                       \
		Render_Ellipse(rx, ry, xc, yc);                                        \
	}
#define Render_Ellipse_Color(rx, ry, xc, yc, c)                                \
	Render_Ellipse_RGBA(rx, ry, xc, yc, c.r, c.g, c.b, c.a)

#define Render_Circle(r, x, y) Render_Ellipse(r, r, x, y)
#define Render_Circle_RGBA(r, x, y, red, g, b, a)                              \
	Render_Ellipse_RGBA(r, r, x, y, red, g, b, a)
#define Render_Circle_Color(r, x, y, c) Render_Ellipse_Color(r, r, x, y, c)

#define Render_image_ext(texture, x, y, w, h, angle, center, flip)             \
	SDL_RenderCopyEx(__renderer, texture, NULL, &(SDL_Rect){x, y, w, h},       \
					 angle, center, flip)
#define Render_image_extF(texture, x, y, w, h, angle, center, flip)            \
	SDL_RenderCopyExF(__renderer, texture, NULL, &(SDL_FRect){x, y, w, h},     \
					  angle, center, flip)

void Render_subimage_ext(SDL_Texture *texture, int image_x, int image_y, int w,
						 int h, int renderX, int renderY, const double angle,
						 const SDL_Point *center, const SDL_RendererFlip flip);

#define Render_subimage(texture, image_x, image_y, w, h, renderX, renderY)     \
	Render_subimage_ext(texture, image_x, image_y, w, h, renderX, renderY, 0,  \
						NULL, SDL_FLIP_NONE)

#ifdef __cplusplus
}
#endif

#endif // _GRAPHICS_H
