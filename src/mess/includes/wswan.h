/*****************************************************************************
 *
 * includes/wswan.h
 *
 ****************************************************************************/

#ifndef WSWAN_H_
#define WSWAN_H_

#define WSWAN_TYPE_MONO 0
#define WSWAN_TYPE_COLOR 1

#define WSWAN_X_PIXELS  (28*8)
#define WSWAN_Y_PIXELS  (18*8)

#define INTERNAL_EEPROM_SIZE    1024

#include "emu.h"
#include "cpu/v30mz/v30mz.h"
#include "imagedev/cartslot.h"
#include "machine/nvram.h"


struct EEPROM
{
	UINT8   mode;       /* eeprom mode */
	UINT16  address;    /* Read/write address */
	UINT8   command;    /* Commands: 00, 01, 02, 03, 04, 08, 0C */
	UINT8   start;      /* start bit */
	UINT8   write_enabled;  /* write enabled yes/no */
	int size;       /* size of eeprom/sram area */
	UINT8   *data;      /* pointer to start of sram/eeprom data */
	UINT8   *page;      /* pointer to current sram/eeprom page */
};

struct RTC
{
	UINT8   present;    /* Is an RTC present */
	UINT8   setting;    /* Timer setting byte */
	UINT8   year;       /* Year */
	UINT8   month;      /* Month */
	UINT8   day;        /* Day */
	UINT8   day_of_week;    /* Day of the week */
	UINT8   hour;       /* Hour, high bit = 0 => AM, high bit = 1 => PM */
	UINT8   minute;     /* Minute */
	UINT8   second;     /* Second */
	UINT8   index;      /* index for reading/writing of current of alarm time */
};

struct SoundDMA
{
	UINT32  source;     /* Source address */
	UINT16  size;       /* Size */
	UINT8   enable;     /* Enabled */
};

struct VDP
{
	UINT8 layer_bg_enable;          /* Background layer on/off */
	UINT8 layer_fg_enable;          /* Foreground layer on/off */
	UINT8 sprites_enable;           /* Sprites on/off */
	UINT8 window_sprites_enable;        /* Sprite window on/off */
	UINT8 window_fg_mode;           /* 0:inside/outside, 1:??, 2:inside, 3:outside */
	UINT8 current_line;         /* Current scanline : 0-158 (159?) */
	UINT8 line_compare;         /* Line to trigger line interrupt on */
	UINT32 sprite_table_address;        /* Address of the sprite table */
	UINT8 sprite_table_buffer[512];
	UINT8 sprite_first;         /* First sprite to draw */
	UINT8 sprite_count;         /* Number of sprites to draw */
	UINT16 layer_bg_address;        /* Address of the background screen map */
	UINT16 layer_fg_address;        /* Address of the foreground screen map */
	UINT8 window_fg_left;           /* Left coordinate of foreground window */
	UINT8 window_fg_top;            /* Top coordinate of foreground window */
	UINT8 window_fg_right;          /* Right coordinate of foreground window */
	UINT8 window_fg_bottom;         /* Bottom coordinate of foreground window */
	UINT8 window_sprites_left;      /* Left coordinate of sprites window */
	UINT8 window_sprites_top;       /* Top coordinate of sprites window */
	UINT8 window_sprites_right;     /* Right coordinate of sprites window */
	UINT8 window_sprites_bottom;        /* Bottom coordinate of sprites window */
	UINT8 layer_bg_scroll_x;        /* Background layer X scroll */
	UINT8 layer_bg_scroll_y;        /* Background layer Y scroll */
	UINT8 layer_fg_scroll_x;        /* Foreground layer X scroll */
	UINT8 layer_fg_scroll_y;        /* Foreground layer Y scroll */
	UINT8 lcd_enable;           /* LCD on/off */
	UINT8 icons;                /* FIXME: What do we do with these? Maybe artwork? */
	UINT8 color_mode;           /* monochrome/color mode */
	UINT8 colors_16;            /* 4/16 colors mode */
	UINT8 tile_packed;          /* layered/packed tile mode switch */
	UINT8 timer_hblank_enable;      /* Horizontal blank interrupt on/off */
	UINT8 timer_hblank_mode;        /* Horizontal blank timer mode */
	UINT16 timer_hblank_reload;     /* Horizontal blank timer reload value */
	UINT16 timer_hblank_count;      /* Horizontal blank timer counter value */
	UINT8 timer_vblank_enable;      /* Vertical blank interrupt on/off */
	UINT8 timer_vblank_mode;        /* Vertical blank timer mode */
	UINT16 timer_vblank_reload;     /* Vertical blank timer reload value */
	UINT16 timer_vblank_count;      /* Vertical blank timer counter value */
	UINT8 *vram;                /* pointer to start of ram/vram (set by MACHINE_RESET) */
	UINT8 *palette_vram;            /* pointer to start of palette area in ram/vram (set by MACHINE_RESET), WSC only */
	int main_palette[8];
	emu_timer *timer;
};

class wswan_state : public driver_device
{
public:
	wswan_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_cursx(*this, "CURSX")
		, m_cursy(*this, "CURSY")
		, m_buttons(*this, "BUTTONS")
	{ }

