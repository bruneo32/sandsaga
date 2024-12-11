#include "entities.h"

void move_player(Player *player, const Uint8 *keyboard) {
	player->prev_x = player->x;
	player->prev_y = player->y;

	float bx, by;
	box2d_body_get_position(player->body, &bx, &by);
	player->x = clamp(((short)U_TO_X(bx)), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_WIDTH - CHUNK_SIZE_DIV_2);
	player->y = clamp(((short)U_TO_X(by)), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_HEIGHT - CHUNK_SIZE_DIV_2);

	const short player_width_div2  = player->width / 2;
	const short player_height_div2 = player->height / 2;
	const short player_bottom	   = (player->y + player_height_div2);

	bool player_is_on_floor = false;
	for (short ix = player->x - player_width_div2;
		 ix < player->x + player_width_div2; ++ix) {
		if (gameboard[player_bottom][ix] != GO_NONE) {
			player_is_on_floor = true;
			break;
		}
	}

	/* Move down / Jump */
	if (keyboard[SDL_SCANCODE_W]) {
		/* Jump */
		box2d_body_set_velocity_v(player->body, player->flying
													? -PLAYER_FLYING_SPEED
													: PLAYER_VSPEED_JUMP);
	} else if (keyboard[SDL_SCANCODE_S]) {
		box2d_body_set_velocity_v(
			player->body, player->flying ? PLAYER_FLYING_SPEED : PLAYER_SPEED);
	} else if (player->flying) {
		box2d_body_set_velocity_v(player->body, 0);
	}

	short hspeed = keyboard[SDL_SCANCODE_D] - keyboard[SDL_SCANCODE_A];

	/* Move left/right */
	if (hspeed != 0) {
		player->fliph = (hspeed < 0) ? true : false;
		hspeed *= player->flying ? PLAYER_FLYING_SPEED : PLAYER_SPEED;
		box2d_body_set_velocity_h(player->body, hspeed);
	} else if (player->flying || player_is_on_floor) {
		box2d_body_set_velocity_h(player->body, 0);
	}

	const size_t si = (player->x) / SUBCHUNK_WIDTH;
	const size_t sj = (player->y) / SUBCHUNK_HEIGHT;

	for (size_t j = sj - 2; j <= sj + 2; ++j) {
		for (size_t i = si - 2; i <= si + 2; ++i) {
			if (i >= si - 1 && i <= si + 1 && j >= sj - 1 && j <= sj + 1)
				activate_soil(i, j);
			else
				deactivate_soil(i, j);
		}
	}
}

