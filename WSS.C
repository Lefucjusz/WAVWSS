#include "wss.h"
#include "wav.h"
#include "utils.h"
#include <errno.h>
#include <dos.h>
#include <stddef.h>
#include <stdlib.h>

#define WSS_TIMEOUT_LOOPS 10000
#define WSS_BUFFERS_NUM 2 // Double buffering - generate interrupt when each half of buffer transferred
#define WSS_DAC_ATTEN_LIMIT (WSS_DAC_ATTEN_MAX_VAL * 2 / 3) // Reduce the range as the lowest value are very quiet

/* Default WSS I/O config */
#define WSS_DEFAULT_BASE 0x530
#define WSS_DEFAULT_IRQ 5
#define WSS_DEFAULT_DMA 1

struct wss_sample_rate_map_t
{
	uint32_t sample_rate;
	uint8_t css;
	uint8_t cfs;
};

static uint16_t wss_base_addr;
static uint8_t wss_irq_num;
static uint8_t wss_dma_ch;

/* WSS supports more, but these are the most popular */
static const struct wss_sample_rate_map_t sample_rate_map[] =
{
	{8000UL,	0,		0},
	{11025UL,	WSS_CSS_BIT,	WSS_CFS0_BIT},
	{16000UL,	0,		WSS_CFS0_BIT},
	{22050UL,	WSS_CSS_BIT,	WSS_CFS1_BIT | WSS_CFS0_BIT},
	{32000UL,	0,		WSS_CFS1_BIT | WSS_CFS0_BIT},
	{44100UL,	WSS_CSS_BIT,	WSS_CFS2_BIT | WSS_CFS0_BIT},
	{48000UL,	0,		WSS_CFS2_BIT | WSS_CFS1_BIT}
};

static const struct wss_sample_rate_map_t *wss_get_sample_rate_config(uint32_t sample_rate)
{
	uint8_t i;
	for (i = 0; i < ARRAY_COUNT(sample_rate_map); ++i) {
		if (sample_rate_map[i].sample_rate == sample_rate) {
			return &sample_rate_map[i];
		}
	}
	return NULL;
}

/* Wait until soundcard ready */
static int wss_wait(void)
{
	int err = 0;
	uint16_t timeout = WSS_TIMEOUT_LOOPS;

	while ((wss_read_direct(WSS_INDEX_REG_OFFSET) & WSS_INIT_BIT) != 0) {
		if (timeout == 0) {
			err = -EBUSY;
			break;
		}
		--timeout;
	}

	return err;
}

uint8_t wss_read_direct(uint16_t offset)
{
	return inp(wss_base_addr + offset);
}

void wss_write_direct(uint16_t offset, uint8_t data)
{
	outp(wss_base_addr + offset, data);
}

int wss_read_indirect(uint8_t addr, uint8_t *data)
{
	if (data == NULL) {
		return -EINVAL;
	}

	wss_write_direct(WSS_INDEX_REG_OFFSET, addr);
	*data = wss_read_direct(WSS_IDATA_REG_OFFSET);

	return 0;
}

int wss_write_indirect(uint8_t addr, uint8_t data)
{
	int err;

	wss_write_direct(WSS_INDEX_REG_OFFSET, addr);
	err = wss_wait();
	if (err) {
		return err;
	}

	wss_write_direct(WSS_IDATA_REG_OFFSET, data);
	err = wss_wait();
	if (err) {
		return err;
	}

	return 0;
}

void wss_init(void)
{
	const char *ultra16;
	uint16_t base_addr;
	uint16_t irq_num;
	uint16_t dma_ch;
	int parsed_items;

	/* Set default config */
	wss_base_addr = WSS_DEFAULT_BASE;
	wss_irq_num = WSS_DEFAULT_IRQ;
	wss_dma_ch = WSS_DEFAULT_DMA;

	/* Get ULTRA16 variable */
	ultra16 = getenv("ULTRA16");
	if (ultra16 == NULL) {
		printf("ULTRA16 variable not found, using defaults (base 0x%X, IRQ %u, DMA %u)\n\n", wss_base_addr, wss_irq_num, wss_dma_ch);
		return;
	}

	/* Parse variable */
	parsed_items = sscanf(ultra16, "%x,%u,%u", &base_addr, &irq_num, &dma_ch);
	if (parsed_items != 3) {
		printf("Malformed ULTRA16 variable, using defaults (base 0x%X, IRQ %u, DMA %u)\n\n", wss_base_addr, wss_irq_num, wss_dma_ch);
		return;
	}

	/* Assign parsed values */
	wss_base_addr = base_addr;
	wss_irq_num = irq_num;
	wss_dma_ch = dma_ch;
}

