#ifndef _ENTITIES_H
#define _ENTITIES_H

#include <stdbool.h>

#include "../assets/assets.h"
#include "engine.h"

typedef struct _Player {
	short	x;
	short	y;
	Chunk	chunk_id;
	Sprite *sprite;
	bool	fliph;
} Player;

#endif // _ENTITIES_H
