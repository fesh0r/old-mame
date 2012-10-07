class truco_state : public driver_device
{
public:
	truco_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_videoram(*this, "videoram"),
		m_battery_ram(*this, "battery_ram"){ }

	required_shared_ptr<UINT8> m_videoram;
	required_shared_ptr<UINT8> m_battery_ram;
	int m_trigger;
	DECLARE_WRITE8_MEMBER(porta_w);
	DECLARE_WRITE8_MEMBER(pia_ca2_w);
	DECLARE_WRITE8_MEMBER(portb_w);
	DECLARE_WRITE8_MEMBER(pia_irqa_w);
	DECLARE_WRITE8_MEMBER(pia_irqb_w);
	virtual void machine_reset();
	virtual void palette_init();
	UINT32 screen_update_truco(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(truco_interrupt);
};
