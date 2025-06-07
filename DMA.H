#ifndef __DMA_H__
#define __DMA_H__

#include "stdint.h"

/* Program the DMA controller for autoinit transfer */
int dma_autoinit_start(uint8_t channel, uint8_t page, uint16_t offset, uint32_t length);

/* Release DMA channel */
int dma_release(uint8_t channel);

#endif
