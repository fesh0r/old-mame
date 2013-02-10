#include "video/poly.h"

#define NAMCOS22_PALETTE_SIZE 0x8000
#define MAX_LIT_SURFACES 0x80
#define MAX_RENDER_CMD_SEQ 0x1c

#define GFX_CHAR               0
#define GFX_TEXTURE_TILE       1
#define GFX_SPRITE             2

enum
{
	NAMCOS22_AIR_COMBAT22,
	NAMCOS22_ALPINE_RACER,
	NAMCOS22_CYBER_COMMANDO,
	NAMCOS22_CYBER_CYCLES,
	NAMCOS22_PROP_CYCLE,
	NAMCOS22_RAVE_RACER,
	NAMCOS22_RIDGE_RACER,
	NAMCOS22_RIDGE_RACER2,
	NAMCOS22_TIME_CRISIS,
	NAMCOS22_VICTORY_LAP,
	NAMCOS22_ACE_DRIVER,
	NAMCOS22_ALPINE_RACER_2,
	NAMCOS22_ALPINE_SURFER,
	NAMCOS22_TOKYO_WARS,
	NAMCOS22_AQUA_JET,
	NAMCOS22_DIRT_DASH,
	NAMCOS22_ARMADILLO_RACING
};

class namcos22_state : public driver_device
{
public:
	namcos22_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this,"maincpu"),
		m_master(*this,"master"),
		m_slave(*this,"slave"),
		m_mcu(*this,"mcu"),
		m_iomcu(*this,"iomcu"),
		m_spriteram(*this,"spriteram"),
		m_shareram(*this,"shareram"),
		m_system_controller(*this,"syscontrol"),
		m_nvmem(*this,"nvmem"),
		m_pSlaveExternalRAM(*this,"slaveextram"),
		m_pMasterExternalRAM(*this,"masterextram"),
		m_cgram(*this,"cgram"),
		m_textram(*this,"textram"),
		m_polygonram(*this,"polygonram"),
		m_gamma(*this,"gamma"),
		m_vics_data(*this,"vics_data"),
		m_vics_control(*this,"vics_control"),
		m_czattr(*this,"czattr"),
		m_tilemapattr(*this,"tilemapattr"),
		m_czram(*this,"czram"),
		m_pc_pedal_interrupt(*this, "pc_p_int")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_master;
	required_device<cpu_device> m_slave;
	required_device<cpu_device> m_mcu;
	optional_device<cpu_device> m_iomcu;
	optional_shared_ptr<UINT32> m_spriteram;
	required_shared_ptr<UINT32> m_shareram;
	required_shared_ptr<UINT32> m_system_controller;
	required_shared_ptr<UINT32> m_nvmem;
	required_shared_ptr<UINT16> m_pSlaveExternalRAM;
	required_shared_ptr<UINT16> m_pMasterExternalRAM;
	required_shared_ptr<UINT32> m_cgram;
	required_shared_ptr<UINT32> m_textram;
	required_shared_ptr<UINT32> m_polygonram;
	required_shared_ptr<UINT32> m_gamma;
	optional_shared_ptr<UINT32> m_vics_data;
	optional_shared_ptr<UINT32> m_vics_control;
	optional_shared_ptr<UINT32> m_czattr;
	required_shared_ptr<UINT32> m_tilemapattr;
	optional_shared_ptr<UINT32> m_czram;
	optional_device<timer_device> m_pc_pedal_interrupt;

	int m_bEnableDspIrqs;
	emu_timer *m_ar_tb_interrupt[2];
	UINT16 m_MasterBIOZ;
	UINT32 *m_pPointRAM;
	UINT32 m_old_coin_state;
	UINT32 m_credits1;
	UINT32 m_credits2;
	UINT32 m_PointAddr;
	UINT32 m_PointData;
	UINT16 m_SerialDataSlaveToMasterNext;
	UINT16 m_SerialDataSlaveToMasterCurrent;
	int m_RenderBufSize;
	UINT16 m_RenderBufData[MAX_RENDER_CMD_SEQ];
	UINT16 m_portbits[2];
	int m_irq_state;
	int m_DspUploadState;
	int m_UploadDestIdx;
	UINT32 m_AlpineSurferProtData;
	int m_motor_status;
	emu_timer *m_motor_timer;
	int m_p4;
	UINT16 m_su_82;
	UINT16 m_keycus_id;
	int m_gametype;
	int m_bSuperSystem22;
	int m_chipselect;
	int m_spot_enable;
	int m_spot_read_address;
	int m_spot_write_address;
	UINT16 *m_spotram;
	UINT16 *m_banked_czram[4];
	UINT8 *m_recalc_czram[4];
	UINT32 m_cz_was_written[4];
	int m_cz_adjust;
	poly_manager *m_poly;
	UINT16 *m_pTextureTileMap16;
	UINT8 *m_pTextureTileMapAttr;
	UINT8 *m_pTextureTileData;
	UINT8 m_XYAttrToPixel[16][16][16];
	UINT16 m_dspram_bank;
	UINT16 m_UpperWordLatch;
	int m_bDSPisActive;
	INT32 m_AbsolutePriority;
	INT32 m_ObjectShiftValue22;
	UINT16 m_PrimitiveID;
	float m_ViewMatrix[4][4];
	UINT8 m_LitSurfaceInfo[MAX_LIT_SURFACES];
	INT32 m_SurfaceNormalFormat;
	unsigned m_LitSurfaceCount;
	unsigned m_LitSurfaceIndex;
	int m_PtRomSize;
	const UINT8 *m_pPolyH;
	const UINT8 *m_pPolyM;
	const UINT8 *m_pPolyL;
	UINT8 *m_dirtypal;
	bitmap_ind16 *m_mix_bitmap;
	tilemap_t *m_bgtilemap;

	DECLARE_READ32_MEMBER(namcos22_gamma_r);
	DECLARE_WRITE32_MEMBER(namcos22_gamma_w);
	DECLARE_WRITE32_MEMBER(namcos22s_czram_w);
	DECLARE_READ32_MEMBER(namcos22s_czram_r);
	DECLARE_READ32_MEMBER(namcos22s_vics_control_r);
	DECLARE_WRITE32_MEMBER(namcos22s_vics_control_w);
	DECLARE_READ32_MEMBER(namcos22_textram_r);
	DECLARE_WRITE32_MEMBER(namcos22_textram_w);
	DECLARE_READ32_MEMBER(namcos22_tilemapattr_r);
	DECLARE_WRITE32_MEMBER(namcos22_tilemapattr_w);
	DECLARE_READ32_MEMBER(namcos22s_spotram_r);
	DECLARE_WRITE32_MEMBER(namcos22s_spotram_w);
	DECLARE_READ32_MEMBER(namcos22_dspram_r);
	DECLARE_WRITE32_MEMBER(namcos22_dspram_w);
	DECLARE_READ32_MEMBER(namcos22_cgram_r);
	DECLARE_WRITE32_MEMBER(namcos22_cgram_w);
	DECLARE_READ32_MEMBER(namcos22_paletteram_r);
	DECLARE_WRITE32_MEMBER(namcos22_paletteram_w);
	DECLARE_WRITE16_MEMBER(namcos22_dspram16_bank_w);
	DECLARE_READ16_MEMBER(namcos22_dspram16_r);
	DECLARE_WRITE16_MEMBER(namcos22_dspram16_w);
	DECLARE_CUSTOM_INPUT_MEMBER(alpine_motor_read);
	DECLARE_READ16_MEMBER(pdp_status_r);
	DECLARE_READ16_MEMBER(pdp_begin_r);
	DECLARE_READ16_MEMBER(slave_external_ram_r);
	DECLARE_WRITE16_MEMBER(slave_external_ram_w);
	DECLARE_READ16_MEMBER(dsp_HOLD_signal_r);
	DECLARE_WRITE16_MEMBER(dsp_HOLD_ACK_w);
	DECLARE_WRITE16_MEMBER(dsp_XF_output_w);
	DECLARE_WRITE16_MEMBER(point_ram_idx_w);
	DECLARE_WRITE16_MEMBER(point_ram_loword_iw);
	DECLARE_WRITE16_MEMBER(point_ram_hiword_w);
	DECLARE_READ16_MEMBER(point_ram_loword_r);
	DECLARE_READ16_MEMBER(point_ram_hiword_ir);
	DECLARE_WRITE16_MEMBER(dsp_unk2_w);
	DECLARE_READ16_MEMBER(dsp_unk_port3_r);
	DECLARE_WRITE16_MEMBER(upload_code_to_slave_dsp_w);
	DECLARE_READ16_MEMBER(dsp_unk8_r);
	DECLARE_READ16_MEMBER(custom_ic_status_r);
	DECLARE_READ16_MEMBER(dsp_upload_status_r);
	DECLARE_READ16_MEMBER(master_external_ram_r);
	DECLARE_WRITE16_MEMBER(master_external_ram_w);
	DECLARE_WRITE16_MEMBER(slave_serial_io_w);
	DECLARE_READ16_MEMBER(master_serial_io_r);
	DECLARE_WRITE16_MEMBER(dsp_unk_porta_w);
	DECLARE_WRITE16_MEMBER(dsp_led_w);
	DECLARE_WRITE16_MEMBER(dsp_unk8_w);
	DECLARE_WRITE16_MEMBER(master_render_device_w);
	DECLARE_READ16_MEMBER(dsp_BIOZ_r);
	DECLARE_READ16_MEMBER(dsp_slave_port3_r);
	DECLARE_READ16_MEMBER(dsp_slave_port4_r);
	DECLARE_READ16_MEMBER(dsp_slave_port5_r);
	DECLARE_READ16_MEMBER(dsp_slave_port6_r);
	DECLARE_WRITE16_MEMBER(dsp_slave_portc_w);
	DECLARE_READ16_MEMBER(dsp_slave_port8_r);
	DECLARE_READ16_MEMBER(dsp_slave_portb_r);
	DECLARE_WRITE16_MEMBER(dsp_slave_portb_w);
	DECLARE_READ32_MEMBER(namcos22_C139_SCI_r);
	DECLARE_WRITE32_MEMBER(namcos22_C139_SCI_w);
	DECLARE_READ32_MEMBER(namcos22_system_controller_r);
	DECLARE_WRITE32_MEMBER(namcos22s_system_controller_w);
	DECLARE_WRITE32_MEMBER(namcos22_system_controller_w);
	DECLARE_READ32_MEMBER(namcos22_keycus_r);
	DECLARE_WRITE32_MEMBER(namcos22_keycus_w);
	DECLARE_READ16_MEMBER(namcos22_portbit_r);
	DECLARE_WRITE16_MEMBER(namcos22_portbit_w);
	DECLARE_READ16_MEMBER(namcos22_dipswitch_r);
	DECLARE_READ32_MEMBER(namcos22_mcuram_r);
	DECLARE_WRITE32_MEMBER(namcos22_mcuram_w);
	DECLARE_READ32_MEMBER(namcos22_gun_r);
	DECLARE_WRITE16_MEMBER(namcos22_cpuleds_w);
	DECLARE_READ32_MEMBER(alpinesa_prot_r);
	DECLARE_WRITE32_MEMBER(alpinesa_prot_w);
	DECLARE_WRITE32_MEMBER(namcos22s_nvmem_w);
	DECLARE_WRITE32_MEMBER(namcos22s_chipselect_w);
	DECLARE_READ16_MEMBER(s22mcu_shared_r);
	DECLARE_WRITE16_MEMBER(s22mcu_shared_w);
	DECLARE_WRITE8_MEMBER(mcu_port4_w);
	DECLARE_READ8_MEMBER(mcu_port4_r);
	DECLARE_WRITE8_MEMBER(mcu_port5_w);
	DECLARE_READ8_MEMBER(mcu_port5_r);
	DECLARE_WRITE8_MEMBER(mcu_port6_w);
	DECLARE_READ8_MEMBER(mcu_port6_r);
	DECLARE_WRITE8_MEMBER(mcu_port7_w);
	DECLARE_READ8_MEMBER(mcu_port7_r);
	DECLARE_READ8_MEMBER(namcos22s_mcu_adc_r);
	DECLARE_WRITE8_MEMBER(propcycle_mcu_port5_w);
	DECLARE_WRITE8_MEMBER(alpine_mcu_port5_w);
	DECLARE_READ8_MEMBER(mcu_port4_s22_r);
	DECLARE_READ16_MEMBER(mcu141_speedup_r);
	DECLARE_WRITE16_MEMBER(mcu_speedup_w);
	DECLARE_READ16_MEMBER(mcu130_speedup_r);
	DECLARE_READ16_MEMBER(mcuc74_speedup_r);
	UINT32 ReadFromCommRAM(offs_t offs );
	void WriteToCommRAM(offs_t offs, UINT32 data );
	void WriteToPointRAM(offs_t offs, UINT32 data );
	DECLARE_DRIVER_INIT(acedrvr);
	DECLARE_DRIVER_INIT(aquajet);
	DECLARE_DRIVER_INIT(adillor);
	DECLARE_DRIVER_INIT(cybrcyc);
	DECLARE_DRIVER_INIT(raveracw);
	DECLARE_DRIVER_INIT(ridger2j);
	DECLARE_DRIVER_INIT(victlap);
	DECLARE_DRIVER_INIT(cybrcomm);
	DECLARE_DRIVER_INIT(timecris);
	DECLARE_DRIVER_INIT(tokyowar);
	DECLARE_DRIVER_INIT(propcycl);
	DECLARE_DRIVER_INIT(alpiner2);
	DECLARE_DRIVER_INIT(dirtdash);
	DECLARE_DRIVER_INIT(airco22);
	DECLARE_DRIVER_INIT(alpiner);
	DECLARE_DRIVER_INIT(ridgeraj);
	DECLARE_DRIVER_INIT(alpinesa);
	TILE_GET_INFO_MEMBER(TextTilemapGetInfo);
	virtual void machine_reset();
	virtual void machine_start();
	DECLARE_MACHINE_START(adillor);
	DECLARE_VIDEO_START(namcos22s);
	DECLARE_VIDEO_START(namcos22);
	DECLARE_VIDEO_START(common);
	UINT32 screen_update_namcos22s(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_namcos22(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(namcos22s_interrupt);
	INTERRUPT_GEN_MEMBER(namcos22_interrupt);
	TIMER_DEVICE_CALLBACK_MEMBER(adillor_trackball_update);
	TIMER_CALLBACK_MEMBER(adillor_trackball_interrupt);
	TIMER_DEVICE_CALLBACK_MEMBER(propcycl_pedal_update);
	TIMER_DEVICE_CALLBACK_MEMBER(propcycl_pedal_interrupt);
	TIMER_CALLBACK_MEMBER(alpine_steplock_callback);
	TIMER_DEVICE_CALLBACK_MEMBER(dsp_master_serial_irq);
	TIMER_DEVICE_CALLBACK_MEMBER(dsp_slave_serial_irq);
	TIMER_DEVICE_CALLBACK_MEMBER(mcu_irq);
	void namcos22_reset();
	void namcos22_exit();
};

/*----------- defined in video/namcos22.c -----------*/
void namcos22_draw_direct_poly( running_machine &machine, const UINT16 *pSource );
UINT32 namcos22_point_rom_r( running_machine &machine, offs_t offs );
void namcos22_enable_slave_simulation( running_machine &machine, int enable );
