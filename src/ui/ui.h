#ifndef _UI_H
#define _UI_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../graphics/font/font.h"
#include "../graphics/graphics.h"
#include "../util.h"

typedef enum {
	UI_ALIGN_LEFT,
	UI_ALIGN_CENTER,
	UI_ALIGN_RIGHT,
} UIAlignment;

enum UIButtonState {
	UI_BUTTON_NORMAL  = 1,
	UI_BUTTON_HOVER	  = 1 << 1,
	UI_BUTTON_PRESSED = 1 << 2,
};

typedef struct _UIButton {
	/* Properties */
	const char *text;
	UIAlignment alignment;
	/* Private, don't touch */
	Point	m_center;
	Rect	m_rect;
	uint8_t state;
	/* Actions */
	void (*onClick)(void);
} UIButton;

#define MAX_BUTTONS 32
typedef struct _UICanvas {
	size_t	  button_count;
	UIButton *button_list[MAX_BUTTONS];
} UICanvas;

void ui_set_cursor(SDL_SystemCursor id);

UIButton *UIButton_new(const char *text, int x, int y, UIAlignment alignment);

void canvas_process(const UICanvas *canvas, const uint32_t mouse_buttons,
					const int mouse_x, const int mouse_y);

void canvas_draw(const UICanvas *canvas);
void canvas_delete(const UICanvas *canvas);

#endif // _UI_H
