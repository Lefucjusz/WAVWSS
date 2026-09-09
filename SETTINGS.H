#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "stdint.h"

int settings_init(const char *exe_path);
int settings_deinit(void);

uint32_t settings_get_played_seconds(void);
void settings_add_played_second(void);

uint8_t settings_get_volume(void);
void settings_set_volume(uint8_t volume);

int settings_sync(void);

#endif
