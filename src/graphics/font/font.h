#ifndef _FONT_H
#define _FONT_H

#include <stdint.h>
#include <string.h>

#include "../../util.h"

#define BITFONT_CHAR_WIDTH	8
#define BITFONT_CHAR_HEIGHT 8
typedef uint8_t *BitFont;

extern BitFont __font_current;

/**
 * Sets the current font to be used for drawing characters.
 * @param font Pointer to the font bitmap.
 */
#define Font_SetCurrent(font) __font_current = font

/**
 * Calculates the width of a string in pixels when drawn using the current font.
 * @param str The string to calculate the width of.
 * @return The width of the string in pixels.
 */
#define FSTR_WIDTH(str) (strlen(str) * BITFONT_CHAR_WIDTH)

/**
 * Draws a character from a font bitmap at the specified position.
 *
 * @param font Pointer to the font bitmap.
 * @param c The ascii character to draw.
 * @param x The x-coordinate to draw on renderer.
 * @param y The y-coordinate to draw on renderer.
 */
void draw_char(unsigned char c, int x, int y);

/**
 * Draws a string using the specified font starting at the given position.
 *
 * @param font Pointer to the font bitmap.
 * @param str The string to draw.
 * @param x The x-coordinate to start drawing the string.
 * @param y The y-coordinate to start drawing the string.
 */
void draw_string(const char *str, int x, int y);

#endif // _FONT_H
