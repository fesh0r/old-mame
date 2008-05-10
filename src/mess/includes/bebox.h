/*****************************************************************************
 *
 * includes/bebox.h
 * 
 * BeBox
 *
 ****************************************************************************/

#ifndef BEBOX_H_
#define BEBOX_H_

#include "machine/ins8250.h"

/*----------- defined in machine/bebox.c -----------*/

extern const struct pit8253_config bebox_pit8254_config;
extern const struct dma8237_interface bebox_dma8237_1_config;
extern const struct dma8237_interface bebox_dma8237_2_config;
extern const struct pic8259_interface bebox_pic8259_master_config;
extern const struct pic8259_interface bebox_pic8259_slave_config;
extern const ins8250_interface bebox_uart_inteface[4];

MACHINE_START( bebox );
MACHINE_RESET( bebox );
DRIVER_INIT( bebox );
NVRAM_HANDLER( bebox );

READ64_HANDLER( bebox_cpu0_imask_r );
READ64_HANDLER( bebox_cpu1_imask_r );
READ64_HANDLER( bebox_interrupt_sources_r );
READ64_HANDLER( bebox_crossproc_interrupts_r );
READ64_HANDLER( bebox_800001F0_r );
READ64_HANDLER( bebox_800003F0_r );
READ64_HANDLER( bebox_interrupt_ack_r );
READ64_HANDLER( bebox_page_r );
READ64_HANDLER( bebox_80000480_r );
READ64_HANDLER( bebox_flash_r );

WRITE64_HANDLER( bebox_cpu0_imask_w );
WRITE64_HANDLER( bebox_cpu1_imask_w );
WRITE64_HANDLER( bebox_crossproc_interrupts_w );
WRITE64_HANDLER( bebox_processor_resets_w );
WRITE64_HANDLER( bebox_800001F0_w );
WRITE64_HANDLER( bebox_800003F0_w );
WRITE64_HANDLER( bebox_page_w );
WRITE64_HANDLER( bebox_80000480_w );
WRITE64_HANDLER( bebox_flash_w );

void bebox_ide_interrupt(int state);


#endif /* BEBOX_H_ */
