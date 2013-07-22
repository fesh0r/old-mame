#ifndef _PC090OJ_H_
#define _PC090OJ_H_

struct pc090oj_interface
{
	int                m_gfxnum;

	int                m_x_offset, m_y_offset;
	int                m_use_buffer;
};

class pc090oj_device : public device_t,
						public pc090oj_interface
{
public:
	pc090oj_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~pc090oj_device() {}

	DECLARE_READ16_MEMBER( word_r );
	DECLARE_WRITE16_MEMBER( word_w );

	void set_sprite_ctrl(UINT16 sprctrl);
	void eof_callback();
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect, int pri_type);

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();

private:
	/* NB: pc090oj_ctrl is the internal register controlling flipping

	pc090oj_sprite_ctrl is a representation of the hardware OUTSIDE the pc090oj
	which impacts on sprite plotting, and which varies between games. It
	includes color banking and (optionally) priority. It allows each game to
	control these aspects of the sprites in different ways, while keeping the
	routines here modular.

*/

	UINT16     m_ctrl;
	UINT16     m_sprite_ctrl;

	UINT16 *   m_ram;
	UINT16 *   m_ram_buffered;
};

extern const device_type PC090OJ;

#define MCFG_PC090OJ_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, PC090OJ, 0) \
	MCFG_DEVICE_CONFIG(_interface)

#endif
