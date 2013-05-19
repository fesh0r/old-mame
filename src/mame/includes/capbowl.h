/*************************************************************************

    Coors Light Bowling/Bowl-O-Rama hardware

*************************************************************************/

#include "machine/nvram.h"

class capbowl_state : public driver_device
{
public:
	enum
	{
		TIMER_CAPBOWL_UPDATE
	};

	capbowl_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_rowaddress(*this, "rowaddress"),
		m_maincpu(*this, "maincpu"),
		m_audiocpu(*this, "audiocpu"){ }

	void init_nvram(nvram_device &nvram, void *base, size_t size);

	/* memory pointers */
	required_shared_ptr<UINT8> m_rowaddress;

	/* video-related */
	offs_t m_blitter_addr;

	/* input-related */
	UINT8 m_last_trackball_val[2];

	/* devices */
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
	DECLARE_WRITE8_MEMBER(capbowl_rom_select_w);
	DECLARE_READ8_MEMBER(track_0_r);
	DECLARE_READ8_MEMBER(track_1_r);
	DECLARE_WRITE8_MEMBER(track_reset_w);
	DECLARE_WRITE8_MEMBER(capbowl_sndcmd_w);
	DECLARE_WRITE8_MEMBER(capbowl_tms34061_w);
	DECLARE_READ8_MEMBER(capbowl_tms34061_r);
	DECLARE_WRITE8_MEMBER(bowlrama_blitter_w);
	DECLARE_READ8_MEMBER(bowlrama_blitter_r);
	DECLARE_DRIVER_INIT(capbowl);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update_capbowl(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(capbowl_interrupt);
	TIMER_CALLBACK_MEMBER(capbowl_update);
	inline rgb_t pen_for_pixel( UINT8 *src, UINT8 pix );
	DECLARE_WRITE_LINE_MEMBER(firqhandler);

protected:
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
};
