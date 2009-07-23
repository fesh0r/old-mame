/***************************************************************************
    commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#ifndef PET_H_
#define PET_H_

#include "video/mc6845.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "devices/cartslot.h"

typedef struct _pet_state pet_state;
struct _pet_state
{
	int pet_basic1; /* basic version 1 for quickloader */
	int superpet;
	int cbm8096;
};

/*----------- defined in video/pet.c -----------*/

/* call to init videodriver */
void pet_vh_init (running_machine *machine);
void pet80_vh_init (running_machine *machine);
void superpet_vh_init (running_machine *machine);
VIDEO_UPDATE( pet );
MC6845_UPDATE_ROW( pet40_update_row );
MC6845_UPDATE_ROW( pet80_update_row );
WRITE_LINE_DEVICE_HANDLER( pet_display_enable_changed );


/*----------- defined in machine/pet.c -----------*/

extern int pet_font;
extern const via6522_interface pet_via;
extern const pia6821_interface pet_pia0;
extern const pia6821_interface petb_pia0;
extern const pia6821_interface pet_pia1;

extern UINT8 *pet_memory;
extern UINT8 *pet_videoram;
extern UINT8 *superpet_memory;

WRITE8_HANDLER(cbm8096_w);
extern READ8_HANDLER(superpet_r);
extern WRITE8_HANDLER(superpet_w);

DRIVER_INIT( pet2001 );
DRIVER_INIT( pet );
DRIVER_INIT( pet80 );
DRIVER_INIT( superpet );
MACHINE_RESET( pet );
INTERRUPT_GEN( pet_frame_interrupt );

MACHINE_DRIVER_EXTERN( pet_cartslot );
MACHINE_DRIVER_EXTERN( pet4_cartslot );

#endif /* PET_H_ */
