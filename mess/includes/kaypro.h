/******************************************************************************
 *
 *	kaypro.h
 *
 *	interface for Kaypro 2x
 *
 *	Juergen Buchmueller, July 1998
 *
 ******************************************************************************/

#include "includes/wd179x.h"
#include "machine/cpm_bios.h"

extern int kaypro_floppy_init(int id);
extern void init_kaypro(void);
extern MACHINE_INIT( kaypro );
extern MACHINE_STOP( kaypro );

extern INTERRUPT_GEN( kaypro_interrupt );

#define KAYPRO_FONT_W 	8
#define KAYPRO_FONT_H 	16
#define KAYPRO_SCREEN_W	80
#define KAYPRO_SCREEN_H   25

extern VIDEO_START( kaypro );
extern VIDEO_UPDATE( kaypro );

extern READ_HANDLER ( kaypro_const_r );
extern WRITE_HANDLER ( kaypro_const_w );
extern READ_HANDLER ( kaypro_conin_r );
extern WRITE_HANDLER ( kaypro_conin_w );
extern READ_HANDLER ( kaypro_conout_r );
extern WRITE_HANDLER ( kaypro_conout_w );

extern int	kaypro_sh_start(const struct MachineSound *msound);
extern void kaypro_sh_stop(void);
extern void kaypro_sh_update(void);

extern void kaypro_bell(void);
extern void kaypro_click(void);
