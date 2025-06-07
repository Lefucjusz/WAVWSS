#ifndef __IRQ_H__
#define __IRQ_H__

#include "stdint.h"

#define IRQ_PIC_MASK_REG 0x21
#define IRQ_PIC_STATUS_REG 0x20
#define IRQ_PIC_END_OF_IRQ 0x20

void irq_init(uint8_t irq_num, void interrupt (*isr)(void));
void irq_release(uint8_t irq_num);

#endif