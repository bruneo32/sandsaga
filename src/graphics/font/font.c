#include "font.h"

#include "../graphics.h"

BitFont __font_current;

void draw_char(unsigned char c, int x, int y) {
	/* Get the address of the character in the font bitmap. */
	BitFont charat = &__font_current[c * BITFONT_CHAR_HEIGHT];

	for (int j = 0; j < BITFONT_CHAR_HEIGHT; ++j) {
		for (int i = 0; i < BITFONT_CHAR_WIDTH; ++i) {
			/* If the bit at the current position is set, draw a pixel,
			 * the color is driven by Render_Setcolor() */
			if ((*charat & (1 << (7 - i))) != 0)
				Render_Pixel(x + i, y + j);
		}

		/* Move to the next row of the character. */
		charat++;
	}
}

void draw_string(const char *str, int x, int y) {
	const int start_x = x;

	while (*str != 0) {
		if (*str == '\n') {
			x = start_x;
			y += BITFONT_CHAR_HEIGHT;
			++str;
			continue;
		}

		draw_char(*str++, x, y);
		x += BITFONT_CHAR_WIDTH;
	}
}
