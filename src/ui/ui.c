#include "ui.h"

static SDL_Cursor *m_sys_cursors[] = {
	NULL, // SDL_SYSTEM_CURSOR_ARROW,
	NULL, // SDL_SYSTEM_CURSOR_IBEAM,
	NULL, // SDL_SYSTEM_CURSOR_WAIT,
	NULL, // SDL_SYSTEM_CURSOR_CROSSHAIR,
	NULL, // SDL_SYSTEM_CURSOR_WAITARROW,
	NULL, // SDL_SYSTEM_CURSOR_SIZENWSE,
	NULL, // SDL_SYSTEM_CURSOR_SIZENESW,
	NULL, // SDL_SYSTEM_CURSOR_SIZEWE,
	NULL, // SDL_SYSTEM_CURSOR_SIZENS,
	NULL, // SDL_SYSTEM_CURSOR_SIZEALL,
	NULL, // SDL_SYSTEM_CURSOR_NO,
	NULL, // SDL_SYSTEM_CURSOR_HAND,
	NULL, // SDL_NUM_SYSTEM_CURSORS
};

void ui_set_cursor(SDL_SystemCursor id) {
	const SDL_Cursor *cursor = SDL_GetCursor();

	/* If the cursor is already set, do nothing */
	if (cursor == m_sys_cursors[id])
		return;

	/* Initialize the first time */
	if (m_sys_cursors[id] == NULL)
		m_sys_cursors[id] = SDL_CreateSystemCursor(id);

	SDL_SetCursor(m_sys_cursors[id]);
}

UIButton *UIButton_new(const char *text, int x, int y, UIAlignment alignment) {
	UIButton *button   = new (UIButton);
	button->text	   = text;
	button->alignment  = alignment;
	button->m_center.x = x;
	button->m_center.y = y;

	button->state = 0;

	/* Calculate rect */
	button->m_rect = (Rect){x, y, FSTR_WIDTH(text), BITFONT_CHAR_HEIGHT};

	if (alignment == UI_ALIGN_CENTER)
		button->m_rect.x -= button->m_rect.w / 2;
	else if (alignment == UI_ALIGN_RIGHT)
		button->m_rect.x -= button->m_rect.w;

	return button;
}

void canvas_process(const UICanvas *canvas, const uint32_t mouse_buttons,
					const int mouse_x, const int mouse_y) {
	if (!canvas || canvas->button_count == 0)
		return;

	for (size_t i = 0; i < canvas->button_count; ++i) {
		UIButton *button = canvas->button_list[i];
		if (!button || !button->onClick)
			continue;

		const bool is_hovered = mouse_x >= button->m_rect.x &&
								mouse_x < button->m_rect.x + button->m_rect.w &&
								mouse_y >= button->m_rect.y &&
								mouse_y < button->m_rect.y + button->m_rect.h;
		if (is_hovered) {
			button->state |= UI_BUTTON_HOVER;

			/* Button hover */
			ui_set_cursor(SDL_SYSTEM_CURSOR_HAND);

			/* Button clicked */
			if (mouse_buttons & (SDL_BUTTON(SDL_BUTTON_LEFT)))
				button->onClick();

			/* Stop propagating */
			break;
		} else {
			/* Reset button state */
			button->state = UI_BUTTON_NORMAL;
			ui_set_cursor(0); /* Default cursor */
		}
	}
}

void canvas_draw(const UICanvas *canvas) {
	/* Draw buttons */
	for (size_t bi = 0; bi < canvas->button_count; ++bi) {
		UIButton *button = canvas->button_list[bi];

		Color foreground = C_WHITE;
		if (button->state == UI_BUTTON_NORMAL)
			foreground.a = 0x99;

		Render_Setcolor(foreground);
		draw_string(button->text, button->m_rect.x, button->m_rect.y);
	}
}

void canvas_delete(const UICanvas *canvas) {
	if (!canvas)
		return;

	for (size_t i = 0; i < canvas->button_count; ++i) {
		UIButton *btn = canvas->button_list[i];
		if (btn)
			free(btn);
	}
}
