/*************************************************************************

    4enraya

*************************************************************************/

#include "sound/ay8910.h"

class _4enraya_state : public driver_device
{
public:
	_4enraya_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_ay(*this, "aysnd"),
		m_snd_latch_bit(4),
		m_maincpu(*this, "maincpu") { }


	required_device<ay8910_device> m_ay;

	/* memory pointers */
	UINT8      m_videoram[0x1000];
	UINT8      m_workram[0x1000];

	/* video-related */
	tilemap_t    *m_bg_tilemap;

	/* sound-related */
	int        m_soundlatch;
	int        m_last_snd_ctrl;

	int                 m_snd_latch_bit;
	DECLARE_WRITE8_MEMBER(sound_data_w);
	DECLARE_READ8_MEMBER(fenraya_custom_map_r);
	DECLARE_WRITE8_MEMBER(fenraya_custom_map_w);
	DECLARE_WRITE8_MEMBER(fenraya_videoram_w);
	DECLARE_WRITE8_MEMBER(sound_control_w);
	DECLARE_DRIVER_INIT(unkpacg);
	TILE_GET_INFO_MEMBER(get_tile_info);

	UINT8* m_prom;
	UINT8* m_rom;

	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_4enraya(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	required_device<cpu_device> m_maincpu;
};
