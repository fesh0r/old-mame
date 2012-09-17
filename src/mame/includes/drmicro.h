/*************************************************************************

    Dr. Micro

*************************************************************************/


class drmicro_state : public driver_device
{
public:
	drmicro_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	/* memory pointers */
	UINT8 *        m_videoram;

	/* video-related */
	tilemap_t        *m_bg1;
	tilemap_t        *m_bg2;
	int            m_flipscreen;

	/* misc */
	int            m_nmi_enable;
	int            m_pcm_adr;

	/* devices */
	device_t *m_msm;
	DECLARE_WRITE8_MEMBER(nmi_enable_w);
	DECLARE_WRITE8_MEMBER(pcm_set_w);
	DECLARE_WRITE8_MEMBER(drmicro_videoram_w);
	TILE_GET_INFO_MEMBER(get_bg1_tile_info);
	TILE_GET_INFO_MEMBER(get_bg2_tile_info);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	virtual void palette_init();
};


/*----------- defined in video/drmicro.c -----------*/



SCREEN_UPDATE_IND16( drmicro );

