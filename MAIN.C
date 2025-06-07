#include "list.h"
#include "errno.h"
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <dir.h>

int main(int argc, char *argv[])
{
    char last_key;
    const char *dir_path;

    printf("WAVWSS - play WAV files through WSS compatible soundcard\n");
    printf("(c) Lefucjusz, Warszawa 2025\n\n");

    if (argc != 2) {
        printf("Usage: %s path_to_directory_with_wav_files\n", argv[0]);
        return -EINVAL;
    }

    dir_path = argv[1];

    player_init();
    gui_init(dir_path);

    do {
        player_task();
        last_key = gui_task();
    } while (last_key != GUI_KEY_EXIT);

    gui_deinit();
    player_deinit();

    return 0;
}
