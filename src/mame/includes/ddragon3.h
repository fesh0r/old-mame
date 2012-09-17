/*************************************************************************

    Double Dragon 3 & The Combatribes

*************************************************************************/


class ddragon3_state : public driver_device
{
public:
	ddragon3_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_bg_videoram(*this, "bg_videoram"),
		m_fg_videoram(*this, "fg_videoram"),
		m_spriteram(*this, "spriteram"){ }

	/* memory pointers */
	required_shared_ptr<UINT16> m_bg_videoram;
	required_shared_ptr<UINT16> m_fg_videoram;
	required_shared_ptr<UINT16> m_spriteram;
//  UINT16 *        m_paletteram; // currently this uses generic palette handling

	/* video-related */
	tilemap_t         *m_fg_tilemap;
	tilemap_t         *m_bg_tilemap;
	UINT16          m_vreg;
	UINT16          m_bg_scrollx;
	UINT16          m_bg_scrolly;
	UINT16          m_fg_scrollx;
	UINT16          m_fg_scrolly;
	UINT16          m_bg_tilebase;

	/* misc */
	UINT16          m_io_reg[8];

	/* devices */
	cpu_device *m_maincpu;
	cpu_device *m_audiocpu;
	DECLARE_WRITE16_MEMBER(ddragon3_io_w);
	DECLARE_WRITE16_MEMBER(ddragon3_scroll_w);
	DECLARE_READ16_MEMBER(ddragon3_scroll_r);
	DECLARE_WRITE16_MEMBER(ddragon3_bg_videoram_w);
	DECLARE_WRITE16_MEMBER(ddragon3_fg_videoram_w);
	DECLARE_WRITE8_MEMBER(oki_bankswitch_w);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	TILE_GET_INFO_MEMBER(get_fg_tile_info);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
};


/*----------- defined in video/ddragon3.c -----------*/


extern VIDEO_START( ddragon3 );
extern SCREEN_UPDATE_IND16( ddragon3 );
extern SCREEN_UPDATE_IND16( ctribe );
