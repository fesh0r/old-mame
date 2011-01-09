#ifndef __BW2__
#define __BW2__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"ic1"
#define I8255A_TAG		"ic4"
#define WD2797_TAG		"ic5"
#define PIT8253_TAG		"ic6"
#define MSM8251_TAG		"ic7"
#define MSM6255_TAG		"ic49"
#define CENTRONICS_TAG	"centronics"

#define BW2_VIDEORAM_SIZE	0x4000
#define BW2_RAMCARD_SIZE	0x80000

enum {
	BANK_RAM1 = 0,
	BANK_VRAM,
	BANK_RAM2, BANK_RAMCARD_ROM = BANK_RAM2,
	BANK_RAM3,
	BANK_RAM4,
	BANK_RAM5, BANK_RAMCARD_RAM = BANK_RAM5,
	BANK_RAM6,
	BANK_ROM
};

class bw2_state : public driver_device
{
public:
	bw2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
  		  m_uart(*this, MSM8251_TAG),
		  m_fdc(*this, WD2797_TAG),
		  m_lcdc(*this, MSM6255_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_ram(*this, "messram"),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_uart;
	required_device<device_t> m_fdc;
	required_device<device_t> m_lcdc;
	required_device<device_t> m_centronics;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;
	
	virtual void machine_start();
	virtual void machine_reset();

	virtual bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	int get_ramdisk_size();
	void bankswitch(UINT8 data);
	void ramcard_bankswitch(UINT8 data);

	DECLARE_WRITE8_MEMBER( ramcard_bank_w );
	DECLARE_READ8_MEMBER( fdc_r );
	DECLARE_WRITE8_MEMBER( fdc_w );
	DECLARE_WRITE8_MEMBER( ppi_pa_w );
	DECLARE_READ8_MEMBER( ppi_pb_r );
	DECLARE_WRITE8_MEMBER( ppi_pc_w );
	DECLARE_READ8_MEMBER( ppi_pc_r );
	DECLARE_WRITE_LINE_MEMBER( pit_out0_w );
	DECLARE_WRITE_LINE_MEMBER( mtron_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_drq_w );

	/* keyboard state */
	UINT8 m_kb_row;

	/* memory state */
	UINT8 *m_work_ram;
	UINT8 *m_ramcard_ram;
	UINT8 m_bank;

	/* floppy state */
	int m_drive;
	int m_mtron;
	int m_mfdbk;

	/* video state */
	UINT8 *m_video_ram;

	/* devices */
};

#endif
