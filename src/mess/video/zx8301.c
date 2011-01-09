/**********************************************************************

    Sinclair ZX8301 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

	- wait state on memory access during video update
    - proper video timing
	- get rid of flash timer

*/

#include "emu.h"
#include "zx8301.h"



//**************************************************************************
//	MACROS / CONSTANTS
//**************************************************************************

#define LOG 0


// low resolution palette
static const int ZX8301_COLOR_MODE4[] = { 0, 2, 4, 7 };



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type ZX8301 = zx8301_device_config::static_alloc_device_config;


// default address map
static ADDRESS_MAP_START( zx8301, 0, 8 )
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
ADDRESS_MAP_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  zx8301_device_config - constructor
//-------------------------------------------------

zx8301_device_config::zx8301_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "Sinclair ZX8301", tag, owner, clock),
	  device_config_memory_interface(mconfig, *this),
	  m_space_config("videoram", ENDIANNESS_LITTLE, 8, 17, 0, NULL, *ADDRESS_MAP_NAME(zx8301))
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *zx8301_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(zx8301_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *zx8301_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(&machine, zx8301_device(machine, *this));
}


//-------------------------------------------------
//  memory_space_config - return a description of
//  any address spaces owned by this device
//-------------------------------------------------

const address_space_config *zx8301_device_config::memory_space_config(int spacenum) const
{
	return (spacenum == 0) ? &m_space_config : NULL;
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void zx8301_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const zx8301_interface *intf = reinterpret_cast<const zx8301_interface *>(static_config());
	if (intf != NULL)
		*static_cast<zx8301_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&out_vsync_func, 0, sizeof(out_vsync_func));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  readbyte - read a byte at the given address
//-------------------------------------------------

inline UINT8 zx8301_device::readbyte(offs_t address)
{
	return space()->read_byte(address);
}


//-------------------------------------------------
//  writebyte - write a byte at the given address
//-------------------------------------------------

inline void zx8301_device::writebyte(offs_t address, UINT8 data)
{
	space()->write_byte(address, data);
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  zx8301_device - constructor
//-------------------------------------------------

zx8301_device::zx8301_device(running_machine &_machine, const zx8301_device_config &config)
    : device_t(_machine, config),
	  device_memory_interface(_machine, config, *this),
	  m_dispoff(1),
	  m_mode8(0),
	  m_base(0),
	  m_flash(1),
	  m_vsync(1),
	  m_vda(0),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void zx8301_device::device_start()
{
	// get the CPU
	m_cpu = machine->device<cpu_device>(m_config.cpu_tag);
	assert(m_cpu != NULL);

	// get the screen device
	m_screen = machine->device<screen_device>(m_config.screen_tag);
	assert(m_screen != NULL);

	// resolve callbacks
    devcb_resolve_write_line(&m_out_vsync_func, &m_config.out_vsync_func, this);
	
	// allocate timers
	m_vsync_timer = device_timer_alloc(*this, TIMER_VSYNC);
	m_flash_timer = device_timer_alloc(*this, TIMER_FLASH);

	// adjust timer periods
	timer_adjust_periodic(m_vsync_timer, attotime_zero, 0, ATTOTIME_IN_HZ(50));
	timer_adjust_periodic(m_flash_timer, ATTOTIME_IN_HZ(2), 0, ATTOTIME_IN_HZ(2));

	// register for state saving
	state_save_register_device_item(this, 0, m_dispoff);
	state_save_register_device_item(this, 0, m_mode8);
	state_save_register_device_item(this, 0, m_base);
 	state_save_register_device_item(this, 0, m_flash);
	state_save_register_device_item(this, 0, m_vsync);
	state_save_register_device_item(this, 0, m_vda);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void zx8301_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_VSYNC:
		//m_vsync = !m_vsync;
		devcb_call_write_line(&m_out_vsync_func, m_vsync);
		break;

	case TIMER_FLASH:
		m_flash = !m_flash;
		break;
	}
}


//-------------------------------------------------
//  control_w - display control register
//-------------------------------------------------

WRITE8_MEMBER( zx8301_device::control_w )
{
	/*

		bit		description

		0
		1		display off
		2
		3		graphics mode
		4
		5
		6
		7		display base address

	*/

	if (LOG) logerror("ZX8301 Control: %02x\n", data);

	// display off
	m_dispoff = BIT(data, 1);

	// graphics mode
	m_mode8 = BIT(data, 3);

	// display base address
	m_base = BIT(data, 7);
}


//-------------------------------------------------
//  data_r - RAM read
//-------------------------------------------------

READ8_MEMBER( zx8301_device::data_r )
{
	if (LOG) logerror("ZX8301 RAM Read: %06x\n", offset);

	if (m_vda)
	{
		cpu_spinuntil_time(m_cpu, m_screen->time_until_pos(256, 0));
	}

	return readbyte(offset);
}


//-------------------------------------------------
//  data_w - RAM write
//-------------------------------------------------

WRITE8_MEMBER( zx8301_device::data_w )
{
	if (LOG) logerror("ZX8301 RAM Write: %06x = %02x\n", offset, data);

	if (m_vda)
	{
		cpu_spinuntil_time(m_cpu, m_screen->time_until_pos(256, 0));
	}

	writebyte(offset, data);
}


//-------------------------------------------------
//  draw_line_mode4 - draw mode 4 line
//-------------------------------------------------

void zx8301_device::draw_line_mode4(bitmap_t *bitmap, int y, UINT16 da)
{
	int x = 0;

	for (int word = 0; word < 64; word++)
	{
		UINT8 byte_high = readbyte(da++);
		UINT8 byte_low = readbyte(da++);

		for (int pixel = 0; pixel < 8; pixel++)
		{
			int red = BIT(byte_low, 7);
			int green = BIT(byte_high, 7);
			int color = (green << 1) | red;

			*BITMAP_ADDR16(bitmap, y, x++) = ZX8301_COLOR_MODE4[color];

			byte_high <<= 1;
			byte_low <<= 1;
		}
	}
}


//-------------------------------------------------
//  draw_line_mode8 - draw mode 8 line
//-------------------------------------------------

void zx8301_device::draw_line_mode8(bitmap_t *bitmap, int y, UINT16 da)
{
	int x = 0;

	for (int word = 0; word < 64; word++)
	{
		UINT8 byte_high = readbyte(da++);
		UINT8 byte_low = readbyte(da++);

		for (int pixel = 0; pixel < 4; pixel++)
		{
			int red = BIT(byte_low, 7);
			int green = BIT(byte_high, 7);
			int blue = BIT(byte_low, 6);
			int flash = BIT(byte_high, 6);

			int color = (green << 2) | (red << 1) | blue;

			if (flash && m_flash)
			{
				color = 0;
			}

			*BITMAP_ADDR16(bitmap, y, x++) = color;
			*BITMAP_ADDR16(bitmap, y, x++) = color;

			byte_high <<= 2;
			byte_low <<= 2;
		}
	}
}


//-------------------------------------------------
//  update_screen -
//-------------------------------------------------

void zx8301_device::update_screen(bitmap_t *bitmap, const rectangle *cliprect)
{
	if (!m_dispoff)
	{
		UINT32 da = m_base << 15;

		for (int y = 0; y < 256; y++)
		{
			if (m_mode8)
			{
				draw_line_mode8(bitmap, y, da);
			}
			else
			{
				draw_line_mode4(bitmap, y, da);
			}

			da += 128;
		}
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(machine));
	}
}
