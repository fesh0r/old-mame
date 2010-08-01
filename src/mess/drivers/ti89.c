/***************************************************************************

    TI-89, TI-92 and TI-92 plus driver by Sandro Ronco

    NVRAM works only if the calculator is turned off (2nd + ON) before closing of MESS

    TODO:
     -Link
     -HW 3 I/O port
     -RTC
     -LCD contrast
****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "includes/ti89.h"

static UINT8 ti_68k_keypad_r (running_machine *machine);

class t68k_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, t68k_state(machine)); }

	t68k_state(running_machine &machine) { }

	/* HW specifications */
	UINT8 hw_version;
	UINT8 mem_type;
	UINT32 initial_pc;
	UINT16 *ram;
	UINT32 ram_size;

	/* keyboard */
	UINT16 kb_mask;
	UINT8 on_key;

	/* LCD */
	UINT8 lcd_on;
	UINT32 lcd_base;
	UINT8 lcd_width;
	UINT8 lcd_height;
	UINT8 lcd_contrast;

	/* Flash */
	UINT8 flash_protect;
	UINT8 flash_state;
	UINT8 flash_wait;
	UINT8 flash_ready;

	/* I/O */
	UINT16 io_hw1[0x10];
	UINT16 io_hw2[0x80];

	/* Timer */
	UINT8 timer_on;
	UINT8 timer_val;
	UINT16 timer_mask;
};


static UINT8 ti_68k_keypad_r (running_machine *machine)
{
	t68k_state *state = (t68k_state *)machine->driver_data;
	UINT8 port, bit, data = 0xff;
	static const char *const bitnames[] = {"BIT0", "BIT1", "BIT2", "BIT3", "BIT4", "BIT5", "BIT6", "BIT7"};

	for (bit = 0; bit < 10; bit++)
		if (~state->kb_mask & (0x01 << bit))
			for (port = 0; port < 8; port++)
				data ^= input_port_read(machine, bitnames[port]) & (0x01 << bit) ? 0x01 << port : 0x00;
	return data;
}


WRITE16_HANDLER ( ti68k_io_w )
{
	t68k_state *state = (t68k_state *)space->machine->driver_data;

	switch(offset & 0x0f)
	{
		case 0x00:
			state->lcd_contrast=(state->lcd_contrast & 0xfe) | BIT(data, 5);
			break;
		case 0x02:
			if (!(data & 0x10) && data != state->io_hw1[offset])
				cputag_suspend(space->machine, "maincpu", SUSPEND_REASON_DISABLE, 1);
			break;
		case 0x08:
				state->lcd_base = data << 3;
			break;
		case 0x09:
				state->lcd_width = (0x40 - ((data >> 8) & 0xff)) * 0x10;
				state->lcd_height = 0x100 - (data & 0xff);
			break;
		case 0x0a:
			state->timer_on = BIT(data ,1);
			switch((data >> 4) & 0x03)
			{
				case 0:		state->timer_mask = 0x0000;	break;
				case 1:		state->timer_mask = 0x000f;	break;
				case 2:		state->timer_mask = 0x007f;	break;
				case 3:		state->timer_mask = 0x1fff;	break;
			}
			break;
		case 0x0b:
			state->timer_val = data & 0xff;
		case 0x0c:
			state->kb_mask = data & 0x03ff;
			break;
		case 0x0e:
			state->lcd_on = (((data >> 8) & 0x3c) == 0x3c) ? 0 : 1;
			state->lcd_contrast = (state->lcd_contrast & 0x01) | ((data & 0x0f) << 1);
			break;
		default: break;
	}
	state->io_hw1[offset & 0x0f] = data;
}


READ16_HANDLER ( ti68k_io_r )
{
	t68k_state *state = (t68k_state *)space->machine->driver_data;
	UINT16 data;

	switch (offset & 0x0f)
	{
		case 0x00:
			data = 0x0400 | ((state->lcd_contrast & 1) << 13);
			break;
		case 0x0b:
			data = state->timer_val;
			break;
		case 0x0d:
			data = ((!state->on_key) << 9) | ti_68k_keypad_r(space->machine);
			break;
		default:
			data= state->io_hw1[offset & 0x0f];
	}
	return data;
}


