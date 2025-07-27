#include "list.h"
#include "gui.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dir.h>

int main(int argc, char *argv[])
{
	int err;
	char last_key;
	const char *dir_path;

	clrscr();
	printf("WAVWSS - play WAV files through WSS compatible soundcard\n");
	printf("(c) Lefucjusz, Warszawa 2025\n\n");

	if (argc == 1) {
		dir_path = NULL;
	}
	else if (argc == 2) {
		dir_path = argv[1];
	}
	else {
		printf("Usage: %s [path_to_directory_with_wav_files]\n", argv[0]);
		return -EINVAL;
	}

	err = player_init();
	if (err) {
		printf("Failed to init player module, error %d!\n", err);
		goto out_error;
	}

	err = gui_init(dir_path);
	if (err) {
		if (err == -ENOENT) {
			printf("No WAV files found in a specified location!\n");
		}
		else {
			printf("Failed to initialize GUI module, error %d!\n", err);
		}
		goto out_error;
	}

	do {
		player_task();
		last_key = gui_task();
	} while (last_key != GUI_KEY_EXIT);

out_error:
	gui_deinit();
	player_deinit();

	return err;
}
