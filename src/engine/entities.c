#include "entities.h"

#include "bonerig.h"

#include "../disk/worldctrl.h"

#define SLOPE 4

Bone player_bone_rig[] = {
	/* Base bone */
	{NULL, {0.0f, 0.0f}, {0.0}},
	/* Left shoulder */
	{&player_bone_rig[0], {degtorad(-136.0f), 5.0f}, {0.0}},
	/* Right shoulder */
	{&player_bone_rig[0], {degtorad(-44.0f), 5.0f}, {0.0}},
	/* Left leg */
	{&player_bone_rig[0], {degtorad(110.0f), 4.0f}, {0.0}},
	/* Right leg */
	{&player_bone_rig[0], {degtorad(70.0f), 4.0f}, {0.0}},
	/* Head */
	{&player_bone_rig[0], {degtorad(-90.0f), 7.0f}, {0.0}},
};
const size_t player_bone_count =
	sizeof(player_bone_rig) / sizeof(*player_bone_rig);

#define PSF	  4.0f
#define PSF_2 (PSF * 2)

SkinRig player_skin_rig[] = {
	/* Right upper arm */
	{&player_bone_rig[2], {55, 24, 8, 38}, {1.0f, 1.0f}},
	/* Right leg */
	{&player_bone_rig[4], {45, 57, 8, 38}, {1.0f, 1.0f}},
	/* Left leg */
	{&player_bone_rig[3], {10, 57, 8, 38}, {1.0f, 1.0f}},
	/* Chest */
	{&player_bone_rig[0], {20, 24, 24, 36}, {25.0f / PSF_2, 36.0f / PSF_2}},
	/* Left upper arm */
	{&player_bone_rig[1], {0, 24, 8, 38}, {1.0f, 1.0f}},
	/* Head */
	{&player_bone_rig[5], {19, 0, 24, 21}, {24.0f / PSF_2, 21.0f / PSF_2}},
};
const size_t player_skin_count =
	sizeof(player_skin_rig) / sizeof(*player_skin_rig);

static BoneAnimation anim_player_idle = {
	0.0f,
	2,
	5,
	{
		{BA_TRANSITION,
		 {{&player_bone_rig[1], 0.0},
		  {&player_bone_rig[2], 0.0},
		  {&player_bone_rig[3], 0.0},
		  {&player_bone_rig[4], 0.0},
		  {&player_bone_rig[5], 0.0}}},
		{0.2f,
		 {{&player_bone_rig[1], 0.0},
		  {&player_bone_rig[2], 0.0},
		  {&player_bone_rig[3], 0.0},
		  {&player_bone_rig[4], 0.0},
		  {&player_bone_rig[5], 0.0}}},
	},
};

static BoneAnimation anim_player_walk = {
	0.0f,
	3,
	4,
	{
		{BA_TRANSITION,
		 {{&player_bone_rig[1], 0.0},
		  {&player_bone_rig[2], 0.0},
		  {&player_bone_rig[3], 0.0},
		  {&player_bone_rig[4], 0.0}}},
		{0.4f,
		 {{&player_bone_rig[1], 20.0},
		  {&player_bone_rig[2], -20.0},
		  {&player_bone_rig[3], -20.0},
		  {&player_bone_rig[4], 20.0}}},
		{0.8f,
		 {{&player_bone_rig[1], -20.0},
		  {&player_bone_rig[2], 20.0},
		  {&player_bone_rig[3], 20.0},
		  {&player_bone_rig[4], -20.0}}},
	},
};

void create_player_body(Player *player) {
	player->body =
		box2d_body_create(b2_world, X_TO_U(player->x), X_TO_U(player->y),
						  b2_dynamicBody, false, false);
	box2d_body_set_fixed_rotation(player->body, true);

	const float player_width_div4_u = X_TO_U(player->width / 4);
	const float player_hd2_m_wd4m1_u =
		X_TO_U(player->height / 2 - player->width / 4 - 1);
	const float player_mhd2_p_wd4m1_u =
		X_TO_U(-player->height / 2 + player->width / 4 + 1);

	box2d_body_create_fixture(
		player->body,
		box2d_shape_box(player_width_div4_u, player_hd2_m_wd4m1_u, 0, 0),
		PLAYER_DENSITY, PLAYER_FRICTION, 0.0f);

	box2d_body_create_fixture(
		player->body,
		box2d_shape_circle(player_width_div4_u, 0, player_hd2_m_wd4m1_u),
		PLAYER_DENSITY, PLAYER_FRICTION, 0.0f);

	box2d_body_create_fixture(
		player->body,
		box2d_shape_circle(player_width_div4_u, 0, player_mhd2_p_wd4m1_u),
		PLAYER_DENSITY, PLAYER_FRICTION, 0.0f);

	player->animation = &anim_player_idle;
}