WRITE16_HANDLER ( ti68k_io2_w )
{
	t68k_state *state = (t68k_state *)space->machine->driver_data;
	switch(offset & 0x7f)
	{
		case 0x0b:
			state->lcd_base = 0x4c00 + 0x1000 * (data & 0x03);
			break;
		case 0x0e:
			state->lcd_on = (data & 0x02) ? 1 : 0;
			break;
		default: break;
	}
	state->io_hw2[offset & 0x7f] = data;
}


READ16_HANDLER ( ti68k_io2_r )
{
	t68k_state *state = (t68k_state *)space->machine->driver_data;
	UINT16 data;

	switch (offset & 0x7f)
	{
		default:
			data= state->io_hw2[offset & 0x7f];
	}
	return data;
}


WRITE16_HANDLER ( flash_w )
{
	t68k_state *state = (t68k_state *)space->machine->driver_data;
	UINT16 *flash_base = (UINT16 *)memory_region(space->machine, "maincpu");

	/* verification if it is flash memory */
	if (state->mem_type != FLASH)
		return;

	if (state->flash_ready)
	{
		flash_base[offset] &= data;
		state->flash_ready = 0;
		state->flash_wait = 1;
	}
	else if (data == 0x5050)
		state->flash_state = 0x50;
	else if (data == 0x1010)
	{
		state->flash_ready = 1;
		state->flash_state = 0x50;
	}
	else if (data == 0x2020 && state->flash_state == 0x50)
		state->flash_state = 0x20;
	else if (data == 0xd0d0 && state->flash_state == 0x20)
	{
		state->flash_state = 0xd0;
		state->flash_wait = 1;
		memset(&flash_base[offset & 0xff0000], 0xff, 64 * 1024);
	}
	else if (data == 0xffff && state->flash_state == 0x50)
	{
		state->flash_ready = 0;
		state->flash_wait = 0;
	}
	else if (data == 0x9090)
		state->flash_state = 0x90;
}


READ16_HANDLER ( rom_r )
{
	t68k_state *state = (t68k_state *)space->machine->driver_data;
	UINT16 *rom_base = (UINT16 *)memory_region(space->machine, "maincpu");

	if (state->mem_type == FLASH)
	{
		if (state->flash_state == 0x90 )
			switch(offset & 0xffff)
			{
				case 0:
					return 0x8900;
				case 2:
					return 0xb500;
				default:
					return 0xffff;
			}
		else if (state->flash_wait)
			return 0xffff;
		else
			return rom_base[offset];
	}
	else
		return rom_base[offset];
}


static TIMER_CALLBACK( ti68k_timer_callback )
 {
	t68k_state *state = (t68k_state *)machine->driver_data;
	static UINT64 timer;

	timer++;

	if (state->timer_on)
	{
		if (!(timer & state->timer_mask) && BIT(state->io_hw1[0x0a], 3))
		{
			if (state->timer_val)
				state->timer_val++;
			else
				state->timer_val = (state->io_hw1[0x0b]) & 0xff;
		}

		if (!BIT(state->io_hw1[0x0a], 7) && ((state->hw_version == HW1) || (!BIT(state->io_hw1[0x0f], 2) && !BIT(state->io_hw1[0x0f], 1))))
		{
			if (!(timer & 0x003f))
				cputag_set_input_line(machine, "maincpu", M68K_IRQ_1, HOLD_LINE);

			if (!(timer & 0x3fff) && !BIT(state->io_hw1[0x0a], 3))
				cputag_set_input_line(machine, "maincpu", M68K_IRQ_3, HOLD_LINE);

			if (!(timer & state->timer_mask) && BIT(state->io_hw1[0x0a], 3) && state->timer_val == 0)
				cputag_set_input_line(machine, "maincpu", M68K_IRQ_5, HOLD_LINE);
		}
	}

	if (ti_68k_keypad_r(machine) != 0xff)
		cputag_set_input_line(machine, "maincpu", M68K_IRQ_2, HOLD_LINE);
}


