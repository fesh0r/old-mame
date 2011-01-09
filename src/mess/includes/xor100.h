#pragma once

#ifndef __XOR100__
#define __XOR100__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"5b"
#define I8251_A_TAG		"12b"
#define I8251_B_TAG		"14b"
#define I8255A_TAG		"8a"
#define COM5016_TAG		"15c"
#define Z80CTC_TAG		"11b"
#define WD1795_TAG		"wd1795"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"
#define TERMINAL_TAG	"terminal"

class xor100_state : public driver_device
{
public:
	xor100_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_uart_b(*this, I8251_B_TAG),
		  m_fdc(*this, WD1795_TAG),
		  m_ctc(*this, Z80CTC_TAG),
		  m_ram(*this, "messram"),
		  m_terminal(*this, TERMINAL_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_uart_b;
	required_device<device_t> m_fdc;
	required_device<device_t> m_ctc;
	required_device<device_t> m_ram;
	required_device<device_t> m_terminal;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	virtual void machine_start();
	virtual void machine_reset();

	DECLARE_WRITE8_MEMBER( mmu_w );
	DECLARE_WRITE8_MEMBER( prom_toggle_w );
	DECLARE_READ8_MEMBER( prom_disable_r );
	DECLARE_WRITE8_MEMBER( i8251_b_data_w );
	DECLARE_READ8_MEMBER( fdc_wait_r );
	DECLARE_WRITE8_MEMBER( fdc_dcont_w );
	DECLARE_WRITE8_MEMBER( fdc_dsel_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_irq_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_drq_w );

	void bankswitch();

	// memory state
	int m_mode;
	int m_bank;

	// floppy state
	int m_fdc_irq;
	int m_fdc_drq;
	int m_fdc_dden;
};

#endif
