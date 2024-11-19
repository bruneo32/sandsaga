#ifndef _ENTITIES_H
#define _ENTITIES_H

#include <stdbool.h>

#include "../assets/assets.h"
#include "engine.h"

#define GRAVITY				1
#define PLAYER_SPEED_FLYING 4
#define PLAYER_SPEED		2
#define PLAYER_VSPEED_JUMP	9
#define PLAYER_VSPEED_MAX	6

typedef struct _Player {
	short	x;
	short	y;
	short	width;
	short	height;
	short	vspeed;
	Chunk	chunk_id;
	Sprite *sprite;
	bool	fliph;
	bool	flying;
} Player;

void move_player(Player *player, SDL_Rect *camera, const Uint8 *keyboard);

#endif // _ENTITIES_H