/** This is called after box2d_world_step */
void move_camera(Player *player, SDL_Rect *camera) {
	float bx, by;
	box2d_body_get_position(player->body, &bx, &by);
	player->x = clamp(((short)U_TO_X(bx)), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_WIDTH - CHUNK_SIZE_DIV_2);
	player->y = clamp(((short)U_TO_X(by)), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_HEIGHT - CHUNK_SIZE_DIV_2);

	const short player_sx = player->x;
	const short player_sy = player->y;

	/* Generate new chunks and move camera */
	if (player->y < player->prev_y) {
		/* Apply verifications for moving UP */

		/* If went to top chunk.
		 * Move world to bottom and generate new chunks in top
		 */
		if (player->chunk_id.y > 1 && player->y < CHUNK_SIZE) {

			player->y	   = CHUNK_SIZE_M2 - (CHUNK_SIZE - player->y);
			player->prev_y = CHUNK_SIZE_M2 - (CHUNK_SIZE - player->prev_y);
			--player->chunk_id.y;

			/* Move world to bottom */
			deactivate_soil_all;
			memmove(&gameboard[CHUNK_SIZE][0], &gameboard[0][0],
					CHUNK_SIZE_M2 * VSCREEN_WIDTH);

			/* Generate new world at top */
			chunk_axis_t start_x = player->chunk_id.x - 1;
			for (uint_fast8_t i = 0; i < 3; ++i) {
				Chunk chunk = {.x = start_x + i, .y = player->chunk_id.y - 1};
				generate_chunk(WORLD_SEED, chunk, i * CHUNK_SIZE, 0);
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

	} else if (player->y > player->prev_y) {
		/* Apply verifications for moving DOWN */

		/* If went to bottom chunk.
		 * Move world to top and generate new chunks in bottom
		 */
		if (player->chunk_id.y < CHUNK_MAX_Y - 1 &&
			player->y >= CHUNK_SIZE_M2) {

			player->y	   = CHUNK_SIZE + (player->y - CHUNK_SIZE_M2);
			player->prev_y = CHUNK_SIZE + (player->prev_y - CHUNK_SIZE_M2);
			++player->chunk_id.y;

			/* Move world to top */
			deactivate_soil_all;
			memmove(&gameboard[0][0], &gameboard[CHUNK_SIZE][0],
					CHUNK_SIZE_M2 * VSCREEN_WIDTH);

			/* Generate new world at bottom */
			chunk_axis_t start_x = player->chunk_id.x - 1;
			for (uint_fast8_t i = 0; i < 3; ++i) {
				Chunk chunk = {.x = start_x + i, .y = player->chunk_id.y + 1};
				generate_chunk(WORLD_SEED, chunk, i * CHUNK_SIZE,
							   CHUNK_SIZE_M2);
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

	if (player->x < player->prev_x) {
		/* Apply verifications for moving LEFT */

		/* If went to left chunk.
		 * Move world to right and generate new chunks in left
		 */
		if (player->chunk_id.x > 1 && player->x < CHUNK_SIZE) {

			player->x	   = CHUNK_SIZE_M2 - (CHUNK_SIZE - player->x);
			player->prev_x = CHUNK_SIZE_M2 - (CHUNK_SIZE - player->prev_x);
			--player->chunk_id.x;

			/* Move world to right */
			deactivate_soil_all;
			for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
				memmove(&gameboard[j][CHUNK_SIZE], &gameboard[j][0],
						CHUNK_SIZE_M2);
			}

			/* Generate new world at left */
			chunk_axis_t start_j = player->chunk_id.y - 1;
			for (uint_fast8_t j = 0; j < 3; ++j) {
				Chunk chunk = {.x = player->chunk_id.x - 1, .y = start_j + j};
				generate_chunk(WORLD_SEED, chunk, 0, j * CHUNK_SIZE);
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

	} else if (player->x > player->prev_x) {
		/* Apply verifications for moving RIGHT */

		/* If went to right chunk.
		 * Move world to left and generate new chunks in right
		 */
		if (player->chunk_id.x < CHUNK_MAX_X - 1 &&
			player->x >= CHUNK_SIZE_M2) {

			player->x	   = CHUNK_SIZE + (player->x - CHUNK_SIZE_M2);
			player->prev_x = CHUNK_SIZE + (player->prev_x - CHUNK_SIZE_M2);
			++player->chunk_id.x;

			/* Move world to left */
			deactivate_soil_all;
			for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
				memmove(&gameboard[j][0], &gameboard[j][CHUNK_SIZE],
						CHUNK_SIZE_M2);
			}

			/* Generate new world at right */
			chunk_axis_t start_j = player->chunk_id.y - 1;
			for (uint_fast8_t j = 0; j < 3; ++j) {
				Chunk chunk = {.x = player->chunk_id.x + 1, .y = start_j + j};
				generate_chunk(WORLD_SEED, chunk, CHUNK_SIZE_M2,
							   j * CHUNK_SIZE);
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

	/* Reposition player->body */
	if (player->y != player_sy || player->x != player_sx) {
		player->x = clamp(player->x, CHUNK_SIZE_DIV_2 + 1,
						  VSCREEN_WIDTH - CHUNK_SIZE_DIV_2);
		player->y = clamp(player->y, CHUNK_SIZE_DIV_2 + 1,
						  VSCREEN_HEIGHT - CHUNK_SIZE_DIV_2);
		box2d_body_set_position(player->body, X_TO_U(player->x),
								X_TO_U(player->y));
	}
}
