#include "wss.h"
#include "wav.h"
#include "utils.h"
#include "errno.h"
#include <dos.h>
#include <stddef.h>

#define WSS_TIMEOUT_LOOPS 10000
#define WSS_BUFFERS_NUM 2 // Double buffering - generate interrupt when each half of buffer transferred

#define WSS_BYTES_TO_DMA_COUNT(x) (((x) / (WAV_SAMPLE_SIZE_BYTES * WAV_CHANNELS_NUM * WSS_BUFFERS_NUM)) - 1)

/* Wait until soundcard ready */
static int wss_wait(void)
{
	int err = 0;
	uint16_t timeout = WSS_TIMEOUT_LOOPS;

	while ((wss_read_direct(WSS_INDEX_REG) & WSS_INIT_BIT) != 0) {
		if (timeout == 0) {
			err = -ETIMEDOUT;
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

int wss_playback_start(uint32_t buffer_size)
{
	int err;

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
	err = wss_write_indirect(WSS_RDAC_REG, 0x00);  // 0dB attenuation, unmute right channel
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
	err = wss_write_indirect(WSS_PLAYBACK_LCNT_REG, LO_BYTE(WSS_BYTES_TO_DMA_COUNT(buffer_size)));
	if (err) {
		return err;
	}
	err = wss_write_indirect(WSS_PLAYBACK_UCNT_REG, HI_BYTE(WSS_BYTES_TO_DMA_COUNT(buffer_size)));
	if (err) {
		return err;
	}

	/* Configure data format */
	err = wss_write_indirect(WSS_FORMAT_REG | WSS_MCE_BIT, WSS_FMT_BIT | WSS_S_M_BIT | WSS_CFS2_BIT | WSS_CFS0_BIT | WSS_CSS_BIT);  // 16-bit stereo signed PCM, 44.1kHz
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
