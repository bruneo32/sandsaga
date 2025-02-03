#ifndef _DISK_RESOURCES_H
#define _DISK_RESOURCES_H

#include "../assets/assets.h"
#include "../engine/entities.h"

#include "disk.h"

extern Sprite *default_player_sprite;

void load_player_sprite(Player *player, unsigned char default_sprite_data[],
						unsigned int default_sprite_data_len);

#endif // _DISK_RESOURCES_H
