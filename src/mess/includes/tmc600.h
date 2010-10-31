#pragma once

#ifndef __TMC600__
#define __TMC600__

#include "cpu/cosmac/cosmac.h"

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1869_TAG		"cdp1869"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define TMC600_PAGE_RAM_SIZE	0x400
#define TMC600_PAGE_RAM_MASK	0x3ff

class tmc600_state : public driver_device
{
public:
	tmc600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, CDP1802_TAG),
		  m_vis(*this, CDP1869_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_ram(*this, "messram")
	 { }

	required_device<cosmac_device> m_maincpu;
	required_device<cdp1869_device> m_vis;
	required_device<running_device> m_cassette;
	required_device<running_device> m_ram;

	virtual void machine_start();

	virtual void video_start();
	bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect) { m_vis->update_screen(&bitmap, &cliprect); return false; }

	DECLARE_WRITE8_MEMBER(keyboard_latch_w);
	DECLARE_WRITE8_MEMBER(vismac_register_w);
	DECLARE_WRITE8_MEMBER(vismac_data_w);
	DECLARE_WRITE8_MEMBER(page_ram_w);

	UINT8 get_color(UINT16 pma);

	// video state
	int m_vismac_reg_latch;		// video register latch
	int m_vismac_color_latch;	// color latch
	int m_vismac_bkg_latch;		// background color latch
	int m_blink;				// cursor blink

	UINT8 *m_page_ram;			// page memory
	UINT8 *m_color_ram;			// color memory
	UINT8 *m_char_rom;			// character generator ROM

	// keyboard state
	int m_keylatch;				// key latch
};

// ---------- defined in video/tmc600.c ----------

MACHINE_CONFIG_EXTERN( tmc600_video );

#endif
