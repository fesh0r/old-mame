#ifndef __OSI__
#define __OSI__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"m6502"
#define CASSETTE_TAG	"cassette"

#define X1			3932160
#define UK101_X1	XTAL_8MHz

#define OSI600_VIDEORAM_SIZE	0x400
#define OSI630_COLORRAM_SIZE	0x400

class osi_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, osi_state(machine)); }

	osi_state(running_machine &machine) { }

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	int _32;
	int coloren;
	UINT8 *video_ram;
	UINT8 *color_ram;

	/* floppy state */
	int fdc_index;

	/* devices */
	running_device *cassette;
};

/* ---------- defined in video/osi.c ---------- */

MACHINE_DRIVER_EXTERN( osi600_video );
MACHINE_DRIVER_EXTERN( uk101_video );
MACHINE_DRIVER_EXTERN( osi630_video );

#endif
