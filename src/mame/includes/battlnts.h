/*************************************************************************

    Konami Battlantis Hardware

*************************************************************************/

class battlnts_state : public driver_device
{
public:
	battlnts_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	/* memory pointers */
//  UINT8 *      paletteram;    // this currently uses generic palette handling

	/* video-related */
	int m_spritebank;
	int m_layer_colorbase[2];


	/* devices */
	cpu_device *m_audiocpu;
	device_t *m_k007342;
	device_t *m_k007420;
	DECLARE_WRITE8_MEMBER(battlnts_sh_irqtrigger_w);
	DECLARE_WRITE8_MEMBER(battlnts_bankswitch_w);
	DECLARE_WRITE8_MEMBER(battlnts_spritebank_w);
	DECLARE_DRIVER_INIT(rackemup);
	virtual void machine_start();
	virtual void machine_reset();
	UINT32 screen_update_battlnts(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(battlnts_interrupt);
};

/*----------- defined in video/battlnts.c -----------*/
void battlnts_tile_callback(running_machine &machine, int layer, int bank, int *code, int *color, int *flags);
void battlnts_sprite_callback(running_machine &machine, int *code, int *color);
