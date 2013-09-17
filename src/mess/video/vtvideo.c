/**********************************************************************

    DEC VT Terminal video emulation
    [ DC012 and DC011 emulation ]

    01/05/2009 Initial implementation [Miodrag Milanovic]
    Sept. 2013 portions by Karl-Ludwig Deisenhofer.

    STATE OF DEC-100 VIDEO AS OF SEPTEMBER 2013
    -------------------------------------------
    - FURTHER TESTING: do line and character attributes match real hardware?  Does soft scrolling work?
    - LIKELY INCORRECT : implementation of double size attribute in 132 columns mode (additional case)

    - MISSING: undocumented features of DC011 / DC012 - see public domain SQUEEZE.COM pokes:
        0f00 => PORT 0C;
        0b00 => PORT 0C;
        1000 => PORT 04
        (SQUEEZE compresses the display in X and Y direction on a real DEC-100 B)

    - IMPROVEMENTS:
        - find a more realistic approach for intensity control (bold attribute)
        - correct phosphor colors (green, white and amber monitors were common)

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "video/vtvideo.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define VERBOSE         0

#define LOG(x)      do { if (VERBOSE) logerror x; } while (0)


const device_type VT100_VIDEO = &device_creator<vt100_video_device>;
const device_type RAINBOW_VIDEO = &device_creator<rainbow_video_device>;


vt100_video_device::vt100_video_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source)
					: device_t(mconfig, type, name, tag, owner, clock, shortname, source),
						device_video_interface(mconfig, *this)
{
}


vt100_video_device::vt100_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
					: device_t(mconfig, VT100_VIDEO, "VT100 Video", tag, owner, clock, "vt100_video", __FILE__),
						device_video_interface(mconfig, *this)
{
}


rainbow_video_device::rainbow_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
					: vt100_video_device(mconfig, RAINBOW_VIDEO, "Rainbow Video", tag, owner, clock, "rainbow_video", __FILE__)
{
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void vt100_video_device::device_config_complete()
{
	// inherit a copy of the static data
	const vt_video_interface *intf = reinterpret_cast<const vt_video_interface *>(static_config());
	if (intf != NULL)
		*static_cast<vt_video_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_in_ram_cb, 0, sizeof(m_in_ram_cb));
		memset(&m_clear_video_cb, 0, sizeof(m_clear_video_cb));
		m_char_rom_tag = "";
	}
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vt100_video_device::device_start()
{
	/* resolve callbacks */
	m_in_ram_func.resolve(m_in_ram_cb, *this);
	m_clear_video_interrupt.resolve(m_clear_video_cb, *this);

	m_gfx = machine().root_device().memregion(m_char_rom_tag)->base();
	assert(m_gfx != NULL);

	// LBA7 is scan line frequency update
	machine().scheduler().timer_pulse(attotime::from_nsec(31778), timer_expired_delegate(FUNC(vt100_video_device::lba7_change),this));


	save_item(NAME(m_lba7));
	save_item(NAME(m_scroll_latch));
	save_item(NAME(m_blink_flip_flop));
	save_item(NAME(m_reverse_field));
	save_item(NAME(m_basic_attribute));
	save_item(NAME(m_columns));
	save_item(NAME(m_height));
	save_item(NAME(m_height_MAX));
	save_item(NAME(m_skip_lines));
	save_item(NAME(m_frequency));
	save_item(NAME(m_interlaced));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void vt100_video_device::device_reset()
{
	palette_set_color_rgb(machine(), 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine(), 1, 0xff, 0xff, 0xff); // white

	m_height = 25;
	m_height_MAX = 25;

	m_lba7 = 0;

	m_scroll_latch = 0;
	m_blink_flip_flop = 0;
	m_reverse_field = 0;
	m_basic_attribute = 0;

	m_columns = 80;
	m_frequency = 60;
	m_interlaced = 1;
	m_skip_lines = 2; // for 60Hz
}