int wss_playback_start(const struct wss_playback_cfg_t *cfg)
{
	int err;
	uint8_t format_reg;
	uint16_t dma_count;
	const struct wss_sample_rate_map_t *sample_rate_cfg;

	/* Get sample rate config */
	sample_rate_cfg = wss_get_sample_rate_config(cfg->sample_rate);
	if (sample_rate_cfg == NULL) {
		return -EINVFMT;
	}

	/* Configure line-in */
	err = wss_write_indirect(WSS_LADC_REG, 0x00); // 0dB gain, disable left mic +20dB gain, ADC input from left line
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_RADC_REG, 0x00); // 0dB gain, disable right mic +20dB gain, ADC input from right line
	if (err) {
		return err;
	}

	/* Configure AUX 1 */
	err = wss_write_indirect(WSS_LAUX1_REG, WSS_AUX_MUTE_BIT); // 0dB gain, mute left channel
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_RAUX1_REG, WSS_AUX_MUTE_BIT); // 0dB gain, mute right channel
	if (err) {
		return err;
	}

	/* Configure AUX 2 */
	err = wss_write_indirect(WSS_LAUX2_REG, WSS_AUX_MUTE_BIT); // 0dB gain, mute left channel
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_RAUX2_REG, WSS_AUX_MUTE_BIT); // 0dB gain, mute right channel
	if (err) {
		return err;
	}

	/* Configure loopback */
	err = wss_write_indirect(WSS_MIX_REG, 0x00); // 0dB attenuation, disable loopback
	if (err) {
		return err;
	}

	/* Configure interrupt pin */
	err = wss_write_indirect(WSS_PIN_REG, WSS_IEN_BIT); // Enable interrupt pin
	if (err) {
		return err;
	}

	/* Configure DMA count */
	dma_count = (cfg->buffer_size / (BITS_TO_BYTES(cfg->bit_depth) * cfg->channels_num * WSS_BUFFERS_NUM)) - 1;
	err = wss_write_indirect(WSS_PLAYBACK_LCNT_REG, LO_BYTE(dma_count));
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_PLAYBACK_UCNT_REG, HI_BYTE(dma_count));
	if (err) {
		return err;
	}

	/* Configure data format */
	format_reg = sample_rate_cfg->cfs | sample_rate_cfg->css;
	if (cfg->bit_depth == 16) {
		format_reg |= WSS_FMT_BIT;
	}
	if (cfg->channels_num == 2) {
		format_reg |= WSS_S_M_BIT;
	}
	err = wss_write_indirect(WSS_FORMAT_REG | WSS_MCE_BIT, format_reg);
	if (err) {
		return err;
	}

	/* Configure interface */
	err = wss_write_indirect(WSS_CONFIG_REG | WSS_MCE_BIT, WSS_PEN_BIT | WSS_ACAL_BIT); // Enable playback, perform calibration
	if (err) {
		return err;
	}

	/* Wait until calibration done */
	wss_write_direct(WSS_INDEX_REG_OFFSET, WSS_TEST_INIT_REG); // Deassert MCE, select init register
	while ((wss_read_direct(WSS_IDATA_REG_OFFSET) & WSS_ACI_BIT) != 0) {
		continue;
	}

	return 0;
}

int wss_playback_stop(void)
{
	int err;
	uint8_t reg;

	/* Read config register */
	err = wss_read_indirect(WSS_CONFIG_REG, &reg);
	if (err) {
		return err;
	}

	/* Stop playback */
	reg &= ~WSS_PEN_BIT;
	err = wss_write_indirect(WSS_CONFIG_REG, reg);
	if (err) {
		return err;
	}

	return 0;
}


int wss_playback_continue(void)
{
	int err;
	uint8_t reg;

	/* Read config register */
	err = wss_read_indirect(WSS_CONFIG_REG, &reg);
	if (err) {
		return err;
	}

	/* Start playback */
	reg |= WSS_PEN_BIT;
	err = wss_write_indirect(WSS_CONFIG_REG, reg);
	if (err) {
		return err;
	}

	return 0;
}

int wss_set_volume(uint8_t percent)
{
	int err;
	uint8_t reg_val;

	/* Sanity check */
	if (percent > PERCENT_MAX) {
		return -EINVAL;
	}

	/* Map percent value to register value, handle mute case separately */
	if (percent == 0) {
		reg_val = WSS_DAC_MUTE_BIT;
	}
	else {
		reg_val = MAP(percent, PERCENT_MIN, PERCENT_MAX, WSS_DAC_ATTEN_LIMIT, 0);
		if (reg_val > WSS_DAC_ATTEN_MAX_VAL) {
			reg_val = WSS_DAC_ATTEN_MAX_VAL;
		}
	}

	/* Write to sound card */
	err = wss_write_indirect(WSS_LDAC_REG, reg_val);
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_RDAC_REG, reg_val);
	if (err) {
		return err;
	}

	return 0;
}

uint16_t wss_get_base_address(void)
{
	return wss_base_addr;
}

uint8_t wss_get_irq_number(void)
{
	return wss_irq_num;
}

uint8_t wss_get_dma_channel(void)
{
	return wss_dma_ch;
}
