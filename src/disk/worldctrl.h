#ifndef _WORLDCTRL_H
#define _WORLDCTRL_H

/*
 * ==== WorldFile doc ====
 * World is divided in chunks of 384x384 bytes (144K)
 * World is 65536x256 = 16777216 CHUNKS of 144K each!
 * This means that if the whole world is written to the disk it would take
 * more than 2T of space! Without taking into account file metadata.
 *
 * So the workaround here is to divide the game in two stages:
 *   1. A data file containing unordered chunks raw bytes.
 *   2. A control file telling which chunks are already written on the disk and
 *      where, this file also contains world seed and other world specific data.
 *
 * This way, we ensure that only the chunks that are needed are written to the
 * disk unordered so they don't bloat offset with zeroes. The control tells us
 * the order of the data file.
 */

#include <stdint.h>

#include "../engine/engine.h"
#include "../engine/gameobjects.h"
#include "../util.h"

#define SAVEFILE_VERSION (1)

/** World Control Chunk Addresser */
#define INVALID_CATABLE ((catable_t)~0)
#define CATABLE_SIZE (CHUNK_MAX_X * CHUNK_MAX_Y)
typedef uint32_t catable_t;

#pragma pack(push, 1)
typedef struct {
	byte	  version;
	seed_t	  seed;
	catable_t catable[CATABLE_SIZE];
} PACKED WorldControl;
#pragma pack(pop)

extern int world_control_fd;
extern int world_data_fd;

extern WorldControl *world_control;

void world_control_init(WorldControl *world_control);
void save_chunk_to_disk(Chunk chunk, GO_ID *chunk_data);
int	 load_chunk_from_disk(Chunk chunk, void *chunk_data);

#endif // _WORLDCTRL_H
