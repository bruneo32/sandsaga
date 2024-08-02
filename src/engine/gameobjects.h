#ifndef _GAMEOBJECTS_H
#define _GAMEOBJECTS_H

#include "../graphics/color.h"


enum GameObject_t /* : uint8_t */ {
	GO_NONE = 0,

	/* Fluids */
	GO_WATER,
	GO_SAND,

	/* Solids */
	GO_STONE,

	/* Enum defs */
	GO_FIRST = GO_WATER,
	GO_LAST	 = GO_STONE,

	GO_LAST_FLUID = GO_SAND,
	GO_LAST_SOLID = GO_STONE,
};

#define GO_IS_FLUID(go) ((go) > GO_NONE && (go) <= GO_LAST_FLUID)
#define GO_IS_SOLID(go) ((go) > GO_LAST_FLUID && (go) <= GO_LAST_SOLID)

static const Color GO_COLORS[] = {
	/* None (transparent) */
	{0xFF, 0xFF, 0xFF, 0},
	/* Water */
	{0x7F, 0x7F, 0xFF, 0xAF},
	/* Sand */
	{0xFF, 0xFF, 0x7F, 0xFF},
	/* Stone */
	{0x7F, 0x7F, 0x7F, 0xFF},
};

#define BITMASK_GO_UPDATED (0b10000000)
#define GUPDATE(go)		   ((go) | BITMASK_GO_UPDATED)
#define GOBJECT(go)		   ((go) & ~BITMASK_GO_UPDATED)
#define IS_GUPDATED(go)	   (((go) & BITMASK_GO_UPDATED) != 0)

#endif // _GAMEOBJECTS_H
