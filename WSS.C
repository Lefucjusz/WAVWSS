#include "wss.h"
#include "wav.h"
#include "utils.h"
#include <errno.h>
#include <dos.h>
#include <stddef.h>

#define WSS_TIMEOUT_LOOPS 10000
#define WSS_BUFFERS_NUM 2 // Double buffering - generate interrupt when each half of buffer transferred
#define WSS_SAMPLE_SIZE_BYTES sizeof(int16_t) // TODO this should be autodetected too

struct wss_config_map_t
{
	uint32_t sample_rate;
	uint8_t css;
	uint8_t cfs;
};

/* WSS supports more, but these are the most popular */
static const struct wss_config_map_t config_map[] =
{
	{8000UL,	0,		0},
	{11025UL,	WSS_CSS_BIT,	WSS_CFS0_BIT},
	{16000UL,	0,		WSS_CFS0_BIT},
	{22050UL,	WSS_CSS_BIT,	WSS_CFS1_BIT | WSS_CFS0_BIT},
	{32000UL,	0,		WSS_CFS1_BIT | WSS_CFS0_BIT},
	{44100UL,	WSS_CSS_BIT,	WSS_CFS2_BIT | WSS_CFS0_BIT},
	{48000UL,	0,		WSS_CFS2_BIT | WSS_CFS1_BIT}
};

static const struct wss_config_map_t *wss_get_sample_rate_config(uint32_t sample_rate)
{
	uint8_t i;
	for (i = 0; i < ARRAY_COUNT(config_map); ++i) {
		if (config_map[i].sample_rate == sample_rate) {
			return &config_map[i];
		}
	}
	return NULL;
}

/* Wait until soundcard ready */
static int wss_wait(void)
{
	int err = 0;
	uint16_t timeout = WSS_TIMEOUT_LOOPS;

	while ((wss_read_direct(WSS_INDEX_REG) & WSS_INIT_BIT) != 0) {
		if (timeout == 0) {
			err = -EBUSY;
			break;
		}
		--timeout;
	}

	return err;
}

uint8_t wss_read_direct(uint16_t addr)
{
	return inp(addr);
}

void wss_write_direct(uint16_t addr, uint8_t data)
{
	outp(addr, data);
}

int wss_read_indirect(uint8_t addr, uint8_t *data)
{
	if (data == NULL) {
		return -EINVAL;
	}

	outp(WSS_INDEX_REG, addr);
	*data = inp(WSS_IDATA_REG);

	return 0;
}

int wss_write_indirect(uint8_t addr, uint8_t data)
{
	int err;

	outp(WSS_INDEX_REG, addr);
	err = wss_wait();
	if (err) {
		return err;
	}

	outp(WSS_IDATA_REG, data);
	err = wss_wait();
	if (err) {
		return err;
	}

	return 0;
}

int wss_playback_start(uint32_t buffer_size, uint32_t sample_rate, uint8_t channels)
{
	int err;
	uint8_t format_reg;
	uint16_t dma_count;
	const struct wss_config_map_t *config;

	/* Get sample rate config */
	config = wss_get_sample_rate_config(sample_rate);
	if (config == NULL) {
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

	/* Configure DAC */
	err = wss_write_indirect(WSS_LDAC_REG, 0x00); // 0dB attenuation, unmute left channel
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_RDAC_REG, 0x00); // 0dB attenuation, unmute right channel
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
	dma_count = (buffer_size / (WSS_SAMPLE_SIZE_BYTES * channels * WSS_BUFFERS_NUM)) - 1;
	err = wss_write_indirect(WSS_PLAYBACK_LCNT_REG, LO_BYTE(dma_count));
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_PLAYBACK_UCNT_REG, HI_BYTE(dma_count));
	if (err) {
		return err;
	}

	/* Configure data format */
	format_reg = WSS_FMT_BIT | config->cfs | config->css;
	if (channels == 2) {
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
	wss_write_direct(WSS_INDEX_REG, WSS_TEST_INIT_REG); // Deassert MCE, select init register
	while ((wss_read_direct(WSS_IDATA_REG) & WSS_ACI_BIT) != 0) {
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