static ADDRESS_MAP_START(ti92_mem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x0fffff) AM_RAM AM_BASE_SIZE_MEMBER(t68k_state, ram, ram_size)
	AM_RANGE(0x200000, 0x5fffff) AM_UNMAP	// ROM
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(ti68k_io_r, ti68k_io_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(ti89_mem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x0fffff) AM_RAM AM_BASE_SIZE_MEMBER(t68k_state, ram, ram_size)
	AM_RANGE(0x200000, 0x3fffff) AM_READWRITE(rom_r, flash_w)
	AM_RANGE(0x400000, 0x5fffff) AM_NOP
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(ti68k_io_r, ti68k_io_w)
	AM_RANGE(0x700000, 0x7fffff) AM_READWRITE(ti68k_io2_r, ti68k_io2_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(ti92p_mem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x0fffff) AM_RAM AM_BASE_SIZE_MEMBER(t68k_state, ram, ram_size)
	AM_RANGE(0x200000, 0x3fffff) AM_NOP
	AM_RANGE(0x400000, 0x5fffff) AM_READWRITE(rom_r, flash_w)
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(ti68k_io_r, ti68k_io_w)
	AM_RANGE(0x700000, 0x7fffff) AM_READWRITE(ti68k_io2_r, ti68k_io2_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(v200_mem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x0fffff) AM_RAM AM_BASE_SIZE_MEMBER(t68k_state, ram, ram_size)
	AM_RANGE(0x200000, 0x5fffff) AM_READWRITE(rom_r, flash_w)
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(ti68k_io_r, ti68k_io_w)
	AM_RANGE(0x700000, 0x70ffff) AM_READWRITE(ti68k_io2_r, ti68k_io2_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(ti89t_mem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x0fffff) AM_RAM AM_MIRROR(0x200000) AM_BASE_SIZE_MEMBER(t68k_state, ram, ram_size)
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(ti68k_io_r, ti68k_io_w)
	AM_RANGE(0x700000, 0x70ffff) AM_READWRITE(ti68k_io2_r, ti68k_io2_w)
	AM_RANGE(0x800000, 0xbfffff) AM_READWRITE(rom_r, flash_w)
	AM_RANGE(0xbf0000, 0xffffff) AM_NOP
ADDRESS_MAP_END


static INPUT_CHANGED( ti68k_on_key )
{
	t68k_state *state = (t68k_state *)field->port->machine->driver_data;

	state->on_key = newval;

	if (state->on_key)
	{
		if (field->port->machine->device<cpu_device>("maincpu")->suspended(SUSPEND_REASON_DISABLE))
			cputag_resume(field->port->machine, "maincpu", SUSPEND_REASON_DISABLE);

		cputag_set_input_line(field->port->machine, "maincpu", M68K_IRQ_6, HOLD_LINE);
	}
}


/* Input ports for TI-89 and TI-89 Titanium*/
static INPUT_PORTS_START (ti8x)
	PORT_START("BIT0")   /* bit 0 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER  (ENTRY)") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(-)") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("APPS") PORT_CODE(KEYCODE_F11)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_START("BIT1")   /* bit 1 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STORE") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ON") PORT_CODE(KEYCODE_F10) PORT_CHANGED(ti68k_on_key, 0)
	PORT_START("BIT2")   /* bit 2 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("EE") PORT_CODE(KEYCODE_E)
	PORT_START("BIT3")   /* bit 3 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("|") PORT_CODE(KEYCODE_COLON)
	PORT_START("BIT4")   /* bit 4 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2nd") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(")") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
	PORT_START("BIT5")   /* bit 5 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_START("BIT6")   /* bit 6 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3nd") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("<--") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CATALOG") PORT_CODE(KEYCODE_F8)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Mode") PORT_CODE(KEYCODE_F12)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
	PORT_START("BIT7")   /* bit 7 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ALPHA") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
INPUT_PORTS_END

/* Input ports for TI-92 and TI92 Plus*/
static INPUT_PORTS_START (ti9x)
	PORT_START("BIT0")   /* bit 0 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2nd") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_UNUSED
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_UNUSED
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STORE") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("<--") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_START("BIT1")   /* bit 1 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3nd") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("o") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_START("BIT2")   /* bit 2 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_START("BIT3")   /* bit 3 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Clock") PORT_CODE(KEYCODE_F11)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_START("BIT4")   /* bit 4 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_PLUS_PAD)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_START("BIT5")   /* bit 5 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SIN") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LN") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MODE") PORT_CODE(KEYCODE_F12)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_START("BIT6")   /* bit 6 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(")") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("COS") PORT_CODE(KEYCODE_END)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("APPS") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_START("BIT7")   /* bit 7 */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAN") PORT_CODE(KEYCODE_INSERT)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_ASTERISK)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ON") PORT_CODE(KEYCODE_F10) PORT_CHANGED(ti68k_on_key, 0)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(-)") PORT_CODE(KEYCODE_MINUS_PAD)
INPUT_PORTS_END


NVRAM_HANDLER( ti68k )
{
	t68k_state *state = (t68k_state *)machine->driver_data;
	UINT16 *rom = (UINT16 *)memory_region(machine, "maincpu");

	if (read_or_write)
	{
		mame_fwrite(file, state->ram, state->ram_size);

		/* if the memory is flash saves the content */
		if (state->mem_type == FLASH)
			mame_fwrite(file, rom, 0x200000);
	}
	else
	{
		if (file)
		{
			mame_fread(file, state->ram, state->ram_size);
			if (state->mem_type == FLASH)
				mame_fread(file, rom, 0x200000);
		}
		else
			memset(state->ram, 0, sizeof(state->ram));
	}
}


static MACHINE_RESET(ti68k)
{
	t68k_state *state = (t68k_state *)machine->driver_data;
	cpu_set_reg(machine->device("maincpu"), M68K_PC, state->initial_pc);

	state->kb_mask = 0xff;
	state->on_key = 0;
	state->lcd_base = 0;
	state->lcd_width = 0;
	state->lcd_height = 0;
	state->lcd_on = 0;
	state->lcd_contrast = 0;
	state->flash_protect = 0;
	state->flash_state = 0;
	state->flash_wait = 0;
	state->flash_ready = 0;
	memset(state->io_hw1, 0, sizeof(state->io_hw1));
	memset(state->io_hw2, 0, sizeof(state->io_hw2));
	state->timer_on = 0;
	state->timer_mask = 0xf;
	state->timer_val = 0;
}

/* video */

static VIDEO_START( ti68k )
{

}

static VIDEO_UPDATE( ti68k )
{
	/* preliminary implementation, doesn't use the contrast value */
	t68k_state *state = (t68k_state *)screen->machine->driver_data;
	const address_space *space = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 width = screen->width();
	UINT8 height = screen->height();
	UINT8 x, y, b;

	if (!state->lcd_on || !state->lcd_base)
		bitmap_fill(bitmap, NULL, 1);
	else
		for (y = 0; y < height; y++)
			for (x = 0; x < width / 8; x++)
			{
				UINT8 s_byte= memory_read_byte(space, state->lcd_base + y * (width/8) + x);
				for (b = 0; b<8; b++)
					*BITMAP_ADDR16(bitmap, y, x * 8 + (7 - b)) = (BIT(s_byte, b) > 0) ? 0 : 1;
			}

	return 0;
}


MACHINE_START( ti68k )
{
	t68k_state *state = (t68k_state *)machine->driver_data;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 *rom = (UINT16 *)memory_region(space->machine, "maincpu");
	int i;

	state->mem_type= (rom[0x32] & 0x0f) ? EPROM : FLASH;

	if (state->mem_type == FLASH)
	{
		UINT32 base = ((((rom[0x82]) << 16) | rom[0x83]) & 0xffff)>>1;

		if (rom[base] >= 8)
			state->hw_version = ((rom[base + 0x0b]) << 16) | rom[base + 0x0c];

		if (!state->hw_version)
			state->hw_version = HW1;

		for (i = 0x9000; i < 0x100000; i++)
			if (rom[i] == 0xcccc && rom[i + 1] == 0xcccc)
				break;

		state->initial_pc = ((rom[i + 4]) << 16) | rom[i + 5];
	}
	else
	{
		state->hw_version = HW1;
		state->initial_pc = ((rom[2]) << 16) | rom[3];

		memory_unmap_read(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x200000, 0x5fffff, 0, 0);

		if (state->initial_pc > 0x400000)
			memory_install_read16_handler( cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x400000, 0x5fffff, 0, 0, rom_r );
		else
			memory_install_read16_handler( cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x200000, 0x3fffff, 0, 0, rom_r );

	}

	timer_pulse(machine, ATTOTIME_IN_HZ(1<<14), NULL, 0, ti68k_timer_callback);
}


static MACHINE_DRIVER_START( ti89 )

	MDRV_DRIVER_DATA(t68k_state)

    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_10MHz)
    MDRV_CPU_PROGRAM_MAP(ti89_mem)

    MDRV_MACHINE_RESET(ti68k)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(240, 128)
    MDRV_SCREEN_VISIBLE_AREA(0, 160-1, 0, 100-1)
    MDRV_PALETTE_LENGTH(2)
	MDRV_DEFAULT_LAYOUT(layout_lcd)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ti68k)
    MDRV_VIDEO_UPDATE(ti68k)

	MDRV_MACHINE_START( ti68k )

	MDRV_NVRAM_HANDLER(ti68k)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ti92 )

	MDRV_IMPORT_FROM( ti89 )

    MDRV_CPU_MODIFY("maincpu")
    MDRV_CPU_PROGRAM_MAP(ti92_mem)

    /* video hardware */
    MDRV_SCREEN_MODIFY("screen")
    MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 128-1)


MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ti92p )

	MDRV_IMPORT_FROM( ti89 )

    MDRV_CPU_REPLACE("maincpu",M68000, XTAL_12MHz)
    MDRV_CPU_PROGRAM_MAP(ti92p_mem)

    /* video hardware */
    MDRV_SCREEN_MODIFY("screen")
    MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 128-1)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( v200 )

	MDRV_IMPORT_FROM( ti89 )

    MDRV_CPU_REPLACE("maincpu",M68000, XTAL_12MHz)
    MDRV_CPU_PROGRAM_MAP(v200_mem)

    /* video hardware */
    MDRV_SCREEN_MODIFY("screen")
    MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 128-1)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ti89t )

	MDRV_IMPORT_FROM( ti89 )

    MDRV_CPU_REPLACE("maincpu",M68000, XTAL_16MHz)
    MDRV_CPU_PROGRAM_MAP(ti89t_mem)

MACHINE_DRIVER_END


/* ROM definition */
ROM_START( ti89 )
  ROM_REGION( 0x200000, "maincpu", ROMREGION_ERASEFF )
  ROM_SYSTEM_BIOS( 0, "v100", "V 1.00 - HW1" )
  ROMX_LOAD( "ti89v100.rom",   0x000000, 0x200000, CRC(264b34ad) SHA1(c87586a7e9b6d49fbe908fbb6f3c0038f3498573), ROM_BIOS(1))
  ROM_SYSTEM_BIOS( 1, "v100a", "V 1.00 [a] - HW1" )
  ROMX_LOAD( "ti89v100a.rom",  0x000000, 0x200000, CRC(95199934) SHA1(b8e3cdeb4705b0c7e0a15ab6c6f62bcde14a3a55), ROM_BIOS(2))
  ROM_SYSTEM_BIOS( 2, "v100m", "V 1.00 [m] - HW1" )
  ROMX_LOAD( "ti89v100m.rom",  0x000000, 0x200000, CRC(b9059e06) SHA1(b33a7c2935eb9f73b210bcf6e7c7f32d1548a9d5), ROM_BIOS(3))
  ROM_SYSTEM_BIOS( 3, "v100m2", "V 1.00 [m2] - HW1" )
  ROMX_LOAD( "ti89v100m2.rom", 0x000000, 0x200000, CRC(cdd69d34) SHA1(1686362b0997bb9597f39b443490d4d8d85b56cc), ROM_BIOS(4))
  ROM_SYSTEM_BIOS( 4, "v105", "V 1.05 - HW1" )
  ROMX_LOAD( "ti89v105.rom",   0x000000, 0x200000, CRC(3bc0b474) SHA1(46fe0cd511eb81d53dc12cc4bdacec8a5bba5171), ROM_BIOS(5))
  ROM_SYSTEM_BIOS( 5, "v201", "V 2.01 - HW1" )
  ROMX_LOAD( "ti89v201.rom",   0x000000, 0x200000, CRC(fa6745e9) SHA1(e4ee6067df9b975356cef6c5a81d0ec664371c1d), ROM_BIOS(6))
  ROM_SYSTEM_BIOS( 6, "v203", "V 2.03 - HW1" )
  ROMX_LOAD( "ti89v203.rom",   0x000000, 0x200000, CRC(a3a74eca) SHA1(55aae3561722a96973b430e3d4cb4f513298ea4e), ROM_BIOS(7))
  ROM_SYSTEM_BIOS( 7, "v203m", "V 2.03 [m] - HW 1" )
  ROMX_LOAD( "ti89v203m.rom",  0x000000, 0x200000, CRC(d79068f7) SHA1(5b6f571417889b11ae19eef99a5fda4f027d5ec2), ROM_BIOS(8))
  ROM_SYSTEM_BIOS( 8, "v209",  "V 2.09 - HW 1" )
  ROMX_LOAD( "ti89v209.rom",   0x000000, 0x200000, CRC(f76f9c15) SHA1(66409ef4b20190a3b7c0d48cbd30257580b47dcd), ROM_BIOS(9))
  ROM_SYSTEM_BIOS( 9, "v105-2","V 1.05 - HW2" )
  ROMX_LOAD( "ti89v105-2.rom", 0x000000, 0x200000, CRC(83817402) SHA1(b2ddf785e973cc3f9a437d058a68abdf7ca52ea2), ROM_BIOS(10))
  ROM_SYSTEM_BIOS( 10, "v203-2",  "V 2.03 - HW2" )
  ROMX_LOAD( "ti89v203-2.rom", 0x000000, 0x200000, CRC(5e0400a9) SHA1(43c608ee72f15aed56cb5762948ec6a3c93dd9d8), ROM_BIOS(11))
  ROM_SYSTEM_BIOS( 11, "v203m-2", "V 2.03 [m] - HW2" )
  ROMX_LOAD( "ti89v203m-2.rom", 0x000000, 0x200000, CRC(04d5d76d) SHA1(14ca44b64c29aa1bf274508ca40fe69224f5a7cc), ROM_BIOS(12))
  ROM_SYSTEM_BIOS( 12, "v205-2", "V 2.05 - HW2" )
  ROMX_LOAD( "ti89v205-2.rom", 0x000000, 0x200000, CRC(37c4653c) SHA1(f48d00a57430230e489e243383513485009b1b98), ROM_BIOS(13))
  ROM_SYSTEM_BIOS( 13, "v205-2m", "V 2.05[m] - HW2" )
  ROMX_LOAD( "ti89v205m-2.rom", 0x000000, 0x200000, CRC(e58a23f9) SHA1(d4cb23fb4b414a43802c37dc3c572a8ede670e0f), ROM_BIOS(14))
  ROM_SYSTEM_BIOS( 14, "v205-2m2","V 2.05[m2] - HW2" )
  ROMX_LOAD( "ti89v205m2-2.rom", 0x000000, 0x200000, CRC(a8ba976c) SHA1(38bd25ada5e2066c64761d1008a9327a37d68654), ROM_BIOS(15))
  ROM_SYSTEM_BIOS( 15,"v209-2", "V 2.09 - HW2" )
  ROMX_LOAD( "ti89v209-2.rom", 0x000000, 0x200000, CRC(242a238f) SHA1(9668df314a0180ef210796e9cb651c5e9f17eb07), ROM_BIOS(16))
