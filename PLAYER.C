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
static uint32_t bytes_total;
static struct wav_header_t wav_header;
static FILE *fd;

static void interrupt player_irq_handler(void)
{
	wss_request = true;
	outp(WSS_STATUS_REG, 0x00); // Clear WSS interrupt bit
	outp(IRQ_PIC_STATUS_REG, IRQ_PIC_END_OF_IRQ); // Acknowledge interrupt
}

int player_init(void)
{
	int err;

	/* Set initial state */
	state = PLAYER_STOPPED;

	/* Allocate PCM buffer */
	err = buffer_allocate(&buffer);
	if (err) {
		return -ENOMEM;
	}

	/* Initialize IRQ handler */
	irq_init(WSS_IRQ_NUM, player_irq_handler);

	return 0;
}

void player_deinit(void)
{
	/* Stop player */
	player_stop();

	/* Release resources */
	dma_release(WSS_DMA_CHANNEL);
	irq_release(WSS_IRQ_NUM);

	/* Free PCM buffer */
	buffer_free(&buffer);
}

int player_start(const char *path)
{
	int err;
	uint32_t bytes_read;

	/* Stopped player if not stopped yet */
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
	bytes_read = fread(buffer.data, sizeof(*buffer.data), BUFFER_SIZE_BYTES, fd);
	bytes_played += bytes_read;

	/* Start DMA */
	dma_autoinit_start(WSS_DMA_CHANNEL, buffer.page, buffer.offset, BUFFER_SIZE_BYTES);

	/* Start playback */
	err = wss_playback_start(BUFFER_SIZE_BYTES, wav_header.sample_rate, wav_header.num_channels);
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

enum player_state_t player_get_state(void)
{
	return state;
}

uint32_t player_get_seconds_played(void)
{
	return bytes_played / wav_header.byte_rate;
}

uint32_t player_get_seconds_total(void)
{
	return bytes_total / wav_header.byte_rate;
}

void player_task(void)
{
	uint32_t offset;
	uint32_t bytes_read;

	if (state != PLAYER_PLAYING) {
		return;
	}

	if (wss_request) {
		offset = buffer_index * PLAYER_SINGLE_BUFFER_SIZE;

		bytes_read = fread(&buffer.data[offset], sizeof(*buffer.data), PLAYER_SINGLE_BUFFER_SIZE, fd);
		bytes_played += bytes_read;
		if (bytes_read == 0) {
			player_stop();
		}

		buffer_index ^= 1;
		wss_request = false;
	}
}
