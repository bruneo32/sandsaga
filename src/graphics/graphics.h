#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <SDL.h>
#include <math.h>
#include <stdbool.h>

#include "../assets/assets.h"
#include "color.h"

extern SDL_Window	*__window;
extern SDL_Renderer *__renderer;

/** Create a window and a renderer */
void Render_init(const char *WINDOW_TITLE, int WINDOW_WIDTH, int WINDOW_HEIGHT,
				 int RENDER_WIDTH, int RENDER_HEIGHT);
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
void Render_ResizeWindow(int newWidth, int newHeight, SDL_Rect *viewport,
						 bool keepAspectRatio);

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
	SDL_RenderCopyExF(__renderer, texture, NULL, &(SDL_FRect){x, y, w, h},     \
					  angle, center, flip)

#endif // _GRAPHICS_H
