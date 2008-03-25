/*
	tms3556.h: header file for tms3556.c
*/

READ8_HANDLER(tms3556_vram_r);
WRITE8_HANDLER(tms3556_vram_w);
READ8_HANDLER(tms3556_reg_r);
WRITE8_HANDLER(tms3556_reg_w);

extern void tms3556_init(int vram_size);
extern void tms3556_reset(void);
extern void tms3556_interrupt(running_machine *machine);

MACHINE_DRIVER_EXTERN( tms3556 );
