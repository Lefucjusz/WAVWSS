#ifndef __WSS_H__
#define __WSS_H__

#include "stdint.h"
#include "wssdefs.h"

struct wss_playback_cfg_t
{
	uint32_t buffer_size;
	uint32_t sample_rate;
	uint8_t bit_depth;
	uint8_t channels_num;
};

/* Direct registers access */
uint8_t wss_read_direct(uint16_t offset);
void wss_write_direct(uint16_t offset, uint8_t data);

/* Indirect registers access */
int wss_read_indirect(uint8_t addr, uint8_t *data);
int wss_write_indirect(uint8_t addr, uint8_t data);

/* Initialize - read config from ULTRA16 variable or set defaults */
void wss_init(void);

/* Playback control */
int wss_playback_start(const struct wss_playback_cfg_t *cfg);
int wss_playback_stop(void);
int wss_playback_continue(void);

/* Setters */
int wss_set_volume(uint8_t percent);

/* Getters */
uint16_t wss_get_base_address(void);
uint8_t wss_get_irq_number(void);
uint8_t wss_get_dma_channel(void);

#endif
