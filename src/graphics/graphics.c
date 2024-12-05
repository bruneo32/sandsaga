#include "graphics.h"

SDL_Window	 *__window	 = NULL;
SDL_Renderer *__renderer = NULL;
SDL_Texture	 *vscreen_	 = NULL;

uint32_t __windowWidth	= 0;
uint32_t __windowHeight = 0;

void Render_init(const char *WINDOW_TITLE, uint32_t WINDOW_WIDTH,
				 uint32_t WINDOW_HEIGHT) {
	/* Save data for calculations */
	__windowWidth  = WINDOW_WIDTH;
	__windowHeight = WINDOW_HEIGHT;

	/* Initialize SDL2 */
	if (0 != SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
		exit(-1);
	}

	/* Create the window */
	__window = SDL_CreateWindow(
		WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_RESIZABLE /*| SDL_WINDOW_FULLSCREEN_DESKTOP*/);

	if (!__window) {
		fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
		SDL_Quit();
		exit(-2);
	}

	/* Create a 2D renderer */
	__renderer = SDL_CreateRenderer(__window, -1,
									SDL_RENDERER_ACCELERATED |
										SDL_RENDERER_TARGETTEXTURE |
										SDL_RENDERER_PRESENTVSYNC);
	if (!__renderer) {
		fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(__window);
		SDL_Quit();
		exit(-3);
	}
	SDL_SetRenderDrawBlendMode(__renderer, SDL_BLENDMODE_BLEND);

	vscreen_ = SDL_CreateTexture(__renderer, SDL_PIXELFORMAT_RGBA8888,
								 SDL_TEXTUREACCESS_TARGET, VSCREEN_WIDTH,
								 VSCREEN_HEIGHT);

	SDL_SetTextureBlendMode(vscreen_, SDL_BLENDMODE_BLEND);
	Render_SetcolorRGBA(0xFF, 0xFF, 0xFF, 0);
	SDL_RenderClear(__renderer);
}

void Render_Rescale(float scalex, float scaley) {
	/* Save user-defined render target */
	SDL_Texture *__rtex = SDL_GetRenderTarget(__renderer);

	/* Set up the rendering to use the new dimensions */
	SDL_RenderSetScale(__renderer, scalex, scaley);

	if (!__rtex)
		return;

	/* Repeat for (screen) */
	SDL_SetRenderTarget(__renderer, NULL);
	SDL_RenderSetScale(__renderer, scalex, scaley);

	/* Reset render target */
	SDL_SetRenderTarget(__renderer, __rtex);
}

void Render_SetPosition(int x, int y) {
	/* Save user-defined render target */
	SDL_Texture *__rtex = SDL_GetRenderTarget(__renderer);
	// SDL_Texture *__rtex = NULL;

	SDL_Rect viewport = {x, y, __windowWidth, __windowHeight};
	SDL_RenderSetViewport(__renderer, &viewport);

	if (!__rtex)
		return;

	/* Repeat for (screen) */
	SDL_SetRenderTarget(__renderer, NULL);
	SDL_RenderSetViewport(__renderer, &viewport);

	/* Reset render target */
	SDL_SetRenderTarget(__renderer, __rtex);
}

void Render_SetPositionAndScale(int x, int y, float scalex, float scaley) {
	/* Save user-defined render target */
	SDL_Texture *__rtex = SDL_GetRenderTarget(__renderer);

	SDL_Rect viewport = {x, y, __windowWidth, __windowHeight};

	SDL_RenderSetScale(__renderer, scalex, scaley);
	SDL_RenderSetViewport(__renderer, &viewport);

	if (!__rtex)
		return;

	/* Repeat for (screen) */
	SDL_SetRenderTarget(__renderer, NULL);

	SDL_RenderSetScale(__renderer, scalex, scaley);
	SDL_RenderSetViewport(__renderer, &viewport);

	/* Reset render target */
	SDL_SetRenderTarget(__renderer, __rtex);
}

void Render_ResizeWindow(int newWidth, int newHeight, bool keepAspectRatio) {
	float scaleX = ((float)newWidth / (float)__windowWidth);
	float scaleY = ((float)newHeight / (float)__windowHeight);

	float scale = 0.0;
	if (keepAspectRatio)
		scale = fminf(scaleX, scaleY);

	/* Calculate the new scaled dimensions */
	int scaledWidth =
		(int)((float)__windowWidth * (keepAspectRatio ? scale : scaleX));
	int scaledHeight =
		(int)((float)__windowHeight * (keepAspectRatio ? scale : scaleY));

	/* Calculate centered positioning offset */
	int offsetX =
		(newWidth - scaledWidth) / (keepAspectRatio ? scale : scaleX) / 2;
	int offsetY =
		(newHeight - scaledHeight) / (keepAspectRatio ? scale : scaleY) / 2;

	Render_SetPositionAndScale(offsetX, offsetY,
							   (keepAspectRatio ? scale : scaleX),
							   (keepAspectRatio ? scale : scaleY));
}

void Render_subimage_ext(SDL_Texture *texture, int image_x, int image_y, int w,
						 int h, int renderX, int renderY, const double angle,
						 const SDL_Point *center, const SDL_RendererFlip flip) {
	/* Define source rectangle (sub-tile area in the tileset) */
	SDL_Rect srcRect = {image_x, image_y, w,
						h}; /*  Adjust TILE_SIZE if needed */

	/* Define destination rectangle (where to render the sub-tile on the screen)
	 */
	SDL_Rect destRect = {renderX, renderY, w,
						 h}; /*  Adjust position as needed */

	/* Render the sub-tile from the tileset to the screen */
	SDL_RenderCopyEx(__renderer, texture, &srcRect, &destRect, angle, center,
					 flip);
}
