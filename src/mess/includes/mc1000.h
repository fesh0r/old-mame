#ifndef __MC1000__
#define __MC1000__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"u13"
#define AY8910_TAG		"u21"
#define MC6845_TAG		"mc6845"
#define MC6847_TAG		"u19"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define MC1000_MC6845_VIDEORAM_SIZE		0x800
#define MC1000_MC6847_VIDEORAM_SIZE		0x1800

class mc1000_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, mc1000_state(machine)); }

	mc1000_state(running_machine &machine) { }

	/* cpu state */
	int ne555_int;

	/* memory state */
	int rom0000;
	int mc6845_bank;
	int mc6847_bank;

	/* keyboard state */
	int keylatch;

	/* video state */
	int hsync;
	int vsync;
	UINT8 *mc6845_video_ram;
	UINT8 *mc6847_video_ram;
	UINT8 mc6847_attr;

	/* devices */
	running_device *mc6845;
	running_device *mc6847;
	running_device *centronics;
	running_device *cassette;
};

#endif
