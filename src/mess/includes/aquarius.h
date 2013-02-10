/*****************************************************************************
 *
 * includes/aquarius.h
 *
 ****************************************************************************/

#ifndef __AQUARIUS__
#define __AQUARIUS__

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"

class aquarius_state : public driver_device
{
public:
	aquarius_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, "maincpu"),
			m_cassette(*this, CASSETTE_TAG),
			m_speaker(*this, SPEAKER_TAG),
			m_screen(*this, "screen"),
			m_ram(*this, RAM_TAG),
			m_rom(*this, "maincpu"),
			m_videoram(*this, "videoram"),
			m_colorram(*this, "colorram"),
			m_y0(*this, "Y0"),
			m_y1(*this, "Y1"),
			m_y2(*this, "Y2"),
			m_y3(*this, "Y3"),
			m_y4(*this, "Y4"),
			m_y5(*this, "Y5"),
			m_y6(*this, "Y6"),
			m_y7(*this, "Y7")
	{ }

	required_device<legacy_cpu_device> m_maincpu;
	required_device<cassette_image_device> m_cassette;
	required_device<speaker_sound_device> m_speaker;
	required_device<screen_device> m_screen;
	required_device<ram_device> m_ram;
	required_memory_region m_rom;
	required_shared_ptr<UINT8> m_videoram;
	required_shared_ptr<UINT8> m_colorram;
	required_ioport m_y0;
	required_ioport m_y1;
	required_ioport m_y2;
	required_ioport m_y3;
	required_ioport m_y4;
	required_ioport m_y5;
	required_ioport m_y6;
	required_ioport m_y7;

	UINT8 m_scrambler;
	tilemap_t *m_tilemap;

	DECLARE_WRITE8_MEMBER(aquarius_videoram_w);
	DECLARE_WRITE8_MEMBER(aquarius_colorram_w);
	DECLARE_READ8_MEMBER(cassette_r);
	DECLARE_WRITE8_MEMBER(cassette_w);
	DECLARE_READ8_MEMBER(vsync_r);
	DECLARE_WRITE8_MEMBER(mapper_w);
	DECLARE_READ8_MEMBER(printer_r);
	DECLARE_WRITE8_MEMBER(printer_w);
	DECLARE_READ8_MEMBER(keyboard_r);
	DECLARE_WRITE8_MEMBER(scrambler_w);
	DECLARE_READ8_MEMBER(cartridge_r);
	DECLARE_DRIVER_INIT(aquarius);
	TILE_GET_INFO_MEMBER(aquarius_gettileinfo);
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_aquarius(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	DECLARE_INPUT_CHANGED_MEMBER(aquarius_reset);
};
#endif /* AQUARIUS_H_ */
