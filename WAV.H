#ifndef __WAV_H__
#define __WAV_H__

#include <stdio.h>
#include "stdint.h"

uint32_t wav_skip_header(FILE *fd);

#endif