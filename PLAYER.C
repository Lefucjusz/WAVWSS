#include "player.h"
#include "wss.h"
#include "wav.h"
#include "irq.h"
#include "buffer.h"
#include "stdbool.h"
#include "utils.h"
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
static size_t pcm_data_offset;
static struct wav_header_t wav_header;
static struct wss_playback_cfg_t playback_cfg;
static FILE *fd;

static void interrupt player_irq_handler(void)
{
	wss_request = true;
	wss_write_direct(WSS_STATUS_REG_OFFSET, 0x00); // Clear WSS interrupt bit
	outp(IRQ_PIC_STATUS_REG, IRQ_PIC_END_OF_IRQ); // Acknowledge interrupt
}

static size_t player_fill_buffer(uint8_t *buffer, size_t size)
{
    size_t bytes_read;

    bytes_read = fread(buffer, 1, size, fd);
    if (bytes_read < size) {
        memset(&buffer[bytes_read], 0, size - bytes_read);
    }

    return bytes_read;
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

	/* Skip WAV header and get file info */
	err = wav_parse_header(fd, &wav_header, &pcm_data_offset);
	if (err) {
		goto out_error;
	}

	/* Preload buffer */
	player_fill_buffer(buffer.data, BUFFER_SIZE_BYTES);

	/* Configure WSS */
	playback_cfg.buffer_size = BUFFER_SIZE_BYTES;
	playback_cfg.sample_rate = wav_header.sample_rate;
	playback_cfg.bit_depth = wav_header.bit_depth;
	playback_cfg.channels_num = wav_header.num_channels;
	err = wss_playback_configure(&playback_cfg);
	if (err) {
		goto out_error;
	}

	/* Start DMA */
	dma_autoinit_start(wss_get_dma_channel(), buffer.page, buffer.offset, BUFFER_SIZE_BYTES);

	/* Start playback */
	err = wss_playback_start();
	if (err) {
		goto out_error;
	}

	state = PLAYER_PLAYING;

out_error:
	if (err) {
		fclose(fd);
		fd = NULL;
	}

	return err;
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

	err = wss_playback_start();
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

int player_seek_relative(int32_t seconds)
{
	int32_t offset_bytes;
	uint32_t target_bytes;
	uint32_t frame_size;
	int err;

	/* Sanity check */
	if ((state != PLAYER_PLAYING) && (state != PLAYER_PAUSED)) {
		return -EINVAL;
	}

	/* Stop WSS playback and release DMA */
	err = wss_playback_stop();
	if (err) {
		return err;
	}
	dma_release(wss_get_dma_channel());

	/* Compute audio frame size */
	frame_size = wav_header.num_channels * BITS_TO_BYTES(wav_header.bit_depth);

	/* Convert seconds to bytes */
	offset_bytes = seconds * frame_size * wav_header.sample_rate;

	/* Compute new position in file */
	target_bytes = CLAMP((int32_t)bytes_played + offset_bytes, 0, (int32_t)wav_header.data_size);
	target_bytes -= target_bytes % frame_size; // Align to full frame

	/* Seek to new position */
	err = fseek(fd, pcm_data_offset + target_bytes, SEEK_SET);
	if (err) {
		return err;
	}

	/* Update played size */
	bytes_played = target_bytes;

	/* Load fresh data to buffer */
	player_fill_buffer(buffer.data, BUFFER_SIZE_BYTES);

	/* Restart playback */
	wss_request = false;
	buffer_index = 0;
	err = wss_playback_configure(&playback_cfg);
	if (err) {
		return err;
	}
	dma_autoinit_start(wss_get_dma_channel(), buffer.page, buffer.offset, BUFFER_SIZE_BYTES);
	if (state == PLAYER_PLAYING) {
		err = wss_playback_start();
		if (err) {
			return err;
		}
	}

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

void player_task(void)
{
	size_t offset;
	size_t bytes_read;

	if (state != PLAYER_PLAYING) {
		return;
	}

	if (wss_request) {
		bytes_played += PLAYER_SINGLE_BUFFER_SIZE;
		offset = buffer_index * PLAYER_SINGLE_BUFFER_SIZE;

		bytes_read = player_fill_buffer(&buffer.data[offset], PLAYER_SINGLE_BUFFER_SIZE);
		if (bytes_read == 0) {
			player_stop();
		}

		buffer_index ^= 1;
		wss_request = false;
	}
}
