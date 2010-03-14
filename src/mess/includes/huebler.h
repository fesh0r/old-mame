#ifndef __HUEBLER__
#define __HUEBLER__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80CTC_TAG		"z80ctc"
#define Z80SIO_TAG		"z80sio"
#define Z80PIO1_TAG		"z80pio1"
#define Z80PIO2_TAG		"z80pio2"
#define CASSETTE_TAG	"cassette"

class amu880_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, amu880_state(machine)); }

	amu880_state(running_machine &machine) { }

	/* keyboard state */
	int key_y;
	int keylatch;
	const UINT8 *keyboard_rom;

	/* video state */
	UINT8 *video_ram;
	const UINT8 *char_rom;

	/* devices */
	running_device *cassette;
};

#endif
