#ifndef _GAMEOBJECTS_H
#define _GAMEOBJECTS_H

#include "../graphics/color.h"

#include <stdbool.h>

typedef enum GO_Type {
	GO_STATIC = 0,
	GO_POWDER,
	GO_LIQUID,
	GO_GAS,
} GO_Type;
#define GO_Type uint8_t

#define GO_IS_FLUID(gtype_) ((gtype_) >= GO_POWDER)

typedef enum GO_Flags {
	GO_FLAG_NONE	  = 0,
	GO_FLAG_FLAMMABLE = 1,
} GO_Flags;
#define GO_Flags uint8_t

typedef void (*GO_Draw)(size_t wx, size_t wy, int vx, int vy);
typedef bool (*GO_Update)(size_t x, size_t y, const bool ltr);

#pragma pack(push, 1)
typedef union GO_ID {
	uint8_t raw;
	struct {
		bit id		: 7;
		bit updated : 1;
	} PACKED;
} GO_ID;

typedef struct GameObject {
	GO_Type	  type;
	float	  density;
	GO_Flags  flags;
	Color	  color;
	GO_Draw	  draw;
	GO_Update update;
} PACKED GameObject;
#pragma pack(pop)

#define MAX_GO_ID (0b01111111)

extern size_t	  go_table_size;
extern GameObject go_table[MAX_GO_ID];

extern GO_ID GO_VAPOR;
extern GO_ID GO_WATER;
extern GO_ID GO_SAND;
extern GO_ID GO_STONE;
extern GO_ID GO_DIRT;
extern GO_ID GO_OIL;
extern GO_ID GO_ACID;
extern GO_ID GO_SMOKE;
extern GO_ID GO_FIRE;

#define GO_NONE		 ((GO_ID){.raw = 0})
#define GO_FIRST	 ((GO_ID){.raw = 1})
#define GO_LAST		 ((GO_ID){.raw = go_table_size})
#define GOBJECT(_id) (go_table[(_id).id - 1])

/**
 * \brief Register a new gameobject in the game
 *
 * \param type The type of the gameobject
 * \param density The density of the gameobject
 * \param color The color of the gameobject
 * \param draw The draw function of the gameobject
 *
 * \return The id of the gameobject [1-127]. Zero is reserved for GO_NONE
 */
GO_ID register_gameobject(GO_Type type, float density, GO_Flags flags,
						  Color color, GO_Draw draw, GO_Update update);

void init_gameobjects();

#endif // _GAMEOBJECTS_H
