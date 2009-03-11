/*****************************************************************************
 *
 * includes/zx.h
 *
 ****************************************************************************/

#ifndef ZX_H_
#define ZX_H_


/*----------- defined in machine/zx.c -----------*/

DRIVER_INIT( zx );
MACHINE_RESET( zx80 );
MACHINE_RESET( pow3000 );
MACHINE_RESET( pc8300 );

READ8_HANDLER( zx_ram_r );
READ8_HANDLER ( pc8300_io_r );
READ8_HANDLER ( pow3000_io_r );
READ8_HANDLER ( zx80_io_r );
WRITE8_HANDLER ( zx80_io_w );
READ8_HANDLER ( zx81_io_r );
WRITE8_HANDLER ( zx81_io_w );

/*----------- defined in video/zx.c -----------*/

VIDEO_START( zx );
VIDEO_EOF( zx );

void zx_ula_bkgnd(running_machine *machine, int color);
void zx_ula_r(running_machine *machine, int offs, const char *region, const UINT8 param);

extern emu_timer *ula_nmi;
extern int ula_irq_active;
extern int ula_nmi_active;
extern int ula_frame_vsync;
extern int ula_scanline_count;
//extern int ula_scancode_count;


#endif /* ZX_H_ */