ROM_END

ROM_START( ti92 )
  ROM_REGION( 0x200000, "maincpu", ROMREGION_ERASEFF )
  ROM_SYSTEM_BIOS( 0, "v111", "V 1.11" )
  ROMX_LOAD( "ti92v111.rom",  0x000000, 0x100000, CRC(67878d52) SHA1(c0fdf162961922a76f286c93fd9b861ce20f23a3), ROM_BIOS(1))
  ROM_SYSTEM_BIOS( 1, "v13", "V 1.3" )
  ROMX_LOAD( "ti92v13e.rom",  0x000000, 0x100000, CRC(316c8196) SHA1(82c8cd484c6aebe36f814a2023d2afad6d87f840), ROM_BIOS(2))
  ROM_SYSTEM_BIOS( 2, "v14", "V 1.4" )
  ROMX_LOAD( "ti92v14e.rom",  0x000000, 0x100000, CRC(239e9405) SHA1(df2f1ab17d490fda43a02f5851b5a15052361b28), ROM_BIOS(3))
  ROM_SYSTEM_BIOS( 3, "v17", "V 1.7" )
  ROMX_LOAD( "ti92v17e.rom",  0x000000, 0x100000, CRC(83e27cc5) SHA1(aec5a6a6157ff94a4e665fa3fe747bacb6688cd4), ROM_BIOS(4))
  ROM_SYSTEM_BIOS( 4, "v111", "V 1.11" )
  ROMX_LOAD( "ti92v111e.rom", 0x000000, 0x100000, CRC(4a343833) SHA1(ab4eaacc8c83a861c8d37df5c10e532d0d580460), ROM_BIOS(5))
  ROM_SYSTEM_BIOS( 5, "v112", "V 1.12" )
  ROMX_LOAD( "ti92v112e.rom", 0x000000, 0x100000, CRC(9a6947a0) SHA1(8bb0538ca98711e9ad46c56e4dfd609d4699be30), ROM_BIOS(6))
  ROM_SYSTEM_BIOS( 6, "v21", "V 2.1" )
  ROMX_LOAD( "ti92v21e.rom",  0x000000, 0x200000, CRC(5afb5863) SHA1(bf7b260d37d1502cc4b08dea5e1d55b523f27925), ROM_BIOS(7))

