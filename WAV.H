#ifndef __WAV_H__
#define __WAV_H__

#include <stdio.h>
#include "stdint.h"

/* TODO detect those dynamically */
#define WAV_SAMPLE_RATE_HZ 44100
#define WAV_SAMPLE_SIZE_BYTES sizeof(int16_t)
#define WAV_CHANNELS_NUM 2

#define WAV_BYTES_PER_SECOND (WAV_SAMPLE_RATE_HZ * WAV_SAMPLE_SIZE_BYTES * WAV_CHANNELS_NUM)

uint32_t wav_skip_header(FILE *fd);

#endif
