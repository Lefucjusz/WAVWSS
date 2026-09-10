#include "settings.h"
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <string.h>

#define SETTINGS_FILENAME "wavwss.dat"

#define SETTINGS_DEFAULT_VOLUME 50

struct settings_data_t
{
	uint32_t seconds_played;
	uint8_t volume;
};

static struct settings_data_t data;
static int fd = -1;

static int settings_get_path(char *out_path, const char *exe_path)
{
	char *p;

	/* Create mutable copy */
	strcpy(out_path, exe_path);

	/* Find a separator between directory and EXE name */
	p = strrchr(out_path, '\\');
	if (p == NULL) {
		return -EINVAL;
	}

	/* Remove EXE name */
	*(p + 1) = '\0';

	/* Append settings filename */
	strcat(out_path, SETTINGS_FILENAME);

	return 0;
}

int settings_init(const char *exe_path)
{
	int err;
	int bytes_read;
	char settings_path[128];

	/* Resolve settings file path */
	err = settings_get_path(settings_path, exe_path);
	if (err) {
		return err;
	}

	/* Open settings file */
	fd = open(settings_path, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
	if (fd < 0) {
		return -EIO;
	}

	/* Validate and set defaults if empty */
	bytes_read = read(fd, &data, sizeof(data));
	if (bytes_read != sizeof(data)) {
		data.seconds_played = 0;
		data.volume = SETTINGS_DEFAULT_VOLUME;

		err = settings_sync();
		if (err) {
			close(fd);
			return err;
		}
	}

	return 0;
}

int settings_deinit(void)
{
	int err;

	err = settings_sync();

	close(fd);
	fd = -1;

	return err;
}

uint32_t settings_get_played_seconds(void)
{
	return data.seconds_played;
}

void settings_add_played_second(void)
{
	++data.seconds_played;
}

uint8_t settings_get_volume(void)
{
	return data.volume;
}

void settings_set_volume(uint8_t volume)
{
	data.volume = volume;
}

int settings_sync(void)
{
	if (fd < 0) {
		return -EBADF;
	}

	if (lseek(fd, 0, SEEK_SET) < 0) {
		return -EIO;
	}

	if (write(fd, &data, sizeof(data)) != sizeof(data)) {
		return -EIO;
	}

	return 0;
}