ROM_END

ROM_START( ti92p )
  ROM_REGION( 0x200000, "maincpu", ROMREGION_ERASEFF )
  ROM_SYSTEM_BIOS( 0, "v100", "V 1.00 - HW1" )
  ROMX_LOAD( "ti92pv100.rom", 0x0000, 0x200000, CRC(c651a586) SHA1(fbbf7e053e70eefe517f9aae40c072036bc614ea), ROM_BIOS(1))
  ROM_SYSTEM_BIOS( 1, "v101", "V 1.01 - HW1" )
  ROMX_LOAD( "ti92pv101.rom", 0x0000, 0x200000, CRC(826b1539) SHA1(dd8969511fc6233bf2047f83c3306ac8d2be5644), ROM_BIOS(2))
  ROM_SYSTEM_BIOS( 2, "v101a","V 1.01 [a] - HW1" )
  ROMX_LOAD( "ti92pv101a.rom", 0x0000, 0x200000, CRC(18f9002f) SHA1(2bf13ba7da0212a8706c5a43853dc2ccb8c2257d), ROM_BIOS(3))
  ROM_SYSTEM_BIOS( 3, "v101m", "V 1.01 [m] - HW1" )
  ROMX_LOAD( "ti92pv101m.rom", 0x0000, 0x200000, CRC(fe2b6e77) SHA1(0e1bb8c677a726ee086c1a4280ab59de95b4abe2), ROM_BIOS(4))
  ROM_SYSTEM_BIOS( 4, "v105", "V 1.05 - HW1" )
  ROMX_LOAD( "ti92pv105.rom", 0x0000, 0x200000, CRC(cd945824) SHA1(6941aca243c6fd5c8a377253bffc2ffb5a84c41b), ROM_BIOS(5))
  ROM_SYSTEM_BIOS( 5, "v105-2", "V 1.05 - HW2" )
  ROMX_LOAD( "ti92pv105-2.rom", 0x0000, 0x200000, CRC(289aa84f) SHA1(c9395750e20d5a201401699d156b62f00530fcdd), ROM_BIOS(6))
  ROM_SYSTEM_BIOS( 6, "v203", "V 2.03 - HW2" )
  ROMX_LOAD( "ti92pv203.rom", 0x0000, 0x200000, CRC(1612213e) SHA1(1715dd5913bed12baedc4912e9abe0cb4e48cd45), ROM_BIOS(7))
  ROM_SYSTEM_BIOS( 7, "v204", "V 2.04 - HW2" )
  ROMX_LOAD( "ti92pv204.rom", 0x0000, 0x200000, CRC(86819be3) SHA1(78032a0f5f11d1e9a45ffbea91e7f9657fd1a8ae), ROM_BIOS(8))
  ROM_SYSTEM_BIOS( 8, "v205", "V 2.05 - HW2" )
  ROMX_LOAD( "ti92pv205.rom",  0x0000, 0x200000, CRC(9509c575) SHA1(703410d8bb98b8ec14277efcd8b7dda45a7cf358), ROM_BIOS(9))
  ROM_SYSTEM_BIOS( 9, "v205m", "V 2.05 [m] - HW2" )
  ROMX_LOAD( "ti92pv205m.rom",  0x000000, 0x200000, CRC(13ef4d57) SHA1(6ef290bb0dda72f645cd3eca9cc1185f6a2d32dc), ROM_BIOS(10))
  ROM_SYSTEM_BIOS( 10, "v209", "V 2.09 - HW2" )
  ROMX_LOAD( "ti92pv209.rom",  0x000000, 0x200000, CRC(4851ad52) SHA1(10e6c2cdc60623bf0be7ea72a9ec840259fb37c3), ROM_BIOS(11))
