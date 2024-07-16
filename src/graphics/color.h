#ifndef _COLOR_H
#define _COLOR_H

#include <stdint.h>
#include "../util.h"

typedef struct _Color {
	uint8_t r; /* Red */
	uint8_t g; /* Green */
	uint8_t b; /* Blue */
	uint8_t a; /* Alpha */
} PACKED Color;

static const Color C_BLACK	= {0, 0, 0, 0xFF};
static const Color C_WHITE	= {0xFF, 0xFF, 0xFF, 0xFF};
static const Color C_GRAY	= {0x7F, 0x7F, 0x7F, 0xFF};
static const Color C_DKGRAY = {0x11, 0x11, 0x11, 0xFF};
static const Color C_LTGRAY = {0xED, 0xED, 0xED, 0xFF};
static const Color C_RED	= {0xFF, 0, 0, 0xFF};
static const Color C_GREEN	= {0, 0xFF, 0, 0xFF};
static const Color C_BLUE	= {0, 0, 0xFF, 0xFF};

#define C_ISBLACK(cs) (!cs.red && !cs.green && !cs.blue)

#endif // _COLOR_H
