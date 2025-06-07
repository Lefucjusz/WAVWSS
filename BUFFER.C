#include "buffer.h"
#include "errno.h"
#include "stdint.h"
#include <dos.h>
#include <stdlib.h>

static uint32_t buffer_to_linear(const void *buffer)
{
	return ((uint32_t)FP_SEG(buffer) << 4) + FP_OFF(buffer);
}

int buffer_allocate(struct buffer_t *buffer)
{
	uint8_t *temp_buffer;
	uint32_t linear_address;
	uint16_t start_page, end_page;

	/* Sanity check */
	if (buffer == NULL) {
		return -EINVAL;
	}

	/* Allocate new buffer */
	temp_buffer = malloc(BUFFER_SIZE_BYTES);
	if (temp_buffer == NULL) {
		return -ENOMEM;
	}

	/* Check if the entire buffer is within a 64KiB DMA page. If not,
	 * allocate new buffer, which for sure won't cross page boundary,
	 * as it will be allocated right next to the one we just checked. */
	linear_address = buffer_to_linear(temp_buffer);
	start_page = linear_address >> 16;
	end_page = (linear_address + BUFFER_SIZE_BYTES - 1) >> 16;
	if (start_page == end_page) {
		buffer->data = temp_buffer;
	}
	else {
		buffer->data = malloc(BUFFER_SIZE_BYTES);
		free(temp_buffer);
	}

	/* Compute page and offset */
	linear_address = buffer_to_linear(buffer->data);
	buffer->page = linear_address >> 16;
	buffer->offset = linear_address & 0xFFFF;

	return 0;
}

void buffer_free(const struct buffer_t *buffer)
{
	if (buffer == NULL) {
		return;
	}

	free(buffer->data);
}
