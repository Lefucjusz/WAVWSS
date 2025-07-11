#ifndef __WSSDEFS_H__
#define __WSSDEFS_H__

/* TODO read this dynamically or allow to configure it */
#define WSS_BASE 0x530
#define WSS_DMA_CHANNEL 3
#define WSS_IRQ_NUM 7

/* NOTE: All of the definitions below were obtained from AD1848 datasheet */

/* Direct registers definitions */
#define WSS_INDEX_REG (WSS_BASE + 4)
#define WSS_IDATA_REG (WSS_BASE + 5)
#define WSS_STATUS_REG (WSS_BASE + 6)

/* Indirect registers definitions */
#define WSS_LADC_REG 0x00
#define WSS_RADC_REG 0x01
#define WSS_LAUX1_REG 0x02
#define WSS_RAUX1_REG 0x03
#define WSS_LAUX2_REG 0x04
#define WSS_RAUX2_REG 0x05
#define WSS_LDAC_REG 0x06
#define WSS_RDAC_REG 0x07
#define WSS_FORMAT_REG 0x08
#define WSS_CONFIG_REG 0x09
#define WSS_PIN_REG 0x0A
#define WSS_TEST_INIT_REG 0x0B
#define WSS_MISC_REG 0x0C
#define WSS_MIX_REG 0x0D
#define WSS_PLAYBACK_UCNT_REG 0x0E
#define WSS_PLAYBACK_LCNT_REG 0x0F

/* Direct registers bit definitions */
/* WSS_INDEX_REG bits */
#define WSS_INIT_BIT (1 << 7)
#define WSS_MCE_BIT (1 << 6)
#define WSS_TRD_BIT (1 << 5)

/* Indirect registers bit definitions */
/* WSS_LAUX1_REG, WSS_RAUX1_REG, WSS_LAUX2_REG & WSS_RAUX2_REG bits */
#define WSS_AUX_MUTE_BIT (1 << 7)

/* WSS_FORMAT_REG bits */
#define WSS_FMT_BIT (1 << 6)
#define WSS_L_C_BIT (1 << 5)
#define WSS_S_M_BIT (1 << 4)
#define WSS_CFS2_BIT (1 << 3)
#define WSS_CFS1_BIT (1 << 2)
#define WSS_CFS0_BIT (1 << 1)
#define WSS_CSS_BIT (1 << 0)

/* WSS_CONFIG_REG bits */
#define WSS_CPIO_BIT (1 << 7)
#define WSS_PPIO_BIT (1 << 6)
#define WSS_ACAL_BIT (1 << 3)
#define WSS_SDC_BIT (1 << 2)
#define WSS_CEN_BIT (1 << 1)
#define WSS_PEN_BIT (1 << 0)

/* WSS_PIN_REG bits */
#define WSS_XCTL1_BIT (1 << 7)
#define WSS_XCTL0_BIT (1 << 6)
#define WSS_IEN_BIT (1 << 1)

/* WSS_TEST_INIT_REG bits */
#define WSS_COR_BIT (1 << 7)
#define WSS_PUR_BIT (1 << 6)
#define WSS_ACI_BIT (1 << 5)
#define WSS_DRS_BIT (1 << 4)

#endif
