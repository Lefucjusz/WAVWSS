#include "player.h"
#include "wss.h"
#include "wav.h"
#include "irq.h"
#include "buffer.h"
#include "stdbool.h"
#include <errno.h>
#include <stdio.h>
#include <dos.h>

/* Double buffering */
#define PLAYER_SINGLE_BUFFER_SIZE (BUFFER_SIZE_BYTES / 2)

static volatile bool wss_request;
static enum player_state_t state;
static struct buffer_t buffer;
static uint8_t buffer_index;
static uint32_t bytes_played;
static struct wav_header_t wav_header;
static FILE *fd;

static void interrupt player_irq_handler(void)
{
	wss_request = true;
	wss_write_direct(WSS_STATUS_REG_OFFSET, 0x00); // Clear WSS interrupt bit
	outp(IRQ_PIC_STATUS_REG, IRQ_PIC_END_OF_IRQ); // Acknowledge interrupt
}

int player_init(void)
{
	int err;

	/* Set initial state */
	state = PLAYER_STOPPED;

	/* Initialize WSS */
	wss_init();

	/* Allocate PCM buffer */
	err = buffer_allocate(&buffer);
	if (err) {
		return err;
	}

	/* Initialize IRQ handler */
	irq_init(wss_get_irq_number(), player_irq_handler);

	return 0;
}

void player_deinit(void)
{
	/* Stop player */
	player_stop();

	/* Release resources */
	dma_release(wss_get_dma_channel());
	irq_release(wss_get_irq_number());

	/* Free PCM buffer */
	buffer_free(&buffer);
}

int player_start(const char *path)
{
	int err;
	size_t bytes_read;
	struct wss_playback_cfg_t playback_cfg;

	/* Stop player if not stopped yet */
	if (state != PLAYER_STOPPED) {
		player_stop();
	}

	/* Set initial state */
	wss_request = false;
	buffer_index = 0;
	bytes_played = 0;

	/* Open WAV file */
	fd = fopen(path, "rb");
	if (fd == NULL) {
		return -ENOENT;
	}

	/* Skip WAV header and get PCM data size */
	err = wav_parse_header(fd, &wav_header);
	if (err) {
		fclose(fd);
		return err;
	}

	/* Preload buffer */
	bytes_read = fread(buffer.data, 1, BUFFER_SIZE_BYTES, fd);
	bytes_played += bytes_read;

	/* Start DMA */
	dma_autoinit_start(wss_get_dma_channel(), buffer.page, buffer.offset, BUFFER_SIZE_BYTES);

	/* Start playback */
	playback_cfg.buffer_size = BUFFER_SIZE_BYTES;
	playback_cfg.sample_rate = wav_header.sample_rate;
	playback_cfg.bit_depth = wav_header.bit_depth;
	playback_cfg.channels_num = wav_header.num_channels;
	err = wss_playback_start(&playback_cfg);
	if (err) {
		fclose(fd);
		return err;
	}

	state = PLAYER_PLAYING;

	return 0;
}

int player_pause(void)
{
	int err;

	err = wss_playback_stop();
	if (err) {
		return err;
	}

	state = PLAYER_PAUSED;

	return 0;
}

int player_resume(void)
{
	int err;

	err = wss_playback_continue();
	if (err) {
		return err;
	}

	state = PLAYER_PLAYING;

	return 0;
}

int player_stop(void)
{
	int err;

	/* Stop playback */
	err = wss_playback_stop();
	if (err) {
		return err;
	}

	/* Close file if opened */
	if (fd != NULL) {
		fclose(fd);
		fd = NULL;
	}

	state = PLAYER_STOPPED;

	return 0;
}

int player_set_volume(uint8_t percent)
{
	return wss_set_volume(percent);
}

enum player_state_t player_get_state(void)
{
	return state;
}

uint32_t player_get_seconds_played(void)
{
	return bytes_played / wav_header.byte_rate;
}

void player_task(void)
{
	size_t offset;
	size_t bytes_read;

	if (state != PLAYER_PLAYING) {
		return;
	}

	if (wss_request) {
		offset = buffer_index * PLAYER_SINGLE_BUFFER_SIZE;

		bytes_read = fread(&buffer.data[offset], 1, PLAYER_SINGLE_BUFFER_SIZE, fd);
		bytes_played += bytes_read;
		if (bytes_read == 0) {
			player_stop();
		}

		buffer_index ^= 1;
		wss_request = false;
	}
}
