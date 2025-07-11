#include "wav.h"
#include <string.h>
#include <errno.h>

#define WAV_RIFF_HEADER "RIFF"
#define WAV_WAVE_HEADER "WAVE"
#define WAV_FMT_HEADER "fmt "
#define WAV_DATA_HEADER "data"

/* Assume that no WAV header is longer than this value (that isn't necessarily true...) */
#define WAV_DATA_MAX_HEADER_SIZE 512

static uint32_t wav_search_data_chunk(const uint8_t *buffer, struct wav_header_t *header)
{
	const uint16_t max_offset = WAV_DATA_MAX_HEADER_SIZE - sizeof(header->data_header);
	uint16_t offset;
	uint32_t data_offset;

	/* Search for data header */
	for (offset = 0; offset < max_offset; ++offset) {
		if (memcmp(&buffer[offset], WAV_DATA_HEADER, sizeof(header->data_header)) == 0) {
			/* Data header found, update fields in WAV header */
			memcpy(header->data_header, WAV_DATA_HEADER, sizeof(header->data_header));
			data_offset = offset + sizeof(header->data_header) + sizeof(header->data_size);
			header->data_size = header->wav_size - data_offset;

			return data_offset;
		}
	}

	/* Data marker not found */
	return 0;
}

int wav_parse_header(FILE *fd, struct wav_header_t *header)
{
	uint8_t buffer[WAV_DATA_MAX_HEADER_SIZE];
	uint32_t data_offset;

	/* Sanity checks */
	if ((fd == NULL) || (header == NULL)) {
		return -EINVAL;
	}

	/* Read the header from file */
	fread(buffer, sizeof(*buffer), sizeof(buffer), fd);

	/* Load what should be a WAV header to the struct */
	memcpy(header, buffer, sizeof(*header));

	/* Check if a valid WAV file */
	if ((memcmp(header->riff_header, WAV_RIFF_HEADER, sizeof(header->riff_header)) != 0) ||
		(memcmp(header->wave_header, WAV_WAVE_HEADER, sizeof(header->wave_header)) != 0) ||
		(memcmp(header->fmt_header, WAV_FMT_HEADER, sizeof(header->fmt_header)) != 0)) {
		return -EINVFMT;
	}

	/* Check if 16-bit PCM. TODO add 8-bit PCM support */
	if (header->bit_depth != 16) {
		return -EINVFMT;
	}

	/* Check if data header is valid. If it's not, we're probably dealing
	 * with extended WAV header format containing metadata and we need to
	 * manually search for the data chunk. */
	if (memcmp(header->data_header, WAV_DATA_HEADER, sizeof(header->data_header)) != 0) {
		data_offset = wav_search_data_chunk(buffer, header);
		if (data_offset == 0) {
			return -EINVFMT;
		}
	}
	else {
		data_offset = sizeof(*header);
	}

	/* Seek to the beginning of PCM data */
	fseek(fd, data_offset, SEEK_SET);

	return 0;
}
