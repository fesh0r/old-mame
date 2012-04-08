struct tempsprite
{
	int gfx;
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};

class undrfire_state : public driver_device
{
public:
	undrfire_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT16 m_coin_word;
	UINT16 m_port_sel;
	int m_frame_counter;
	UINT32 *m_ram;
	UINT32 *m_shared_ram;
	struct tempsprite *m_spritelist;
	UINT16 m_rotate_ctrl[8];
	UINT8 m_dislayer[6];
	UINT32 *m_spriteram;
	size_t m_spriteram_size;
	DECLARE_WRITE32_MEMBER(color_ram_w);
	DECLARE_READ32_MEMBER(undrfire_input_r);
	DECLARE_WRITE32_MEMBER(undrfire_input_w);
	DECLARE_READ16_MEMBER(shared_ram_r);
	DECLARE_WRITE16_MEMBER(shared_ram_w);
	DECLARE_READ32_MEMBER(unknown_hardware_r);
	DECLARE_WRITE32_MEMBER(unknown_int_req_w);
	DECLARE_READ32_MEMBER(undrfire_lightgun_r);
	DECLARE_WRITE32_MEMBER(rotate_control_w);
	DECLARE_WRITE32_MEMBER(motor_control_w);
	DECLARE_WRITE32_MEMBER(cbombers_cpua_ctrl_w);
	DECLARE_READ32_MEMBER(cbombers_adc_r);
	DECLARE_WRITE32_MEMBER(cbombers_adc_w);
};


/*----------- defined in video/undrfire.c -----------*/

VIDEO_START( undrfire );
SCREEN_UPDATE_IND16( undrfire );
SCREEN_UPDATE_IND16( cbombers );
