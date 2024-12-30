#include "assets.h"

SDL_Surface *optimize_surface(SDL_Surface *surface, SDL_Window *window) {
	/* Convert the given surface to the format of the window surface */
	SDL_Surface *optimized =
		SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);

	/* If the conversion failed, return the original surface */
	return (!optimized ? surface : optimized);
}

/* =============================================================== */
/* ===== IMG ===== */
Sprite *loadIMG(SDL_Surface *surface, SDL_Window *window,
				SDL_Renderer *render) {
	/* Optimize surface */
	if (window != NULL)
		surface = optimize_surface(surface, window);

	/* Create sprite */
	Sprite *spr	 = new (Sprite);
	spr->surface = surface;

	if (render != NULL)
		spr->texture = SDL_CreateTextureFromSurface(render, spr->surface);

	return spr;
}

Sprite *loadIMG_from_path(const char *imagePath, SDL_Window *window,
						  SDL_Renderer *render) {
	SDL_Surface *surface = IMG_Load(imagePath);
	if (!surface) {
		printf("Failed to load image: %s\n", IMG_GetError());
		return NULL;
	}

	return loadIMG(surface, window, render);
}

Sprite *loadIMG_from_mem(unsigned char res_image[], unsigned int res_image_len,
						 SDL_Window *window, SDL_Renderer *render) {
	SDL_Surface *surface = NULL;
	SDL_RWops	*rw		 = SDL_RWFromConstMem(res_image, res_image_len);
	surface = IMG_Load_RW(rw, 1); /* 1 for freeing the RWops after loading */

	return loadIMG(surface, window, render);
}
