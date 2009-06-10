#ifndef __PC8401A__
#define __PC8401A__

#define SCREEN_TAG		"screen"
#define CRT_SCREEN_TAG	"screen2"

#define Z80_TAG			"z80"
#define PPI8255_TAG		"ppi8255"
#define UPD1990A_TAG	"upd1990a"
#define AY8910_TAG		"ay8910"
#define SED1330_TAG		"sed1330"
#define MC6845_TAG		"mc6845"
#define MSM8251_TAG		"msm8251"

#define PC8401A_CRT_VIDEORAM_SIZE	0x2000
#define PC8401A_LCD_VIDEORAM_SIZE	0x2000
#define PC8401A_LCD_VIDEORAM_MASK	0x1fff
#define PC8500_LCD_VIDEORAM_SIZE	0x4000
#define PC8500_LCD_VIDEORAM_MASK	0x3fff

typedef struct _pc8401a_state pc8401a_state;
struct _pc8401a_state
{
	/* keyboard state */
	int key_strobe;			/* key pressed */

	/* clock state */
	int rtc_data;			/* RTC data output */
	int rtc_tp;				/* RTC timing pulse output */

	/* memory state */
	UINT8 mmr;				/* memory mapping register */
	UINT32 io_addr;			/* I/O ROM address counter */

	/* video state */
	UINT8 *video_ram;		/* LCD video RAM */
	UINT8 *crt_ram;			/* CRT video RAM */

	const device_config *upd1990a;
	const device_config *sed1330;
	const device_config *mc6845;
	const device_config *lcd;
};

/* ---------- defined in video/pc8401a.c ---------- */

MACHINE_DRIVER_EXTERN( pc8401a_video );
MACHINE_DRIVER_EXTERN( pc8500_video );

#endif
