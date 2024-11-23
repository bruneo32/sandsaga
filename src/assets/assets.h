#ifndef _ASSETS_H
#define _ASSETS_H

#include <SDL.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <stdio.h>

#include "../util.h"

typedef struct _Sprite {
	SDL_Texture *texture;
	SDL_Surface *surface;
} Sprite;

/**
 * Optimize the given surface to match the window's format (more speedy).
 * If the conversion fails, the original surface is returned.
 *
 * @param surface The surface to optimize
 * @param window The window to use for conversion
 * @return The optimized surface, or the original surface if the
 * conversion failed
 */
SDL_Surface *optimize_surface(SDL_Surface *surface, SDL_Window *window);

/* =============================================================== */
/* ===== IMG ===== */
Sprite *loadIMG(SDL_Surface *surface, SDL_Window *window, SDL_Renderer *render);
Sprite *loadIMG_from_path(const char *imagePath, SDL_Window *window,
						  SDL_Renderer *render);
Sprite *loadIMG_from_mem(unsigned char res_image[], unsigned int res_image_len,
						 SDL_Window *window, SDL_Renderer *render);

#endif // _ASSETS_H
