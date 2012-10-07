/*************************************************************************

    Tail to Nose / Super Formula

*************************************************************************/

class tail2nos_state : public driver_device
{
public:
	tail2nos_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_bgvideoram(*this, "bgvideoram"),
		m_spriteram(*this, "spriteram"){ }

	/* memory pointers */
	required_shared_ptr<UINT16> m_bgvideoram;
	required_shared_ptr<UINT16> m_spriteram;
	UINT16 *    m_zoomdata;
//  UINT16 *    m_paletteram;    // currently this uses generic palette handling

	/* video-related */
	tilemap_t   *m_bg_tilemap;
	int         m_charbank;
	int         m_charpalette;
	int         m_video_enable;

	/* devices */
	cpu_device *m_maincpu;
	cpu_device *m_audiocpu;
	device_t *m_k051316;
	DECLARE_WRITE16_MEMBER(sound_command_w);
	DECLARE_WRITE16_MEMBER(tail2nos_bgvideoram_w);
	DECLARE_READ16_MEMBER(tail2nos_zoomdata_r);
	DECLARE_WRITE16_MEMBER(tail2nos_zoomdata_w);
	DECLARE_WRITE16_MEMBER(tail2nos_gfxbank_w);
	DECLARE_WRITE8_MEMBER(sound_bankswitch_w);
	TILE_GET_INFO_MEMBER(get_tile_info);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update_tail2nos(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};

/*----------- defined in video/tail2nos.c -----------*/
extern void tail2nos_zoom_callback(running_machine &machine, int *code,int *color,int *flags);
