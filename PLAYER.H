#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "stdint.h"

enum player_state_t
{
	PLAYER_STOPPED,
	PLAYER_PAUSED,
	PLAYER_PLAYING
};

int player_init(void);
void player_deinit(void);

int player_start(const char *path);
int player_pause(void);
int player_resume(void);
int player_stop(void);

int player_set_volume(uint8_t percent);

enum player_state_t player_get_state(void);
uint32_t player_get_seconds_played(void);
uint32_t player_get_seconds_total(void);

void player_task(void);

#endif