// ****** RAINBOW ******
// 3 color palette, 24 and 48 line modes.
void rainbow_video_device::device_reset()
{
	palette_set_color_rgb(machine(), 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine(), 1, 213, 146, 82);      // AMBER (not exact)
	palette_set_color_rgb(machine(), 2, 255, 193, 129);     // AMBER (brighter)

	m_height = 24;  // <---- DEC-100
	m_height_MAX = 48;

	m_lba7 = 0;

	m_scroll_latch = 0;
	m_blink_flip_flop = 0;
	m_reverse_field = 0;
	m_basic_attribute = 0;

	m_columns = 80;
	m_frequency = 60;
	m_interlaced = 1;
	m_skip_lines = 2; // for 60Hz
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

// Also used by Rainbow-100 ************
void vt100_video_device::recompute_parameters()
{
	int horiz_pix_total = 0;
	int vert_pix_total = 0;
	rectangle visarea;

	vert_pix_total  = m_height  * 10;

	if (m_columns == 132) {
		horiz_pix_total = m_columns * 9; // display 1 less filler pixel in 132 char. mode (DEC-100)
	} else {
		horiz_pix_total = m_columns * 10; // normal 80 character mode.
	}

	visarea.set(0, horiz_pix_total - 1, 0, vert_pix_total - 1);

	m_screen->configure(horiz_pix_total, vert_pix_total, visarea, m_screen->frame_period().attoseconds);
}


READ8_MEMBER( vt100_video_device::lba7_r )
{
	return m_lba7;
}

// Also used by Rainbow-100 ************
WRITE8_MEMBER( vt100_video_device::dc012_w )
{
	if (!(data & 0x08))
	{
		if (!(data & 0x04))
		{
			// set lower part scroll
			m_scroll_latch = (m_scroll_latch & 0x0c) | (data & 0x03);
		}
		else
		{
			// set higher part scroll
			m_scroll_latch = (m_scroll_latch & 0x03) | ((data & 0x03) << 2);
		}
	}
	else
	{
		switch (data & 0x0f)
		{
			case 0x08:
				// toggle blink flip flop
				m_blink_flip_flop = !(m_blink_flip_flop) ? 1 : 0;
				break;
			case 0x09:
				// clear vertical frequency interrupt;
				m_clear_video_interrupt(0, 0);
				break;
			case 0x0a:
				// set reverse field on
				m_reverse_field = 1;
				break;
			case 0x0b:
				// set reverse field off
				m_reverse_field = 0;
				break;
			case 0x0c:
				// set basic attribute to underline
				m_basic_attribute = 0;
				m_blink_flip_flop = 0;
				break;
			case 0x0d:
				// set basic attribute to reverse video / blink flip-flop off
				m_basic_attribute = 1;
				m_blink_flip_flop = 0;

				if (m_height_MAX == 25) break; // VT 100.

				if (m_height != 24)
				{
					m_height = 24;  // (DEC Rainbow 100) : 24 line display
					recompute_parameters();
				}
				break;

			case 0x0e:
				break;                  // (DC12) : 'not supported'

			case 0x0f:
				// (DEC Rainbow 100) : set basic attribute to reverse video / blink flip-flop off
				m_basic_attribute = 1;
				m_blink_flip_flop = 0;

				if (m_height_MAX == 25) break; // VT 100.

				if (m_height != 48)
				{
					m_height = 48;   // (DEC Rainbow 100) : 48 line display
					recompute_parameters();
				}
				break;
		}
	}
}


WRITE8_MEMBER( vt100_video_device::dc011_w )
{
	if (!BIT(data, 5))
	{
		UINT8 col = m_columns;
		if (!BIT(data, 4))
		{
			m_columns = 80;
		}
		else
		{
			m_columns = 132;
		}
		if (col != m_columns)
		{
			recompute_parameters();
		}
		m_interlaced = 1;
	}
	else
	{
		if (!BIT(data, 4))
		{
			m_frequency = 60;
			m_skip_lines = 2;
		}
		else
		{
			m_frequency = 50;
			m_skip_lines = 5;
		}
		m_interlaced = 0;
	}
}

WRITE8_MEMBER( vt100_video_device::brightness_w )
{
	//palette_set_color_rgb(machine(), 1, data, data, data);
}



void vt100_video_device::display_char(bitmap_ind16 &bitmap, UINT8 code, int x, int y, UINT8 scroll_region, UINT8 display_type)
{
	UINT8 line = 0;
	int bit = 0, prevbit, invert = 0, j;
	int double_width = (display_type == 2) ? 1 : 0;

	for (int i = 0; i < 10; i++)
	{
		switch (display_type)
		{
			case 0 : // bottom half, double height
						j = (i >> 1) + 5; break;
			case 1 : // top half, double height
						j = (i >> 1); break;
			case 2 : // double width
			case 3 : // normal
						j = i;  break;
			default : j = 0; break;
		}
		// modify line since that is how it is stored in rom
		if (j == 0) j = 15; else j = j - 1;

		line = m_gfx[(code & 0x7f) * 16 + j];

		if (m_basic_attribute == 1)
		{
			if ((code & 0x80) == 0x80)
				invert = 1;
			else
				invert = 0;
		}

		for (int b = 0; b < 8; b++)
		{
			prevbit = bit;
			bit = BIT((line << b), 7);
			if (double_width)
			{
				bitmap.pix16(y * 10 + i, x * 20 + b * 2)     =  (bit | prevbit) ^ invert;
				bitmap.pix16(y * 10 + i, x * 20 + b * 2 + 1) =  bit ^ invert;
			}
			else
			{
				bitmap.pix16(y * 10 + i, x * 10 + b) =  (bit | prevbit) ^ invert;
			}
		}
		prevbit = bit;
		// char interleave is filled with last bit
		if (double_width)
		{
			bitmap.pix16(y * 10 + i, x * 20 + 16) =  (bit | prevbit) ^ invert;
			bitmap.pix16(y * 10 + i, x * 20 + 17) =  bit ^ invert;
			bitmap.pix16(y * 10 + i, x * 20 + 18) =  bit ^ invert;
			bitmap.pix16(y * 10 + i, x * 20 + 19) =  bit ^ invert;
		}
		else
		{
			bitmap.pix16(y * 10 + i, x * 10 + 8) =  (bit | prevbit) ^ invert;
			bitmap.pix16(y * 10 + i, x * 10 + 9) =  bit ^ invert;
		}
	}
}

void vt100_video_device::video_update(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	UINT16 addr = 0;
	int line = 0;
	int xpos = 0;
	int ypos = 0;
	UINT8 code;
	int x = 0;
	UINT8 scroll_region = 1; // binary 1
	UINT8 display_type = 3;  // binary 11
	UINT16 temp = 0;

	if (m_in_ram_func(0) != 0x7f)
		return;

	while (line < (m_height + m_skip_lines))
	{
		code = m_in_ram_func(addr + xpos);
		if (code == 0x7f)
		{
			// end of line, fill empty till end of line
			if (line >= m_skip_lines)
			{
				for (x = xpos; x < ((display_type == 2) ? (m_columns / 2) : m_columns); x++)
				{
					display_char(bitmap, code, x, ypos, scroll_region, display_type);
				}
			}
			// move to new data
			temp = m_in_ram_func(addr + xpos + 1) * 256 + m_in_ram_func(addr + xpos + 2);
			addr = (temp) & 0x1fff;
			// if A12 is 1 then it is 0x2000 block, if 0 then 0x4000 (AVO)
			if (addr & 0x1000) addr &= 0xfff; else addr |= 0x2000;
			scroll_region = (temp >> 15) & 1;
			display_type  = (temp >> 13) & 3;
			if (line >= m_skip_lines)
			{
				ypos++;
			}
			xpos = 0;
			line++;
		}
		else
		{
			// display regular char
			if (line >= m_skip_lines)
			{
				display_char(bitmap, code, xpos, ypos, scroll_region, display_type);
			}
			xpos++;
			if (xpos > m_columns)
			{
				line++;
				xpos = 0;
			}
		}
	}

}

// ****** RAINBOW ******
// 5 possible character states (normal, reverse, bold, blink, underline) are encoded into display_type.
void rainbow_video_device::display_char(bitmap_ind16 &bitmap, UINT8 code, int x, int y, UINT8 scroll_region, UINT8 display_type)
{
	UINT8 xsize, d_xsize;
	if (m_columns == 132)
	{     xsize = 9;
			d_xsize = 18;
	} else
	{
			xsize = 10;
			d_xsize = 20;
	}

	UINT8 line = 0;
	int bit = 0, j=0, invert, bold, blink, underline;

	invert = display_type &  8;        // BIT 3 indicates REVERSE
	bold   = (display_type & 16) >> 4; // BIT 4 indicates BOLD
	blink  = display_type &  32;       // BIT 5 indicates BLINK
	underline = display_type & 64;     // BIT 6 indicates UNDERLINE
	display_type = display_type & 3;

	int double_width = (display_type == 1) ? 1 : 0;

	for (int i = 0; i < 10; i++)
	{
		switch (display_type)
		{
			case 0 : // bottom half, double height
						j = (i >> 1) + 5; break;
			case 2 : // top half, double height
						j = (i >> 1); break;

			default : j = i; break; // 1: double width  /  3: normal
		}
		// modify line since that is how it is stored in rom
		if (j == 0) j = 15; else j = j - 1;

		line = m_gfx[code * 16 + j];

		if ( i == 8 )
		{
			if ( underline != 0  ) line = 0xff;
		}

		//  Code to handle basic attribute from VT-100
		if ( m_basic_attribute == 1 )
		{
			if ((code & 0x80) == 0x80)
				invert = 1;
			else
				invert = 0;
			}

			if (m_blink_flip_flop > 0)
		{
			if ( blink != 0 )
			{
				line = line ^ 0xff;
			}
		}
		if (invert != 0)
				line = line ^ 0xff;

		for (int b = 0; b < 8; b++)
		{
			bit = BIT((line << b), 7) << bold;
			if (double_width)
			{
				bitmap.pix16(y * 10 + i, x * d_xsize + b * 2)     = bit;
				bitmap.pix16(y * 10 + i, x * d_xsize + b * 2 + 1) = bit;
			}
			else
			{
				bitmap.pix16(y * 10 + i, x * xsize + b) = bit;
			}
		}


		// char interleave is filled with last bit
		if (double_width)
		{
			bitmap.pix16(y * 10 + i, x * d_xsize + 16) = bit;
			bitmap.pix16(y * 10 + i, x * d_xsize + 17) = bit;
			bitmap.pix16(y * 10 + i, x * d_xsize + 18) = bit;
			bitmap.pix16(y * 10 + i, x * d_xsize + 19) = bit;
		}
		else
		{
			bitmap.pix16(y * 10 + i, x * xsize + 8) = bit;
			if (m_columns == 80)
				bitmap.pix16(y * 10 + i, x * xsize + 9) = bit;
		}


	}
}

// ****** RAINBOW ******
void rainbow_video_device::video_update(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	UINT16 addr = 0;
	UINT16 attr_addr = 0;
	int line = 0;
	int xpos = 0;
	int ypos = 0;
	UINT8 code;
	int x = 0;
	UINT8 scroll_region = 1; // binary 1
	UINT8 display_type = 3;  // binary 11
	UINT16 temp = 0;

	while (line < (m_height + m_skip_lines))
	{
		code = m_in_ram_func(addr + xpos);
		if (code == 0xff)
		{
			// end of line, fill empty till end of line
			if (line >= m_skip_lines)
			{
				// NOTE: display_type is already SHIFTED by 1  ( 1 = DOUBLE WIDTH 40 / 66 )
				for (x = xpos; x < ((display_type == 1) ? (m_columns / 2) : m_columns); x++)
				{
					display_char(bitmap, code, x, ypos, scroll_region, display_type);
				}
			}
			// move to new data
			temp = m_in_ram_func(addr + xpos + 2) * 256 + m_in_ram_func(addr + xpos + 1);

			addr = (temp) & 0x0fff;
			attr_addr = ((temp) & 0x1fff) - 2;

			//  No AVO here.
			attr_addr |= 0x1000;
			if (attr_addr > 0x2000) // Ignore attributes beyond 8192 byte limit (SRAM).
			{
				scroll_region = 1; // binary 1   <- SET DEFAULTS
				display_type = 3;  // binary 111
			} else
			{
				temp = m_in_ram_func(attr_addr);
				scroll_region = (temp) & 1;
				display_type  = (temp >> 1) & 3;
			}

			if (line >= m_skip_lines)
			{
				ypos++;
			}
			xpos = 0;
			line++;
		}
		else
		{
			// display regular char
			if (line >= m_skip_lines)
			{
				attr_addr = 0x1000 | ( (addr + xpos) & 0x0fff );
				temp = m_in_ram_func(attr_addr); // get character attribute

				// TODO: check if reverse bit is treated the same way on real hardware
				// 1 = display char. in REVERSE   (encoded as 8)
				// 0 = display char. in BOLD      (encoded as 16)
				// 0 = display char. w. BLINK     (encoded as 32)
				// 0 = display char. w. UNDERLINE (encoded as 64).
					display_char(bitmap, code, xpos, ypos, scroll_region, display_type | ( (  (temp & 1)) << 3 )
																					| ( (2-(temp & 2)) << 3 )
																					| ( (4-(temp & 4)) << 3 )
																					| ( (8-(temp & 8)) << 3 )
																					);
			}
			xpos++;
			if (xpos > m_columns)
			{
				line++;
				xpos = 0;
			}
		}
	}

}

TIMER_CALLBACK_MEMBER( vt100_video_device::lba7_change )
{
	m_lba7 = (m_lba7) ? 0 : 1;
}
