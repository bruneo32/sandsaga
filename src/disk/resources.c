#include "resources.h"

#include "../log/log.h"

Sprite *default_player_sprite = NULL;

const char *player_data_folder = "player" PATH_SEP_STR;
const char *player_sprite_file = "skin.png";

void load_player_sprite(Player *player, unsigned char default_sprite_data[],
						unsigned int default_sprite_data_len) {
	/* Create player data folder */
	if (!player_data_folder_path) {
		player_data_folder_path = calloc(
			strlen(user_path) + strlen(player_data_folder) + 1, sizeof(char));
		strcpy(player_data_folder_path, user_path);
		strcat(player_data_folder_path, player_data_folder);
		mkdir_r(player_data_folder_path);
	}

	/* Set player sprite file path */
	if (!player_sprite_file_path) {
		player_sprite_file_path = calloc(strlen(player_data_folder_path) +
											 strlen(player_sprite_file) + 1,
										 sizeof(char));
		strcpy(player_sprite_file_path, player_data_folder_path);
		strcat(player_sprite_file_path, player_sprite_file);
	}

	/* Load default sprite (for when any player doesn't have a skin) */
	default_player_sprite = loadIMG_from_mem(
		default_sprite_data, default_sprite_data_len, __window, __renderer);

	/* Create skin file if not present */
	if (!file_exists(player_sprite_file_path)) {
		player->sprite = default_player_sprite;
		loginfo("Creating default player sprite: \"%s\"",
				player_sprite_file_path);

		/* Write default skin to disk */
		FILE *spr_file = fopen(player_sprite_file_path, "wb");
		if (!spr_file) {
			logerr("Failed to open player sprite file for writing: %s",
				   strerror(errno));
			return;
		}

		size_t bytes_written =
			fwrite(default_sprite_data, 1, default_sprite_data_len, spr_file);

		if (bytes_written != default_sprite_data_len)
			logerr("Failed to write player sprite data: %s", strerror(errno));

		fclose(spr_file);
		return;
	}

	/* Load player sprite */
	player->sprite =
		loadIMG_from_path(player_sprite_file_path, __window, __renderer);
}