	virtual void video_start();

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	required_device<cpu_device> m_maincpu;
	DECLARE_READ8_MEMBER(wswan_port_r);
	DECLARE_WRITE8_MEMBER(wswan_port_w);
	DECLARE_READ8_MEMBER(wswan_sram_r);
	DECLARE_WRITE8_MEMBER(wswan_sram_w);
	VDP m_vdp;
	UINT8 m_ws_portram[256];
	UINT8 *m_ROMMap[256];
	UINT32 m_ROMBanks;
	UINT8 m_internal_eeprom[INTERNAL_EEPROM_SIZE];
	UINT8 m_system_type;
	EEPROM m_eeprom;
	RTC m_rtc;
	SoundDMA m_sound_dma;
	UINT8 *m_ws_ram;
	UINT8 *m_ws_bios_bank;
	UINT8 m_bios_disabled;
	int m_pal[16][16];
	bitmap_ind16 m_bitmap;
	UINT8 m_rotate;
	void wswan_clear_irq_line(int irq);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void palette_init();
	DECLARE_MACHINE_START(wscolor);
	DECLARE_PALETTE_INIT(wscolor);
	TIMER_CALLBACK_MEMBER(wswan_rtc_callback);
	TIMER_CALLBACK_MEMBER(wswan_scanline_interrupt);
	void wswan_machine_stop();
	DECLARE_DRIVER_INIT( wswan );
	DECLARE_DEVICE_IMAGE_LOAD_MEMBER( wswan_cart );

protected:
	/* Interrupt flags */
	static const UINT8 WSWAN_IFLAG_STX    = 0x01;
	static const UINT8 WSWAN_IFLAG_KEY    = 0x02;
	static const UINT8 WSWAN_IFLAG_RTC    = 0x04;
	static const UINT8 WSWAN_IFLAG_SRX    = 0x08;
	static const UINT8 WSWAN_IFLAG_LCMP   = 0x10;
	static const UINT8 WSWAN_IFLAG_VBLTMR = 0x20;
	static const UINT8 WSWAN_IFLAG_VBL    = 0x40;
	static const UINT8 WSWAN_IFLAG_HBLTMR = 0x80;

	/* Interrupts */
	static const UINT8 WSWAN_INT_STX    = 0;
	static const UINT8 WSWAN_INT_KEY    = 1;
	static const UINT8 WSWAN_INT_RTC    = 2;
	static const UINT8 WSWAN_INT_SRX    = 3;
	static const UINT8 WSWAN_INT_LCMP   = 4;
	static const UINT8 WSWAN_INT_VBLTMR = 5;
	static const UINT8 WSWAN_INT_VBL    = 6;
	static const UINT8 WSWAN_INT_HBLTMR = 7;

	required_ioport m_cursx;
	required_ioport m_cursy;
	required_ioport m_buttons;

	void wswan_setup_bios();
	void wswan_handle_irqs();
	void wswan_set_irq_line(int irq);
};


/*----------- defined in video/wswan.c -----------*/

void wswan_refresh_scanline( running_machine &machine );


/*----------- defined in audio/wswan.c -----------*/

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

struct CHAN
{
	CHAN() :
		freq(0),
		period(0),
		pos(0),
		vol_left(0),
		vol_right(0),
		on(0),
		signal(0) { }

	UINT16  freq;           /* frequency */
	UINT32  period;         /* period */
	UINT32  pos;            /* position */
	UINT8   vol_left;       /* volume left */
	UINT8   vol_right;      /* volume right */
	UINT8   on;         /* on/off */
	INT8    signal;         /* signal */
};


// ======================> wswan_sound_device

class wswan_sound_device : public device_t,
									public device_sound_interface
{
public:
	wswan_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~wswan_sound_device() { }

protected:
	// device-level overrides
	virtual void device_start();

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

public:
	DECLARE_WRITE8_MEMBER( wswan_sound_port_w );

private:
	void wswan_ch_set_freq( CHAN *ch, UINT16 freq );

private:
	sound_stream *m_channel;
	CHAN m_audio1;     /* Audio channel 1 */
	CHAN m_audio2;     /* Audio channel 2 */
	CHAN m_audio3;     /* Audio channel 3 */
	CHAN m_audio4;     /* Audio channel 4 */
	INT8    m_sweep_step;     /* Sweep step */
	UINT32  m_sweep_time;     /* Sweep time */
	UINT32  m_sweep_count;        /* Sweep counter */
	UINT8   m_noise_type;     /* Noise generator type */
	UINT8   m_noise_reset;        /* Noise reset */
	UINT8   m_noise_enable;       /* Noise enable */
	UINT16  m_sample_address;     /* Sample address */
	UINT8   m_audio2_voice;       /* Audio 2 voice */
	UINT8   m_audio3_sweep;       /* Audio 3 sweep */
	UINT8   m_audio4_noise;       /* Audio 4 noise */
	UINT8   m_mono;           /* mono */
	UINT8   m_voice_data;     /* voice data */
	UINT8   m_output_volume;      /* output volume */
	UINT8   m_external_stereo;    /* external stereo */
	UINT8   m_external_speaker;   /* external speaker */
	UINT16  m_noise_shift;        /* Noise counter shift register */
	UINT8   m_master_volume;      /* Master volume */
};

extern const device_type WSWAN;


#endif /* WSWAN_H_ */
