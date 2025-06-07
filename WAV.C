#include "wav.h"
#include <string.h>

#define WAV_DATA_MARKER "data"
#define WAV_DATA_MARKER_LENGTH 4
#define WAV_DATA_CHUNK_LENGTH_SIZE sizeof(uint32_t) // WAV data length is stored as uint32_t

/* Assume that no WAV header is longer than this value (that isn't necessarily true...) */
#define WAV_DATA_MAX_HEADER_SIZE 512

uint32_t wav_skip_header(FILE *fd)
{
    char header[WAV_DATA_MAX_HEADER_SIZE];
    uint32_t pcm_offset;
    uint32_t file_size;
    uint16_t offset;

    /* Sanity checks */
    if (fd == NULL) {
        return 0;
    }

    /* Get total file size */
    fseek(fd, 0, SEEK_END);
    file_size = ftell(fd);
    rewind(fd);

    /* Read the header */
    fread(header, sizeof(*header), sizeof(header), fd);

    /* Search for data marker - strstr can't be used, as the header might contain null characters */
    for (offset = 0; offset < (WAV_DATA_MAX_HEADER_SIZE - WAV_DATA_MARKER_LENGTH); ++offset) {
        if (memcmp(&header[offset], WAV_DATA_MARKER, WAV_DATA_MARKER_LENGTH) == 0) {
            break;
        }
    }
    if (offset == (WAV_DATA_MAX_HEADER_SIZE - WAV_DATA_MARKER_LENGTH)) {
        return 0; // Data chunk not found
    }

    /* Compute position of the beginning of PCM data and seek to that position */
    pcm_offset = offset + WAV_DATA_MARKER_LENGTH + WAV_DATA_CHUNK_LENGTH_SIZE;
    fseek(fd, pcm_offset, SEEK_SET);

    /* Return size of PCM data */
    return file_size - pcm_offset;
}
