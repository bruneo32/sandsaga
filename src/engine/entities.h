#ifndef _ENTITIES_H
#define _ENTITIES_H

#include <stdbool.h>

#include "../physics/physics.h"

#include "../assets/assets.h"
#include "engine.h"

#define PLAYER_FLYING_SPEED 16
#define PLAYER_SPEED		6
#define PLAYER_VSPEED_JUMP	-9
#define PLAYER_VSPEED_MAX	48

typedef struct _Player {
	b2Body *body;
	float	x;
	float	y;
	float	prev_x;
	float	prev_y;
	Chunk	chunk_id;
	bool	fliph;
	bool	flying;
	short	width;
	short	height;
	Sprite *sprite;
} Player;

void move_player(Player *player, const Uint8 *keyboard);
void move_camera(Player *player, SDL_Rect *camera);

#endif // _ENTITIES_H
