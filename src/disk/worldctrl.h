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
 *   1. 16384 Region files holding 1024 Chunks each file (32x32 chunks, 144MB
 *      file). In effect, manually sparsing the file instead of relying on
 *      filesystem sparse files.
 *   2. A control file telling which chunks are already written on the disk
 *      (bitarray), this file also contains world seed and other world specific
 * data.
 *
 * This way, we ensure that if a player stays on the same 32x32 region, it will
 * not pass 144MB of disk space. Even if he runs a lot of chunks it would only
 * take a bunch of regions, instead of a dangerous sparse file that could blow
 * up a PC if it is misconfigured or unsupported.
 */

#include <stdint.h>

#include "../engine/engine.h"
#include "../util.h"

/** World Control Chunk Addresser */
#define CATABLE_SIZE ((CHUNK_MAX_X + 1) * (CHUNK_MAX_Y + 1) / 8)

#define SAVEFILE_VERSION (1)

typedef struct {
	byte   version;
	seed_t seed;
	byte   catable[CATABLE_SIZE];
} PACKED WorldControl;

#endif // _WORLDCTRL_H