ROM_END

ROM_START( v200 )
  ROM_REGION( 0x400000, "maincpu", ROMREGION_ERASEFF )
  ROM_SYSTEM_BIOS( 0, "v209", "V 2.09" )
  ROMX_LOAD( "voyage200v209.rom", 0x0000, 0x400000, CRC(f805c7a6) SHA1(818b919058ba3bd7d15604f11fff6740010d07fc), ROM_BIOS(1))
  ROM_SYSTEM_BIOS( 1, "v310", "V 3.10" )
  ROMX_LOAD( "voyage200v310.rom", 0x0000, 0x400000, CRC(ed4cbfd2) SHA1(39cdb9932f314ff792b1cc5e3fe041d98b9fd101), ROM_BIOS(2))
ROM_END

ROM_START( ti89t )
  ROM_REGION( 0x400000, "maincpu", ROMREGION_ERASEFF )
  ROM_SYSTEM_BIOS( 0, "v300", "V 3.00" )
  ROMX_LOAD( "ti89tv300.rom", 0x0000, 0x400000, CRC(55eb4f5a) SHA1(4f919d7752caf2559a79883ec8711a9701d19513), ROM_BIOS(1))
  ROM_SYSTEM_BIOS( 1, "v310", "V 3.10" )
  ROMX_LOAD( "ti89tv310.rom", 0x0000, 0x400000, CRC(b6967cca) SHA1(fb4f09e5c4500dee651b8de537e502ab97cb8328), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1998, ti89,  0,       0,		ti89,	ti8x,	 0, 		"Texas Instruments",	"TI-89",		 GAME_NO_SOUND)
COMP( 1995, ti92,  0,       0,		ti92,	ti9x,	 0, 		"Texas Instruments",	"TI-92",		 GAME_NO_SOUND)
COMP( 1999, ti92p, 0,       0,		ti92p,	ti9x,	 0, 		"Texas Instruments",	"TI-92 Plus",    GAME_NO_SOUND)
COMP( 2002, v200,  0,       0,		v200,	ti9x,	 0, 		"Texas Instruments",	"Voyage 200 PLT",GAME_NO_SOUND)
COMP( 2004, ti89t, 0,       0,		ti89t,	ti8x,	 0, 		"Texas Instruments",	"TI-89 Titanium",GAME_NO_SOUND)
