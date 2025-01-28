#include "worldctrl.h"

#include "disk.h"

#include "../log/log.h"

int world_control_fd = -1;
int world_data_fd	 = -1;

WorldControl *world_control = NULL;

static catable_t m_next_off_chunk = 0;

void world_control_init(WorldControl *world_control) {
	/* Set m_next_off_chunk to the first free chunk.
	 * IE: the max value in the catable, plus one. */
	for (size_t i = 0; i < CATABLE_SIZE; i++) {
		const catable_t x = world_control->catable[i];
		if (x != INVALID_CATABLE && x >= m_next_off_chunk)
			m_next_off_chunk = x + 1;
	}
}

void save_chunk_to_disk(Chunk chunk_id, GO_ID *chunk_data) {
	/* Check if OS have enough space to work */
	const size_t free_space = check_disk_space(user_path);
	if (free_space < _1G) {
		logerr("save_chunk_to_disk: Disk small or running out of space "
			   "(<1GB).\nFree space: %zu MB\n",
			   free_space / _1M);
		exit(1);
	}

	catable_t chunk_file_idx = world_control->catable[CHUNK_ID(chunk_id)];

	/* Create entry if it doesn't exist, and update m_next_off_chunk */
	if (chunk_file_idx == INVALID_CATABLE) {
		chunk_file_idx = m_next_off_chunk++;
		/* Update catable */
		world_control->catable[CHUNK_ID(chunk_id)] = chunk_file_idx;
	}

	/* Save chunk to disk */
	off_t file_offset = chunk_file_idx * CHUNK_MEMSIZE;

	if (lseek(world_data_fd, file_offset, SEEK_SET) < 0) {
		logerr("save_chunk_to_disk: lseek failed: %lu", errno);
		return;
	}

	if (write(world_data_fd, chunk_data, CHUNK_MEMSIZE) != CHUNK_MEMSIZE) {
		logerr("save_chunk_to_disk: write failed: %lu", errno);
	}
}

int load_chunk_from_disk(Chunk chunk_id, void *chunk_data) {
	catable_t chunk_file_idx = world_control->catable[CHUNK_ID(chunk_id)];
	if (chunk_file_idx == INVALID_CATABLE)
		return 0; /* Chunk not stored in disk */

	off_t file_offset = chunk_file_idx * CHUNK_MEMSIZE;

	if (lseek(world_data_fd, file_offset, SEEK_SET) < 0) {
		logerr("load_chunk_from_disk: lseek failed: %lu", errno);
		return 0;
	}

	if (read(world_data_fd, chunk_data, CHUNK_MEMSIZE) != CHUNK_MEMSIZE) {
		logerr("load_chunk_from_disk: read failed: %lu", errno);
		return 0;
	}

	return 1;
}
