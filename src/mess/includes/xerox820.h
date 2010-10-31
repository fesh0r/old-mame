#ifndef __XEROX820__
#define __XEROX820__

#define SCREEN_TAG		"screen"

#define Z80_TAG			"u46"
#define Z80KBPIO_TAG	"u105"
#define Z80GPPIO_TAG	"u101"
#define Z80SIO_TAG		"u96"
#define Z80CTC_TAG		"u99"
#define WD1771_TAG		"u109"
#define COM8116_TAG		"u76"
#define I8086_TAG		"i8086"

#define XEROX820_VIDEORAM_SIZE	0x1000
#define XEROX820_VIDEORAM_MASK	0x0fff

class xerox820_state : public driver_device
{
public:
	xerox820_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int keydata;						/* keyboard data */

	/* video state */
	UINT8 *video_ram;					/* video RAM */
	UINT8 *char_rom;					/* character ROM */
	UINT8 scroll;						/* vertical scroll */
	UINT8 framecnt;
	int ncset2;							/* national character set */
	int vatt;							/* X120 video attribute */
	int lowlite;						/* low light attribute */
	int chrom;							/* character ROM index */

	/* floppy state */
	int fdc_irq;						/* interrupt request */
	int fdc_drq;						/* data request */
	int _8n5;							/* 5.25" / 8" drive select */
	int dsdd;							/* double sided disk detect */

	/* devices */
	running_device *kbpio;
	running_device *z80ctc;
	running_device *wd1771;
};

#endif
