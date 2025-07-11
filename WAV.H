#ifndef __WAV_H__
#define __WAV_H__

#include <stdio.h>
#include "stdint.h"

struct wav_header_t
{
	char riff_header[4];
	uint32_t wav_size;
	char wave_header[4];
	char fmt_header[4];
	uint32_t fmt_chunk_size;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t sample_align;
	uint16_t bit_depth;
	char data_header[4];
	uint32_t data_size;
};

int wav_parse_header(FILE *fd, struct wav_header_t *header);

#endif
