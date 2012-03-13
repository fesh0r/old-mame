/*************************************************************************

    Haunted Castle

*************************************************************************/

#include "video/bufsprite.h"

class hcastle_state : public driver_device
{
public:
	hcastle_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_spriteram(*this, "spriteram"),
		  m_spriteram2(*this, "spriteram2") { }

	/* memory pointers */
	UINT8 *    m_pf1_videoram;
	UINT8 *    m_pf2_videoram;
	UINT8 *    m_paletteram;

	/* video-related */
	tilemap_t    *m_fg_tilemap;
	tilemap_t    *m_bg_tilemap;
	int        m_pf2_bankbase;
	int        m_pf1_bankbase;
	int        m_old_pf1;
	int        m_old_pf2;
	int        m_gfx_bank;

	/* devices */
	device_t *m_audiocpu;
	device_t *m_k007121_1;
	device_t *m_k007121_2;

	required_device<buffered_spriteram8_device> m_spriteram;
	required_device<buffered_spriteram8_device> m_spriteram2;
};


/*----------- defined in video/hcastle.c -----------*/

WRITE8_HANDLER( hcastle_pf1_video_w );
WRITE8_HANDLER( hcastle_pf2_video_w );
READ8_HANDLER( hcastle_gfxbank_r );
WRITE8_HANDLER( hcastle_gfxbank_w );
WRITE8_HANDLER( hcastle_pf1_control_w );
WRITE8_HANDLER( hcastle_pf2_control_w );

PALETTE_INIT( hcastle );
SCREEN_UPDATE_IND16( hcastle );
VIDEO_START( hcastle );
