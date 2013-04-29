#include "machine/eeprom.h"

class xorworld_state : public driver_device
{
public:
	xorworld_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_videoram(*this, "videoram"),
		m_spriteram(*this, "spriteram"),
		m_maincpu(*this, "maincpu"),
		m_eeprom(*this, "eeprom") { }

	required_shared_ptr<UINT16> m_videoram;
	tilemap_t *m_bg_tilemap;
	required_shared_ptr<UINT16> m_spriteram;
	DECLARE_WRITE16_MEMBER(xorworld_irq2_ack_w);
	DECLARE_WRITE16_MEMBER(xorworld_irq6_ack_w);
	DECLARE_WRITE16_MEMBER(xorworld_videoram16_w);
	DECLARE_WRITE16_MEMBER(eeprom_chip_select_w);
	DECLARE_WRITE16_MEMBER(eeprom_serial_clock_w);
	DECLARE_WRITE16_MEMBER(eeprom_data_w);
	DECLARE_DRIVER_INIT(xorworld);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_xorworld(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect );
	required_device<cpu_device> m_maincpu;
	required_device<eeprom_device> m_eeprom;
};
