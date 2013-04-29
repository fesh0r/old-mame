/*************************************************************************

    Driver for Midway Zeus games

**************************************************************************/

#define MIDZEUS_VIDEO_CLOCK     XTAL_66_6667MHz

class midzeus_state : public driver_device
{
public:
	midzeus_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_nvram(*this, "nvram"),
			m_ram_base(*this, "ram_base"),
			m_linkram(*this, "linkram"),
			m_tms32031_control(*this, "tms32031_ctl"),
			m_zeusbase(*this, "zeusbase") ,
		m_maincpu(*this, "maincpu") { }

	required_shared_ptr<UINT32> m_nvram;
	required_shared_ptr<UINT32> m_ram_base;
	optional_shared_ptr<UINT32> m_linkram;
	required_shared_ptr<UINT32> m_tms32031_control;
	required_shared_ptr<UINT32> m_zeusbase;

	DECLARE_WRITE32_MEMBER(cmos_w);
	DECLARE_READ32_MEMBER(cmos_r);
	DECLARE_WRITE32_MEMBER(cmos_protect_w);
	DECLARE_READ32_MEMBER(zpram_r);
	DECLARE_WRITE32_MEMBER(zpram_w);
	DECLARE_READ32_MEMBER(bitlatches_r);
	DECLARE_WRITE32_MEMBER(bitlatches_w);
	DECLARE_READ32_MEMBER(crusnexo_leds_r);
	DECLARE_WRITE32_MEMBER(crusnexo_leds_w);
	DECLARE_READ32_MEMBER(linkram_r);
	DECLARE_WRITE32_MEMBER(linkram_w);
	DECLARE_READ32_MEMBER(tms32031_control_r);
	DECLARE_WRITE32_MEMBER(tms32031_control_w);
	DECLARE_WRITE32_MEMBER(keypad_select_w);
	DECLARE_READ32_MEMBER(analog_r);
	DECLARE_WRITE32_MEMBER(analog_w);
	DECLARE_WRITE32_MEMBER(invasn_gun_w);
	DECLARE_READ32_MEMBER(invasn_gun_r);
	DECLARE_READ8_MEMBER(PIC16C5X_T0_clk_r);
	DECLARE_READ32_MEMBER(zeus_r);
	DECLARE_WRITE32_MEMBER(zeus_w);
	DECLARE_CUSTOM_INPUT_MEMBER(custom_49way_r);
	DECLARE_CUSTOM_INPUT_MEMBER(keypad_r);
	DECLARE_READ32_MEMBER(zeus2_timekeeper_r);
	DECLARE_WRITE32_MEMBER(zeus2_timekeeper_w);
	DECLARE_DRIVER_INIT(invasn);
	DECLARE_DRIVER_INIT(mk4);
	DECLARE_DRIVER_INIT(thegrid);
	DECLARE_DRIVER_INIT(crusnexo);
	DECLARE_MACHINE_START(midzeus);
	DECLARE_MACHINE_RESET(midzeus);
	DECLARE_VIDEO_START(midzeus);
	DECLARE_VIDEO_START(midzeus2);
	UINT32 screen_update_midzeus(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_midzeus2(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(display_irq);
	TIMER_CALLBACK_MEMBER(display_irq_off);
	TIMER_CALLBACK_MEMBER(invasn_gun_callback);
	void exit_handler();
	void exit_handler2();
	required_device<cpu_device> m_maincpu;
};

/*----------- defined in video/midzeus2.c -----------*/
DECLARE_READ32_HANDLER( zeus2_r );
DECLARE_WRITE32_HANDLER( zeus2_w );
