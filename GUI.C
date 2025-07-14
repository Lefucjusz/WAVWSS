#include "gui.h"
#include "wav.h"
#include "list.h"
#include "player.h"
#include "utils.h"
#include "natsort.h"
#include <errno.h>
#include <conio.h>
#include <dir.h>
#include <stdio.h>

#define GUI_PLAY_CHAR '>'
#define GUI_PAUSE_CHAR '='

#define GUI_VOLUME_STEP 2
#define GUI_INITIAL_VOLUME 50

struct gui_track_t
{
	char name[MEMBER_SIZE(struct ffblk, ff_name)];
	uint32_t length;
};

static struct list_t *files;
static struct list_node_t *current;
static uint32_t last_elapsed_time;
static enum player_state_t last_player_state;
static char prev_cwd[128];
static int8_t volume;

static bool gui_compare_ascending(const void *val1, const void *val2)
{
	const struct gui_track_t *track1 = val1;
	const struct gui_track_t *track2 = val2;

	return (strnatcmp(track1->name, track2->name) > 0);
}

static uint32_t gui_get_track_length(const char *path)
{
	int err;
	FILE *fd;
	struct wav_header_t header;

	fd = fopen(path, "rb");
	if (fd == NULL) {
		return 0;
	}

	/* Skips header and returns PCM data size */
	err = wav_parse_header(fd, &header);
	if (err) {
		return 0;
	}

	fclose(fd);

	return (header.data_size / header.byte_rate);
}

static int gui_create_files_list(void)
{
	uint32_t files_count = 0;
	int done;
	struct ffblk fileinfo;
	struct gui_track_t track;

	/* Create new files list */
	files = list_create();
	if (files == NULL) {
		return -ENOMEM;
	}

	/* Append all WAV files to the list and sort it alphabetically */
	done = findfirst("*.wav", &fileinfo, 0);
	while (!done) {
		memcpy(track.name, fileinfo.ff_name, sizeof(track.name));
		track.length = gui_get_track_length(track.name);

		list_add(files, &track, sizeof(track), LIST_APPEND);
		++files_count;

		done = findnext(&fileinfo);
	}
	list_sort(files, gui_compare_ascending);

	if (files_count == 0) {
		return -ENOENT;
	}

	return 0;
}

static void gui_show_files_list(void)
{
	uint32_t length_total = 0;
	uint32_t track_num = 1;
	struct list_node_t *file = files->head;
	struct gui_track_t *track;

	printf("----------------------------\tControls:\n");
	printf("---------- Tracks ----------\t[ESC] - leave to DOS\t[I] - previous track\n");
	printf("----------------------------\t[Y] - volume down\t[O] - next track\n");
	printf("|      Track      |  Time  |\t[U] - volume up\t\t[P] - pause/resume\n");
	printf("|-----------------|--------|\n");

	do {
		track = file->data;

		printf("| %02lu. %-12s| %02lu:%02lu  |\n", track_num, track->name, track->length / 60, track->length % 60);
		length_total += track->length;
		++track_num;

		file = file->next;
	} while (file != NULL);

	printf("----------------------------\n");
	printf("|      Total      | %02lu:%02lu  |\n\n", length_total / 60, length_total % 60);
}

int gui_init(const char *path)
{
	int err;
	struct gui_track_t *track;

	/* Preserve current working directory and change it to the one provided by the user */
	getcwd(prev_cwd, sizeof(prev_cwd));
	err = chdir(path);
	if (err) {
		gui_deinit();
		return -ENOENT;
	}

	/* Create files list */
	err = gui_create_files_list();
	if (err) {
		gui_deinit();
		return err;
	}

	/* Show files list */
	gui_show_files_list();

	/* Set volume */
	volume = GUI_INITIAL_VOLUME;
	player_set_volume(volume);

	/* Start playback of the first song */
	current = files->head;
	track = current->data;
	err = player_start(track->name);
	if (err) {
		return err;
	}

	return 0;
}

void gui_deinit(void)
{
	/* Restore previous working directory */
	chdir(prev_cwd);

	/* Cleanup */
	list_destroy(files);
}

int gui_task(void)
{
	char key;
	char track_state;
	struct gui_track_t *track;
	uint32_t elapsed_time;
	enum player_state_t player_state;

	/* Update track info. This should refresh also if the volume
	 * has been changed, but on Xi8088 this causes audio buffer
	 * underruns if the volume is changed quickly. On faster
	 * machines this shouldn't be an issue. */
	elapsed_time = player_get_seconds_played();
	player_state = player_get_state();
	if ((last_elapsed_time != elapsed_time) ||
		(last_player_state != player_state)) {
		track = current->data;
		track_state = (player_state == PLAYER_PLAYING) ? GUI_PLAY_CHAR : GUI_PAUSE_CHAR;

		printf("Now playing: [%c] %s (%02lu:%02lu/%02lu:%02lu) | Volume: %d%%  \r", track_state, track->name, elapsed_time / 60, elapsed_time % 60, track->length / 60, track->length % 60, (int)volume);

		last_elapsed_time = elapsed_time;
		last_player_state = player_state;
	}

	/* Play next song if previous has ended */
	if (player_get_state() == PLAYER_STOPPED) {
		if (current->next == NULL) {
			printf("\nDone!\n");
			return GUI_KEY_EXIT; // We've played all the files, exit
		}
		current = current->next;
		track = current->data;
		player_start(track->name);
	}

	/* Handle user input */
	if (!kbhit()) {
		return 0;
	}

	key = toupper(getch());
	switch (key) {
		case GUI_KEY_NEXT:
			/* Get next song if exists */
			if (current->next == NULL) {
				break;
			}
			current = current->next;

			/* Start playback */
			track = current->data;
			player_start(track->name);

			/* Force track info refresh */
			last_player_state = PLAYER_STOPPED;
			break;

		case GUI_KEY_PREV:
			/* Get previous song if exists */
			if (current->prev == NULL) {
				break;
			}
			current = current->prev;

			/* Start playback */
			track = current->data;
			player_start(track->name);

			/* Force track info refresh */
			last_player_state = PLAYER_STOPPED;
			break;

		case GUI_KEY_PAUSE:
			switch (player_get_state()) {
				case PLAYER_PLAYING:
					player_pause();
					break;

				case PLAYER_PAUSED:
					player_resume();
					break;

				default:
					break;
			}
			break;

		case GUI_KEY_VOLUME_UP:
			volume += GUI_VOLUME_STEP;
			volume = CLAMP(volume, PERCENT_MIN, PERCENT_MAX);
			player_set_volume(volume);
			break;

		case GUI_KEY_VOLUME_DOWN:
			volume -= GUI_VOLUME_STEP;
			volume = CLAMP(volume, PERCENT_MIN, PERCENT_MAX);
			player_set_volume(volume);
			break;

		default:
			break;
	}

	return key;
}
