#include "gui.h"
#include "list.h"
#include "errno.h"
#include "player.h"
#include <conio.h>
#include <dir.h>

// TODO add files list at the beginning, add files numbering, add elapsed and total time

static struct list_t *files;
static struct list_node_t *last;
static struct list_node_t *current;

static bool gui_compare_ascending(const void *val1, const void *val2)
{
    const char *name1 = val1;
    const char *name2 = val2;

    return (strcmp(name1, name2) > 0); // TODO add natsort
}

static int gui_create_files_list(void)
{
    struct ffblk fileinfo;
    int done;

    /* Create new files list */
    files = list_create();
    if (files == NULL) {
	return -ENOMEM;
    }

    /* Append all WAV files to the list and sort it alphabetically */
    done = findfirst("*.WAV", &fileinfo, 0);
    while (!done) {
	list_add(files, fileinfo.ff_name, sizeof(fileinfo.ff_name), LIST_APPEND);
	done = findnext(&fileinfo);
    }
    list_sort(files, gui_compare_ascending);

    return 0;
}

static void gui_show_files_list(void) // TODO
{

}

int gui_init(const char *path)
{
    int err;

    /* Change directory to the one provided by the user */
    err = chdir(path);
    if (err) {
	return -ENOENT;
    }

    /* Create files list */
    err = gui_create_files_list();
    if (err) {
	return err;
    }

    /* Show files list */
    gui_show_files_list();

    /* Start playback of the first song */
    current = files->head;
    player_start(current->data);

    return 0;
}

void gui_deinit(void)
{
    list_destroy(files);
}

int gui_task(void)
{
    char key;

    /* Display new track name */
    if (last != current) {
	printf("Now playing: %s\n", current->data);
	last = current;
    }

    /* Play next song if previous has ended */
    if ((player_get_state() == PLAYER_STOPPED) && (current->next != NULL)) {
	current = current->next;
	player_start(current->data);
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
	    player_start(current->data);
	    break;

	case GUI_KEY_PREV:
	    /* Get previous song if exists */
	    if (current->prev == NULL) {
		break;
	    }
	    current = current->prev;

	    /* Start playback */
	    player_start(current->data);
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

	default:
	    break;
    }

    return key;
}
