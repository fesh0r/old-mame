/*************************************************************************

    Driver for Midway X-unit games.

**************************************************************************/

class midxunit_state : public driver_device
{
public:
	midxunit_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_nvram(*this, "nvram") { }

	required_shared_ptr<UINT16>	m_nvram;
	UINT8 *decode_memory;
	UINT8 cmos_write_enable;
	UINT16 iodata[8];
	UINT8 ioshuffle[16];
	UINT8 analog_port;
	UINT8 uart[8];
	UINT8 security_bits;
};


/*----------- defined in machine/midxunit.c -----------*/

READ16_HANDLER( midxunit_cmos_r );
WRITE16_HANDLER( midxunit_cmos_w );

WRITE16_HANDLER( midxunit_io_w );
WRITE16_HANDLER( midxunit_unknown_w );

READ16_HANDLER( midxunit_io_r );
READ16_HANDLER( midxunit_analog_r );
WRITE16_HANDLER( midxunit_analog_select_w );
READ16_HANDLER( midxunit_status_r );

READ16_HANDLER( midxunit_uart_r );
WRITE16_HANDLER( midxunit_uart_w );

DRIVER_INIT( revx );

MACHINE_RESET( midxunit );

READ16_HANDLER( midxunit_security_r );
WRITE16_HANDLER( midxunit_security_w );
WRITE16_HANDLER( midxunit_security_clock_w );

READ16_HANDLER( midxunit_sound_r );
WRITE16_HANDLER( midxunit_sound_w );
