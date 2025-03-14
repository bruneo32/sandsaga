#include "graphics.h"

#include <stdio.h>

#include "../log/log.h"

SDL_Window	 *__window	 = NULL;
SDL_Renderer *__renderer = NULL;
SDL_Texture	 *__vscreen	 = NULL;

uint32_t __windowWidth	= 0;
uint32_t __windowHeight = 0;

static void Render_deinit() {
	if (__vscreen)
		SDL_DestroyTexture(__vscreen);

	if (__renderer)
		SDL_DestroyRenderer(__renderer);

	if (__window)
		SDL_DestroyWindow(__window);

	SDL_Quit();
}

void Render_init(const char *WINDOW_TITLE, uint32_t WINDOW_WIDTH,
				 uint32_t WINDOW_HEIGHT) {
	atexit(Render_deinit);

	/* Save data for calculations */
	__windowWidth  = WINDOW_WIDTH;
	__windowHeight = WINDOW_HEIGHT;

	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "0");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

#ifdef _WIN32
	/* In windows, opengl could be suboptimal, d3d11 is the best balance
	 * between performance and compatibility */
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#endif

	/* Initialize SDL2 */
	if (0 != SDL_Init(SDL_INIT_VIDEO)) {
		logerr("Error initializing SDL: %s", SDL_GetError());
		exit(-1);
	}

	/* Create the window */
	__window = SDL_CreateWindow(
		WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_RESIZABLE /*| SDL_WINDOW_FULLSCREEN_DESKTOP*/);

	if (!__window) {
		logerr("Error creating window: %s", SDL_GetError());
		exit(-2);
	}

	/* Create a 2D renderer */
	__renderer = SDL_CreateRenderer(__window, -1,
									SDL_RENDERER_ACCELERATED |
										SDL_RENDERER_TARGETTEXTURE |
										SDL_RENDERER_PRESENTVSYNC);
	if (!__renderer) {
		logerr("Error creating renderer: %s", SDL_GetError());
		exit(-3);
	}

	/* Create a texture for the screen */
	__vscreen = SDL_CreateTexture(__renderer, COLOR_PIXELFORMAT,
								  SDL_TEXTUREACCESS_STREAMING, VIEWPORT_WIDTH,
								  VIEWPORT_HEIGHT);
	SDL_SetTextureBlendMode(__vscreen, SDL_BLENDMODE_BLEND);

	SDL_SetRenderTarget(__renderer, NULL);
	SDL_SetRenderDrawBlendMode(__renderer, SDL_BLENDMODE_BLEND);
	Render_SetcolorRGBA(0xFF, 0xFF, 0xFF, 0x00);
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

void Render_Ellipse(int rx, int ry, int xc, int yc) {
	/* Extracted from
	 * https://www.geeksforgeeks.org/midpoint-ellipse-drawing-algorithm/ */
	float dx, dy, d1, d2, x, y;
	x = 0;
	y = ry;

	/* Initial decision parameter of region 1 */
	d1 = (ry * ry) - (rx * rx * ry) + (0.25 * rx * rx);
	dx = 2 * ry * ry * x;
	dy = 2 * rx * rx * y;

	/* For region 1 */
	while (dx < dy) {

		/* Print points based on 4-way symmetry */
		Render_Pixel(x + xc, y + yc);
		Render_Pixel(-x + xc, y + yc);
		Render_Pixel(x + xc, -y + yc);
		Render_Pixel(-x + xc, -y + yc);

		/* Checking and updating value of decision
		 * parameter based on algorithm */
		if (d1 < 0) {
			x++;
			dx = dx + (2 * ry * ry);
			d1 = d1 + dx + (ry * ry);
		} else {
			x++;
			y--;
			dx = dx + (2 * ry * ry);
			dy = dy - (2 * rx * rx);
			d1 = d1 + dx - dy + (ry * ry);
		}
	}

	/* Decision parameter of region 2 */
	d2 = ((ry * ry) * ((x + 0.5) * (x + 0.5))) +
		 ((rx * rx) * ((y - 1) * (y - 1))) - (rx * rx * ry * ry);

	/* Plotting points of region 2 */
	while (y >= 0) {

		/* Print points based on 4-way symmetry */
		Render_Pixel(x + xc, y + yc);
		Render_Pixel(-x + xc, y + yc);
		Render_Pixel(x + xc, -y + yc);
		Render_Pixel(-x + xc, -y + yc);

		/* Checking and updating parameter value based on algorithm */
		if (d2 > 0) {
			y--;
			dy = dy - (2 * rx * rx);
			d2 = d2 + (rx * rx) - dy;
		} else {
			y--;
			x++;
			dx = dx + (2 * ry * ry);
			dy = dy - (2 * rx * rx);
			d2 = d2 + dx - dy + (rx * rx);
		}
	}
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

Color Color_blend(Color src, Color dst) {
	/* Normalize alpha */
	float alpha		= src.a / 255.0f;
	float inv_alpha = 1.0f - alpha;

	Color out = {0};

	/* Blend each channel */
	out.r = (uint8_t)(src.r * alpha + dst.r * inv_alpha);
	out.g = (uint8_t)(src.g * alpha + dst.g * inv_alpha);
	out.b = (uint8_t)(src.b * alpha + dst.b * inv_alpha);
	out.a = src.a + (dst.a * inv_alpha);

	return out;
}