void move_player(Player *player, const Uint8 *keyboard) {
	player->prev_x = player->x;
	player->prev_y = player->y;

	float bx, by;
	box2d_body_get_position(player->body, &bx, &by);
	player->x = clamp(U_TO_X(bx), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_WIDTH - CHUNK_SIZE_DIV_2);
	player->y = clamp(U_TO_X(by), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_HEIGHT - CHUNK_SIZE_DIV_2);

	short facing = player->fliph ? -1 : 1;

	RaycastData ray_bottom;
	RaycastData ray_forward;

	box2d_sweep_raycast(player->body, &ray_bottom, 4,
						X_TO_U(player->width / 2 - 2), 0,
						X_TO_U(player->height / 2), true);
	box2d_sweep_raycast(player->body, &ray_forward, 8,
						X_TO_U(player->height - SLOPE * 2),
						X_TO_U(facing * (player->width / 4 + 1)), 0, false);

	bool player_is_on_floor = ray_bottom.hit;
	bool player_is_on_wall	= ray_forward.hit;

	/* Move down / Jump */
	if (keyboard[SDL_SCANCODE_W] && (player->flying || player_is_on_floor)) {
		/* Jump */
		box2d_body_set_velocity_v(player->body, player->flying
													? -PLAYER_FLYING_SPEED
													: PLAYER_VSPEED_JUMP);
	} else if (keyboard[SDL_SCANCODE_S] && player->flying) {
		box2d_body_set_velocity_v(
			player->body, player->flying ? PLAYER_FLYING_SPEED : PLAYER_SPEED);
	} else if (player->flying) {
		box2d_body_set_velocity_v(player->body, 0);
	}

	/* Move left/right */
	float hspeed = keyboard[SDL_SCANCODE_D] - keyboard[SDL_SCANCODE_A];
	if (hspeed != 0) {
		player->fliph = (hspeed < 0) ? true : false;
		hspeed *= player->flying ? PLAYER_FLYING_SPEED : PLAYER_SPEED;
		if (!player->flying) {
			if (player_is_on_floor) {
				play_animation(&player->animation, &anim_player_walk, false);
			} else {
				play_animation(&player->animation, &anim_player_idle, false);
			}

			if (player_is_on_wall && SIGN(hspeed) == SIGN(facing)) {
				hspeed *= (float)facing * fabsf(ray_forward.normal_y);
				if (player_is_on_floor)
					box2d_body_add_velocity(player->body, 0,
											-fabsf(ray_forward.normal_x));
			} else if (player_is_on_floor) {
				box2d_body_add_velocity(player->body, 0, 0.8f);
			}
		} else {
			play_animation(&player->animation, &anim_player_idle, false);
		}
		box2d_body_set_velocity_h(player->body, hspeed);
	} else if (player->flying || player_is_on_floor) {
		play_animation(&player->animation, &anim_player_idle, false);
		box2d_body_set_velocity_h(player->body, 0);
	}
}

#define dereference_chunk_by_lines(addr_, vy_, vx_)                            \
	for (size_t __k = 0; __k < CHUNK_SIZE; ++__k) {                            \
		memcpy(&gameboard[(vy_ + __k)][vx_], (addr_) + (__k * CHUNK_SIZE),     \
			   CHUNK_SIZE);                                                    \
	}
