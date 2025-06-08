#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "stdint.h"

#define BUFFER_SIZE_BYTES 32768

struct buffer_t
{
    uint8_t *data;
    uint8_t page;
    uint16_t offset;
};

int buffer_allocate(struct buffer_t *buffer);
void buffer_free(struct buffer_t *buffer);

#endif
