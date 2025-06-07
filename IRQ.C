#include "irq.h"
#include <dos.h>

/* Hardware IRQ0 is at INT8 */
#define IRQ_HW_INTERRUPTS_OFFSET 0x08

static void interrupt (*old_isr)(void);

void irq_init(uint8_t irq_num, void interrupt (*isr)(void))
{
	uint8_t reg;

	/* Store current handler */
	old_isr = getvect(irq_num + IRQ_HW_INTERRUPTS_OFFSET);

	/* Install new handler */
	setvect(irq_num + IRQ_HW_INTERRUPTS_OFFSET, isr);

	/* Unmask the interrupt */
	reg = inp(IRQ_PIC_MASK_REG);
	reg &= ~(1 << irq_num);
	outp(IRQ_PIC_MASK_REG, reg);
}

void irq_release(uint8_t irq_num)
{
	uint8_t reg;

	/* Mask the interrupt */
	reg = inp(IRQ_PIC_MASK_REG);
	reg |= (1 << irq_num);
	outp(IRQ_PIC_MASK_REG, reg);

	/* Restore original handler */
	setvect(irq_num + IRQ_HW_INTERRUPTS_OFFSET, old_isr);
}