/** This is called after box2d_world_step */
void move_camera(Player *player, SDL_FRect *camera) {
	float bx, by;
	box2d_body_get_position(player->body, &bx, &by);

	/* Set player position after world_step */
	player->x = clamp(U_TO_X(bx), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_WIDTH - CHUNK_SIZE_DIV_2);
	player->y = clamp(U_TO_X(by), CHUNK_SIZE_DIV_2 + 1,
					  VSCREEN_HEIGHT - CHUNK_SIZE_DIV_2);
	if (player->y != by || player->x != bx)
		/* Reposition player->body */
		box2d_body_set_position(player->body, X_TO_U(player->x),
								X_TO_U(player->y));

	const float player_sx = player->x;
	const float player_sy = player->y;

	/* Generate new chunks and move camera */
	if (player->y < player->prev_y) {
		/* Apply verifications for moving UP */

		/* If went to top chunk.
		 * Move world to bottom and generate new chunks in top
		 */
		if (player->chunk_id.y > 1 && player->y < CHUNK_SIZE) {

			player->y = (float)CHUNK_SIZE_M2 - (CHUNK_SIZE - player->y);
			player->prev_y =
				(float)CHUNK_SIZE_M2 - (CHUNK_SIZE - player->prev_y);
			--player->chunk_id.y;

			/* Move world to bottom */
			deactivate_soil_all;
			box2d_world_move_all_bodies(b2_world, 0, X_TO_U(CHUNK_SIZE));

			/* Save modified chunks to cache before erasing */
			if (vctable[2][0].modified)
				cache_chunk(vctable[2][0], CHUNK_SIZE_M2, 0);
			if (vctable[2][1].modified)
				cache_chunk(vctable[2][1], CHUNK_SIZE_M2, CHUNK_SIZE);
			if (vctable[2][2].modified)
				cache_chunk(vctable[2][2], CHUNK_SIZE_M2, CHUNK_SIZE_M2);

			/* Overwrite vctable and gameboard */
			memmove(&vctable[1][0], &vctable[0][0], 6 * sizeof(Chunk));
			memmove(&gameboard[CHUNK_SIZE][0], &gameboard[0][0],
					CHUNK_SIZE_M2 * VSCREEN_WIDTH);

			/* Generate new world at top */
			chunk_xaxis_t start_x = player->chunk_id.x - 1;
			for (uint_fast8_t i = 0; i < 3; ++i) {
				Chunk chunk = {
					.x		  = start_x + i,
					.y		  = player->chunk_id.y - 1,
					.modified = 0,
				};
				vctable[0][i] = chunk;

				const size_t vx = i * CHUNK_SIZE;
				const size_t vy = 0;

				/* Find chunk in cache, else read it from disk,
				 * otherwise generate it. */
				GO_ID *chunk_data = cache_get_chunk(chunk);
				if (chunk_data != NULL) {
					dereference_chunk_by_lines(chunk_data, vy, vx);
				} else {
					GO_ID chunk_data_disk[CHUNK_MEMSIZE];
					if (load_chunk_from_disk(chunk, chunk_data_disk)) {
						dereference_chunk_by_lines(chunk_data_disk, vy, vx);
					} else {
						generate_chunk(WORLD_SEED, chunk, vx, vy);
					}
				}
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

			player->y = (float)CHUNK_SIZE + (player->y - CHUNK_SIZE_M2);
			player->prev_y =
				(float)CHUNK_SIZE + (player->prev_y - CHUNK_SIZE_M2);
			++player->chunk_id.y;

			/* Move world to top */
			deactivate_soil_all;
			box2d_world_move_all_bodies(b2_world, 0, -X_TO_U(CHUNK_SIZE));

			/* Save modified chunks to cache before erasing */
			if (vctable[0][0].modified)
				cache_chunk(vctable[0][0], 0, 0);
			if (vctable[0][1].modified)
				cache_chunk(vctable[0][1], 0, CHUNK_SIZE);
			if (vctable[0][2].modified)
				cache_chunk(vctable[0][2], 0, CHUNK_SIZE_M2);

			/* Overwrite vctable and gameboard */
			memmove(&vctable[0][0], &vctable[1][0], 6 * sizeof(Chunk));
			memmove(&gameboard[0][0], &gameboard[CHUNK_SIZE][0],
					CHUNK_SIZE_M2 * VSCREEN_WIDTH);

			/* Generate new world at bottom */
			chunk_xaxis_t start_x = player->chunk_id.x - 1;
			for (uint_fast8_t i = 0; i < 3; ++i) {
				Chunk chunk = {
					.x		  = start_x + i,
					.y		  = player->chunk_id.y + 1,
					.modified = 0,
				};
				vctable[2][i] = chunk;

				const size_t vx = i * CHUNK_SIZE;
				const size_t vy = CHUNK_SIZE_M2;

				/* Find chunk in cache, else read it from disk,
				 * otherwise generate it. */
				GO_ID *chunk_data = cache_get_chunk(chunk);
				if (chunk_data != NULL) {
					dereference_chunk_by_lines(chunk_data, vy, vx);
				} else {
					GO_ID chunk_data_disk[CHUNK_MEMSIZE];
					if (load_chunk_from_disk(chunk, chunk_data_disk)) {
						dereference_chunk_by_lines(chunk_data_disk, vy, vx);
					} else {
						generate_chunk(WORLD_SEED, chunk, vx, vy);
					}
				}
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

			player->x = (float)CHUNK_SIZE_M2 - (CHUNK_SIZE - player->x);
			player->prev_x =
				(float)CHUNK_SIZE_M2 - (CHUNK_SIZE - player->prev_x);
			--player->chunk_id.x;

			/* Move world to right */
			deactivate_soil_all;
			box2d_world_move_all_bodies(b2_world, X_TO_U(CHUNK_SIZE), 0);

			/* Save modified chunks to cache before erasing */
			if (vctable[0][2].modified)
				cache_chunk(vctable[0][2], 0, CHUNK_SIZE_M2);
			if (vctable[1][2].modified)
				cache_chunk(vctable[1][2], CHUNK_SIZE, CHUNK_SIZE_M2);
			if (vctable[2][2].modified)
				cache_chunk(vctable[2][2], CHUNK_SIZE_M2, CHUNK_SIZE_M2);

			/* Overwrite vctable and gameboard */
			for (uint_fast16_t j = 0; j < 3; ++j) {
				vctable[j][2] = vctable[j][1];
				vctable[j][1] = vctable[j][0];
			}
			for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
				memmove(&gameboard[j][CHUNK_SIZE], &gameboard[j][0],
						CHUNK_SIZE_M2);
			}

			/* Generate new world at left */
			chunk_yaxis_t start_j = player->chunk_id.y - 1;
			for (uint_fast8_t j = 0; j < 3; ++j) {
				Chunk chunk = {
					.x		  = player->chunk_id.x - 1,
					.y		  = start_j + j,
					.modified = 0,
				};
				vctable[j][0] = chunk;

				const size_t vx = 0;
				const size_t vy = j * CHUNK_SIZE;

				/* Find chunk in cache, else read it from disk,
				 * otherwise generate it. */
				GO_ID *chunk_data = cache_get_chunk(chunk);
				if (chunk_data != NULL) {
					dereference_chunk_by_lines(chunk_data, vy, vx);
				} else {
					GO_ID chunk_data_disk[CHUNK_MEMSIZE];
					if (load_chunk_from_disk(chunk, chunk_data_disk)) {
						dereference_chunk_by_lines(chunk_data_disk, vy, vx);
					} else {
						generate_chunk(WORLD_SEED, chunk, vx, vy);
					}
				}
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

			player->x = (float)CHUNK_SIZE + (player->x - CHUNK_SIZE_M2);
			player->prev_x =
				(float)CHUNK_SIZE + (player->prev_x - CHUNK_SIZE_M2);
			++player->chunk_id.x;

			/* Move world to left */
			deactivate_soil_all;
			box2d_world_move_all_bodies(b2_world, -X_TO_U(CHUNK_SIZE), 0);

			/* Save modified chunks to cache before erasing */
			if (vctable[0][0].modified)
				cache_chunk(vctable[0][0], 0, 0);
			if (vctable[1][0].modified)
				cache_chunk(vctable[1][0], CHUNK_SIZE, 0);
			if (vctable[2][0].modified)
				cache_chunk(vctable[2][0], CHUNK_SIZE_M2, 0);

			/* Overwrite vctable and gameboard */
			for (uint_fast16_t j = 0; j < 3; ++j) {
				vctable[j][0] = vctable[j][1];
				vctable[j][1] = vctable[j][2];
			}
			for (uint_fast16_t j = 0; j < VSCREEN_HEIGHT; ++j) {
				memmove(&gameboard[j][0], &gameboard[j][CHUNK_SIZE],
						CHUNK_SIZE_M2);
			}

			/* Generate new world at right */
			chunk_yaxis_t start_j = player->chunk_id.y - 1;
			for (uint_fast8_t j = 0; j < 3; ++j) {
				Chunk chunk = {
					.x		  = player->chunk_id.x + 1,
					.y		  = start_j + j,
					.modified = 0,
				};

				vctable[j][2] = chunk;

				const size_t vx = CHUNK_SIZE_M2;
				const size_t vy = j * CHUNK_SIZE;

				/* Find chunk in cache, else read it from disk,
				 * otherwise generate it. */
				GO_ID *chunk_data = cache_get_chunk(chunk);
				if (chunk_data != NULL) {
					dereference_chunk_by_lines(chunk_data, vy, vx);
				} else {
					GO_ID chunk_data_disk[CHUNK_MEMSIZE];
					if (load_chunk_from_disk(chunk, chunk_data_disk)) {
						dereference_chunk_by_lines(chunk_data_disk, vy, vx);
					} else {
						generate_chunk(WORLD_SEED, chunk, vx, vy);
					}
				}
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

void draw_player(Player *player, SDL_FRect *camera) {
	const float player_screen_x = player->x - camera->x;
	const float player_screen_y = player->y - camera->y;

	const float player_angle	 = box2d_body_get_angle(player->body);
	const float player_angle_deg = radtodeg(player_angle);

	for (size_t k = 0; k < player_skin_count; ++k) {
		SkinRig *skinrig = &player_skin_rig[k];
		Bone	*bone	 = skinrig->bone;

		float bone_x, bone_y, bone_angle;
		bone_get_world_position(bone, player_angle, &bone_x, &bone_y,
								&bone_angle, player->fliph);

		double angle = bone->anim_data.angle + player_angle_deg;

		SDL_FRect skin_dst = {bone_x, bone_y,
							  ((float)skinrig->subimage.w / PSF),
							  ((float)skinrig->subimage.h / PSF)};
		skin_dst.x += player_screen_x - skinrig->center.x + 0.5f;
		skin_dst.y += player_screen_y - skinrig->center.y - 0.5f;

		SDL_RenderCopyExF(__renderer, player->sprite->texture,
						  &skinrig->subimage, &skin_dst, angle,
						  &skinrig->center, player->fliph);
	}
}
