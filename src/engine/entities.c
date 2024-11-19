#include "entities.h"

void move_player(Player *player, SDL_Rect *camera, const Uint8 *keyboard) {

	const short player_sx = player->x;
	const short player_sy = player->y;

	const short player_height_2 = player->height / 2;
	const short player_width_4	= player->width / 4;

#define player_top	  (player->y - player_height_2 + 1)
#define player_bottom (player->y + player_height_2 - 1)
#define player_left	  (player->x - player_width_4 + 1)
#define player_right  (player->x + player_width_4 - 1)

	/* Add gravity */
	if (!player->flying) {
		if (player->vspeed < PLAYER_VSPEED_MAX)
			player->vspeed += GRAVITY;

		const size_t vspeed_sign = player->vspeed > 0 ? 1 : -1;
		const size_t vspeed_abs	 = abs(player->vspeed);

		const size_t forward = (!player->fliph) ? 1 : -1;

		for (size_t i = 0; i < vspeed_abs; ++i) {
			/* This is player_bottom based on vspeed_sign */
			if (gameboard[(player->y + (vspeed_sign * player_height_2))]
						 [player->x + (-forward * (player_width_4 - 1))] !=
				GO_NONE)
				break;
			player->y += vspeed_sign;
		}

		player->y = clamp(player->y, 0, VSCREEN_HEIGHT);
	}

	if (keyboard[SDL_SCANCODE_SPACE]) {
		player->flying = !player->flying;
	}

	if (keyboard[SDL_SCANCODE_W]) {
		if (player->flying)
			player->y =
				clamp(player->y - PLAYER_SPEED_FLYING, 0, VSCREEN_HEIGHT);
		else if (gameboard[player_bottom + 1][player->x] != GO_NONE)
			player->vspeed = -PLAYER_VSPEED_JUMP;
	} else if (keyboard[SDL_SCANCODE_S]) {
		if (player->flying)
			player->y =
				clamp(player->y + PLAYER_SPEED_FLYING, 0, VSCREEN_HEIGHT);
	}

	if (keyboard[SDL_SCANCODE_A]) {
		player->fliph = true;
		if (player->flying)
			player->x -= PLAYER_SPEED_FLYING;
		else
			for (size_t i = 0; i < PLAYER_SPEED; ++i) {
				if (gameboard[player_bottom][player_left - 1] != GO_NONE) {
					/* Climb slope */
					bool climbed = false;
					for (size_t j = 0; j < player->height / 4; ++j) {
						if (gameboard[player_bottom - j][player_left - 1] ==
							GO_NONE) {
							player->y -= j;
							climbed = true;
							break;
						}
					}
					if (!climbed)
						break;
				}
				--player->x;
			}
		player->x = clamp(player->x, 0, VSCREEN_WIDTH);
	} else if (keyboard[SDL_SCANCODE_D]) {
		player->fliph = false;
		if (player->flying)
			player->x += PLAYER_SPEED_FLYING;
		else {
			for (size_t i = 0; i < PLAYER_SPEED; ++i) {
				if (gameboard[player_bottom][player_right + 1] != GO_NONE) {
					/* Climb slope */
					bool climbed = false;
					for (size_t j = 0; j < player->height / 4; ++j) {
						if (gameboard[player_bottom - j][player_right + 1] ==
							GO_NONE) {
							player->y -= j;
							climbed = true;
							break;
						}
					}
					if (!climbed)
						break;
				}
				++player->x;
			}
		}
		player->x = clamp(player->x, 0, VSCREEN_WIDTH);
	}

	/* Generate new chunks and move camera */

	if (player->y < player_sy) {
		/* Apply verifications for moving UP */

		/* If went to top chunk.
		 * Move world to bottom and generate new chunks in top
		 */
		if (player->chunk_id.y > 1 && player->y < VIEWPORT_HEIGHT) {
			player->y = VIEWPORT_HEIGHT * 2 - (VIEWPORT_HEIGHT - player->y);
			--player->chunk_id.y;

			/* Move world to bottom */
			memmove(&gameboard[VIEWPORT_HEIGHT][0], &gameboard[0][0],
					VIEWPORT_HEIGHT * 2 * VSCREEN_WIDTH);

			/* Generate new world at top */
			chunk_axis_t start_x = player->chunk_id.x - 1;
			for (uint_fast8_t i = 0; i < 3; ++i) {
				Chunk chunk = {.x = start_x + i, .y = player->chunk_id.y - 1};
				generate_chunk(WORLD_SEED, chunk, i * VIEWPORT_WIDTH, 0);
			}
			ResetSubchunks;
		}

		const int start_cam_y = camera->y;
		camera->y			  = clamp(player->y - VIEWPORT_HEIGHT_DIV_2, 0,
									  VSCREEN_HEIGHT - VIEWPORT_HEIGHT);
		const int end_cam_y	  = camera->y;

		/* Activate the next to subchunk on the top to draw it
		 * when entered the camera rect */
		for (uint_fast16_t j = end_cam_y; j <= start_cam_y; j++)
			for (uint_fast16_t i = camera->x; i < camera->x + camera->w; i++)
				set_subchunk_world(1, i, j);

	} else if (player->y > player_sy) {
		/* Apply verifications for moving DOWN */

		/* If went to bottom chunk.
		 * Move world to top and generate new chunks in bottom
		 */
		if (player->chunk_id.y < CHUNK_MAX_Y - 1 &&
			player->y >= VIEWPORT_HEIGHT * 2) {
			player->y = VIEWPORT_HEIGHT + (player->y - VIEWPORT_HEIGHT * 2);
			++player->chunk_id.y;

			/* Move world to top */
			memmove(&gameboard[0][0], &gameboard[VIEWPORT_HEIGHT][0],
					VIEWPORT_HEIGHT * 2 * VSCREEN_WIDTH);

			/* Generate new world at bottom */
			chunk_axis_t start_x = player->chunk_id.x - 1;
			for (uint_fast8_t i = 0; i < 3; ++i) {
				Chunk chunk = {.x = start_x + i, .y = player->chunk_id.y + 1};
				generate_chunk(WORLD_SEED, chunk, i * VIEWPORT_WIDTH,
							   VIEWPORT_HEIGHT * 2);
			}
			ResetSubchunks;
		}

		const int start_cam_y = camera->y + camera->h - 1;
		camera->y			  = clamp(player->y - VIEWPORT_HEIGHT_DIV_2, 0,
									  VSCREEN_HEIGHT - VIEWPORT_HEIGHT);
		const int end_cam_y	  = camera->y + camera->h - 1;

		/* Activate the next to subchunk on the bottom to draw it
		 * when entered the camera rect */
		for (uint_fast16_t j = start_cam_y; j < end_cam_y; j++)
			for (uint_fast16_t i = camera->x; i < camera->x + camera->w; i++)
				set_subchunk_world(1, i, j);
	}

	if (player->x < player_sx) {
		/* Apply verifications for moving LEFT */

		/* If went to left chunk.
		 * Move world to right and generate new chunks in left
		 */
		if (player->chunk_id.x > 1 && player->x < VIEWPORT_WIDTH) {
			player->x = VIEWPORT_WIDTH_M2 - (VIEWPORT_WIDTH - player->x);
			--player->chunk_id.x;

			/* Move world to right */
			for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
				memmove(&gameboard[j][VIEWPORT_WIDTH_M2],
						&gameboard[j][VIEWPORT_WIDTH], VIEWPORT_WIDTH);
				memmove(&gameboard[j][VIEWPORT_WIDTH], &gameboard[j][0],
						VIEWPORT_WIDTH);
			}

			/* Generate new world at left */
			chunk_axis_t start_j = player->chunk_id.y - 1;
			for (uint_fast8_t j = 0; j < 3; ++j) {
				Chunk chunk = {.x = player->chunk_id.x - 1, .y = start_j + j};
				generate_chunk(WORLD_SEED, chunk, 0, j * VIEWPORT_HEIGHT);
			}
			ResetSubchunks;
		}

		const int start_cam_x = camera->x;
		camera->x			  = clamp(player->x - VIEWPORT_WIDTH_DIV_2, 0,
									  VSCREEN_WIDTH - VIEWPORT_WIDTH);
		const int end_cam_x	  = camera->x;

		/* Activate the next to subchunk on the left to draw it
		 * when entered the camera rect */
		for (uint_fast16_t i = end_cam_x; i <= start_cam_x; i++)
			for (uint_fast16_t j = camera->y; j < camera->y + camera->h; j++)
				set_subchunk_world(1, i, j);

	} else if (player->x > player_sx) {
		/* Apply verifications for moving RIGHT */

		/* If went to right chunk.
		 * Move world to left and generate new chunks in right
		 */
		if (player->chunk_id.x < CHUNK_MAX_X - 1 &&
			player->x >= VIEWPORT_WIDTH_M2) {
			player->x = VIEWPORT_WIDTH + (player->x - VIEWPORT_WIDTH_M2);
			++player->chunk_id.x;

			/* Move world to left */
			for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
				memmove(&gameboard[j][0], &gameboard[j][VIEWPORT_WIDTH],
						VIEWPORT_WIDTH);
				memmove(&gameboard[j][VIEWPORT_WIDTH],
						&gameboard[j][VIEWPORT_WIDTH_M2], VIEWPORT_WIDTH);
			}

			/* Generate new world at right */
			chunk_axis_t start_j = player->chunk_id.y - 1;
			for (uint_fast8_t j = 0; j < 3; ++j) {
				Chunk chunk = {.x = player->chunk_id.x + 1, .y = start_j + j};
				generate_chunk(WORLD_SEED, chunk, VIEWPORT_WIDTH_M2,
							   j * VIEWPORT_HEIGHT);
			}
			ResetSubchunks;
		}

		const int start_cam_x = camera->x + camera->w - 1;
		camera->x			  = clamp(player->x - VIEWPORT_WIDTH_DIV_2, 0,
									  VSCREEN_WIDTH - VIEWPORT_WIDTH);
		const int end_cam_x	  = camera->x + camera->w - 1;

		/* Activate the next to subchunk on the right to draw it
		 * when entered the camera rect */
		for (uint_fast16_t i = start_cam_x; i <= end_cam_x; i++)
			for (uint_fast16_t j = camera->y; j < camera->y + camera->h; j++)
				set_subchunk_world(1, i, j);
	}
}
