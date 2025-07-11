#include "dma.h"
#include "utils.h"
#include <errno.h>
#include <dos.h>

#define DMAC_MASK_REG 0x0A
#define DMAC_MODE_REG 0x0B
#define DMAC_CLEAR_FF_REG 0x0C

#define DMAC_MASK_CHANNEL_BIT (1 << 2)

#define DMAC_MODE_AUTOINIT_READ 0x58 // 01011000

/* Port lookup tables for DMA channels 0-3 */
static const uint8_t dma_page_port[] = {0x87, 0x83, 0x81, 0x82};
static const uint8_t dma_offset_port[] = {0x00, 0x02, 0x04, 0x06};
static const uint8_t dma_count_port[] = {0x01, 0x03, 0x05, 0x07};

int dma_autoinit_start(uint8_t channel, uint8_t page, uint16_t offset, uint32_t length)
{
	/* Sanity check */
	if (channel >= ARRAY_COUNT(dma_page_port)) {
		return -EINVAL;
	}

	/* Mask the channel */
	outp(DMAC_MASK_REG, DMAC_MASK_CHANNEL_BIT | channel);

	/* Clear any executing data transfers */
	outp(DMAC_CLEAR_FF_REG, 0x00);

	/* Set single transfer mode, with address increment, auto-initialization, read from memory */
	outp(DMAC_MODE_REG, DMAC_MODE_AUTOINIT_READ | channel);

	/* Configure offset */
	outp(dma_offset_port[channel], LO_BYTE(offset));
	outp(dma_offset_port[channel], HI_BYTE(offset));

	/* Configure page */
	outp(dma_page_port[channel], page);

	/* Configure transfer size */
	outp(dma_count_port[channel], LO_BYTE(length - 1));
	outp(dma_count_port[channel], HI_BYTE(length - 1));

	/* Unmask the DMA channel */
	outp(DMAC_MASK_REG, channel);

	return 0;
}

int dma_release(uint8_t channel)
{
	/* Sanity check */
	if (channel >= ARRAY_COUNT(dma_page_port)) {
		return -EINVAL;
	}

	/* Mask the channel */
	outp(DMAC_MASK_REG, DMAC_MASK_CHANNEL_BIT | channel);

	/* Clear executing data transfers */
	outp(DMAC_CLEAR_FF_REG, 0x00);

	/* Unmask the channel */
	outp(DMAC_MASK_REG, channel);

	return 0;
}
