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
		: driver_device(mconfig, type, tag),
			m_ram(*this, "ram"),
			m_shared_ram(*this, "shared_ram"),
			m_spriteram(*this, "spriteram") { }

	UINT16 m_coin_word;
	UINT16 m_port_sel;
	int m_frame_counter;
	optional_shared_ptr<UINT32> m_ram;
	optional_shared_ptr<UINT32> m_shared_ram;
	struct tempsprite *m_spritelist;
	UINT16 m_rotate_ctrl[8];
	UINT8 m_dislayer[6];
	required_shared_ptr<UINT32> m_spriteram;
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
	DECLARE_CUSTOM_INPUT_MEMBER(frame_counter_r);
	DECLARE_DRIVER_INIT(undrfire);
	DECLARE_DRIVER_INIT(cbombers);
	virtual void video_start();
	UINT32 screen_update_undrfire(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_cbombers(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(undrfire_interrupt);
	TIMER_CALLBACK_MEMBER(interrupt5);
};
