#ifndef __WSS_H__
#define __WSS_H__

#include "stdint.h"
#include "wssdefs.h"

/* Direct registers access */
uint8_t wss_read_direct(uint16_t addr);
void wss_write_direct(uint16_t addr, uint8_t data);

/* Indirect registers access */
int wss_read_indirect(uint8_t addr, uint8_t *data);
int wss_write_indirect(uint8_t addr, uint8_t data);

/* Playback control */
int wss_playback_start(uint32_t buffer_size, uint32_t sample_rate, uint8_t channels);
int wss_playback_stop(void);
int wss_playback_continue(void);

#endif
