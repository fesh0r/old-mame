/*************************************************************************

    Blades of Steel

*************************************************************************/
#include "sound/upd7759.h"
#include "video/k007342.h"
#include "video/k007420.h"
#include "video/k051733.h"

class bladestl_state : public driver_device
{
public:
	bladestl_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_paletteram(*this, "paletteram"),
		m_audiocpu(*this, "audiocpu"),
		m_k007342(*this, "k007342"),
		m_k007420(*this, "k007420"),
		m_maincpu(*this, "maincpu"),
		m_upd7759(*this, "upd") { }

	/* memory pointers */
	required_shared_ptr<UINT8> m_paletteram;

	/* video-related */
	int        m_spritebank;
	int        m_layer_colorbase[2];

	/* misc */
	int        m_last_track[4];

	/* devices */
	required_device<cpu_device> m_audiocpu;
	required_device<k007342_device> m_k007342;
	required_device<k007420_device> m_k007420;
	DECLARE_READ8_MEMBER(trackball_r);
	DECLARE_WRITE8_MEMBER(bladestl_bankswitch_w);
	DECLARE_WRITE8_MEMBER(bladestl_sh_irqtrigger_w);
	DECLARE_WRITE8_MEMBER(bladestl_port_B_w);
	DECLARE_READ8_MEMBER(bladestl_speech_busy_r);
	DECLARE_WRITE8_MEMBER(bladestl_speech_ctrl_w);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void palette_init();
	UINT32 screen_update_bladestl(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(bladestl_scanline);
	required_device<cpu_device> m_maincpu;
	required_device<upd7759_device> m_upd7759;
};

/*----------- defined in video/bladestl.c -----------*/
void bladestl_tile_callback(running_machine &machine, int layer, int bank, int *code, int *color, int *flags);
void bladestl_sprite_callback(running_machine &machine, int *code, int *color);
