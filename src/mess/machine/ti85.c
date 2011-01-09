/***************************************************************************
  TI-85 driver by Krzysztof Strzecha

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/ti85.h"
#include "formats/ti85_ser.h"
#include "sound/speaker.h"

#define TI85_SNAPSHOT_SIZE	 32976
#define TI86_SNAPSHOT_SIZE	131284

enum
{
	TI_NOT_INITIALIZED,
	TI_81,
	TI_82,
	TI_83,
	TI_83p,
	TI_85,
	TI_86
};














static TIMER_CALLBACK(ti85_timer_callback)
{
	ti85_state *state = machine->driver_data<ti85_state>();
	if (input_port_read(machine, "ON") & 0x01)
	{
		if (state->ON_interrupt_mask && !state->ON_pressed)
		{
			cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
			state->ON_interrupt_status = 1;
			if (!state->timer_interrupt_mask) state->timer_interrupt_mask = 1;
		}
		state->ON_pressed = 1;
		return;
	}
	else
		state->ON_pressed = 0;
	if (state->timer_interrupt_mask)
	{
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
		state->timer_interrupt_status = 1;
	}
}

static void update_ti85_memory (running_machine *machine)
{
	ti85_state *state = machine->driver_data<ti85_state>();
	memory_set_bankptr(machine, "bank2",machine->region("maincpu")->base() + 0x010000 + 0x004000*state->ti8x_memory_page_1);
}

static void update_ti83p_memory (running_machine *machine)
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (state->ti8x_memory_page_1 & 0x40)
	{
		if (state->ti83p_port4 & 1)
		{

			memory_set_bankptr(machine, "bank3", state->ti8x_ram + 0x004000*(state->ti8x_memory_page_1&0x01));
			memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank3");
		}
		else
		{
			memory_set_bankptr(machine, "bank2", state->ti8x_ram + 0x004000*(state->ti8x_memory_page_1&0x01));
			memory_install_write_bank(space, 0x4000, 0x7fff, 0, 0, "bank2");
		}
	}
	else
	{
		if (state->ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank3", machine->region("maincpu")->base() + 0x010000 + 0x004000*(state->ti8x_memory_page_1&0x1f));
			memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);
		}
		else
		{
			memory_set_bankptr(machine, "bank2", machine->region("maincpu")->base() + 0x010000 + 0x004000*(state->ti8x_memory_page_1&0x1f));
			memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
		}
	}

	if (state->ti8x_memory_page_2 & 0x40)
	{
		if (state->ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank4", state->ti8x_ram + 0x004000*(state->ti8x_memory_page_2&0x01));
			memory_install_write_bank(space, 0xc000, 0xffff, 0, 0, "bank4");
		}
		else
		{
			memory_set_bankptr(machine, "bank3", state->ti8x_ram + 0x004000*(state->ti8x_memory_page_2&0x01));
			memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank3");
		}

	}
	else
	{
		if (state->ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank4", machine->region("maincpu")->base() + 0x010000 + 0x004000*(state->ti8x_memory_page_2&0x1f));
			memory_unmap_write(space, 0xc000, 0xffff, 0, 0);
		}
		else
		{
			memory_set_bankptr(machine, "bank3", machine->region("maincpu")->base() + 0x010000 + 0x004000*(state->ti8x_memory_page_2&0x1f));
			memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);
		}
	}
}

static void update_ti86_memory (running_machine *machine)
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (state->ti8x_memory_page_1 & 0x40)
	{
		memory_set_bankptr(machine, "bank2", state->ti8x_ram + 0x004000*(state->ti8x_memory_page_1&0x07));
		memory_install_write_bank(space, 0x4000, 0x7fff, 0, 0, "bank2");
	}
	else
	{
		memory_set_bankptr(machine, "bank2", machine->region("maincpu")->base() + 0x010000 + 0x004000*(state->ti8x_memory_page_1&0x0f));
		memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	}

	if (state->ti8x_memory_page_2 & 0x40)
	{
		memory_set_bankptr(machine, "bank3", state->ti8x_ram + 0x004000*(state->ti8x_memory_page_2&0x07));
		memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank3");
	}
	else
	{
		memory_set_bankptr(machine, "bank3", machine->region("maincpu")->base() + 0x010000 + 0x004000*(state->ti8x_memory_page_2&0x0f));
		memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);
	}

}

/***************************************************************************
  Machine Initialization
***************************************************************************/

MACHINE_START( ti81 )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = machine->region("maincpu")->base();

	state->timer_interrupt_mask = 0;
	state->timer_interrupt_status = 0;
	state->ON_interrupt_mask = 0;
	state->ON_interrupt_status = 0;
	state->ON_pressed = 0;
	state->power_mode = 0;
	state->keypad_mask = 0;
	state->ti8x_memory_page_1 = 0;
	state->LCD_memory_base = 0;
	state->LCD_status = 0;
	state->LCD_mask = 0;
	state->video_buffer_width = 0;
	state->interrupt_speed = 0;
	state->port4_bit0 = 0;
	state->ti81_port_7_data = 0;

	if (state->ti_calculator_model == TI_81)
		memset(mem+0x8000, 0, sizeof(unsigned char)*0x8000);

	state->ti_calculator_model = TI_81;

	timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);

	memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
	memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);
}

MACHINE_START( ti82 )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = machine->region("maincpu")->base();

	state->timer_interrupt_mask = 0;
	state->timer_interrupt_status = 0;
	state->ON_interrupt_mask = 0;
	state->ON_interrupt_status = 0;
	state->ON_pressed = 0;
	state->ti8x_memory_page_1 = 0;
	state->LCD_status = 0;
	state->LCD_mask = 0;
	state->power_mode = 0;
	state->keypad_mask = 0;
	state->video_buffer_width = 0;
	state->interrupt_speed = 0;
	state->port4_bit0 = 0;
	state->ti82_video_mode = 0;
	state->ti82_video_x = 0;
	state->ti82_video_y = 0;
	state->ti82_video_dir = 0;
	state->ti82_video_scroll = 0;
	state->ti82_video_bit = 0;
	state->ti82_video_col = 0;
	memset(state->ti82_video_buffer, 0x00, 0x300);

	if (state->ti_calculator_model == TI_82)
		memset(mem+0x8000, 0, sizeof(unsigned char)*0x8000);

	state->ti_calculator_model = TI_82;

	timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);

	memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
	memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);
}


MACHINE_START( ti85 )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = machine->region("maincpu")->base();

	state->timer_interrupt_mask = 0;
	state->timer_interrupt_status = 0;
	state->ON_interrupt_mask = 0;
	state->ON_interrupt_status = 0;
	state->ON_pressed = 0;
	state->ti8x_memory_page_1 = 0;
	state->LCD_memory_base = 0;
	state->LCD_status = 0;
	state->LCD_mask = 0;
	state->power_mode = 0;
	state->keypad_mask = 0;
	state->video_buffer_width = 0;
	state->interrupt_speed = 0;
	state->port4_bit0 = 0;

	if (state->ti_calculator_model == TI_85)
		memset(mem+0x8000, 0, sizeof(unsigned char)*0x8000);

	state->ti_calculator_model = TI_85;

	timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);

	memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
	memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);
}


MACHINE_RESET( ti85 )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	state->red_out = 0x00;
	state->white_out = 0x00;
	state->PCR = 0xc0;
}


MACHINE_START( ti83p )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = machine->region("maincpu")->base();

	state->timer_interrupt_mask = 0;
	state->timer_interrupt_status = 0;
	state->ON_interrupt_mask = 0;
	state->ON_interrupt_status = 0;
	state->ON_pressed = 0;
	state->ti8x_memory_page_1 = 0;
	state->ti8x_memory_page_2 = 0;
	state->LCD_memory_base = 0;
	state->LCD_status = 0;
	state->LCD_mask = 0;
	state->power_mode = 0;
	state->keypad_mask = 0;
	state->video_buffer_width = 0;
	state->interrupt_speed = 0;
	state->port4_bit0 = 0;
	state->ti82_video_mode = 0;
	state->ti82_video_x = 0;
	state->ti82_video_y = 0;
	state->ti82_video_dir = 0;
	state->ti82_video_scroll = 0;
	state->ti82_video_bit = 0;
	state->ti82_video_col = 0;
	memset(state->ti82_video_buffer, 0x00, 0x300);

	state->ti8x_ram = auto_alloc_array(machine, UINT8, 32*1024);
	{
		memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
		memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
		memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);

		memory_set_bankptr(machine, "bank1", mem + 0x010000);
		memory_set_bankptr(machine, "bank2", mem + 0x010000);
		memory_set_bankptr(machine, "bank3", mem + 0x010000);
		memory_set_bankptr(machine, "bank4", state->ti8x_ram);

		if (state->ti_calculator_model == TI_83p)
			memset(state->ti8x_ram, 0, sizeof(unsigned char)*32*1024);

//      memset(mem + 0x83000, 0xff, 0x1000);

		state->ti_calculator_model = TI_83p;

		timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);
	}
}


MACHINE_START( ti86 )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = machine->region("maincpu")->base();

	state->timer_interrupt_mask = 0;
	state->timer_interrupt_status = 0;
	state->ON_interrupt_mask = 0;
	state->ON_interrupt_status = 0;
	state->ON_pressed = 0;
	state->ti8x_memory_page_1 = 0;
	state->ti8x_memory_page_2 = 0;
	state->LCD_memory_base = 0;
	state->LCD_status = 0;
	state->LCD_mask = 0;
	state->power_mode = 0;
	state->keypad_mask = 0;
	state->video_buffer_width = 0;
	state->interrupt_speed = 0;
	state->port4_bit0 = 0;

	state->ti8x_ram = auto_alloc_array(machine, UINT8, 128*1024);
	{
		memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);

		memory_set_bankptr(machine, "bank1", mem + 0x010000);
		memory_set_bankptr(machine, "bank2", mem + 0x014000);

		memory_set_bankptr(machine, "bank4", state->ti8x_ram);

		if (state->ti_calculator_model == TI_86)
			memset(state->ti8x_ram, 0, sizeof(unsigned char)*128*1024);

		state->ti_calculator_model = TI_86;

		timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);
	}
}


/* I/O ports handlers */

READ8_HANDLER ( ti85_port_0000_r )
{
	return 0xff;
}

READ8_HANDLER ( ti8x_keypad_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	int data = 0xff;
	int port;
	int bit;
	static const char *const bitnames[] = { "BIT0", "BIT1", "BIT2", "BIT3", "BIT4", "BIT5", "BIT6", "BIT7" };

	if (state->keypad_mask == 0x7f) return data;

	for (bit = 0; bit < 7; bit++)
	{
		if (~state->keypad_mask&(0x01<<bit))
		{
			for (port = 0; port < 8; port++)
			{
				data ^= input_port_read(space->machine, bitnames[port]) & (0x01<<bit) ? 0x01<<port : 0x00;
			}
		}
	}
	return data;
}

 READ8_HANDLER ( ti85_port_0002_r )
{
	return 0xff;
}

 READ8_HANDLER ( ti85_port_0003_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	int data = 0;

	if (state->LCD_status)
		data |= state->LCD_mask;
	if (state->ON_interrupt_status)
		data |= 0x01;
	if (state->timer_interrupt_status)
		data |= 0x04;
	if (!state->ON_pressed)
		data |= 0x08;
	state->ON_interrupt_status = 0;
	state->timer_interrupt_status = 0;
	return data;
}

 READ8_HANDLER ( ti85_port_0004_r )
{
	return 0xff;
}

 READ8_HANDLER ( ti85_port_0005_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return state->ti8x_memory_page_1;
}

READ8_HANDLER ( ti85_port_0006_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return state->power_mode;
}

READ8_HANDLER ( ti85_port_0007_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	device_t *ti85serial = space->machine->device("ti85serial");

	ti85_update_serial(ti85serial);
	return (state->white_out<<3)
		| (state->red_out<<2)
		| ((ti85serial_white_in(ti85serial,0)&(1-state->white_out))<<1)
		| (ti85serial_red_in(ti85serial,0)&(1-state->red_out))
		| state->PCR;
}

READ8_HANDLER ( ti82_port_0000_r )
{
	return 0xff; // TODO
}

 READ8_HANDLER ( ti82_port_0002_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return state->ti8x_port2;
}

 READ8_HANDLER ( ti82_port_0010_r )
{
	return 0x00;
}

 READ8_HANDLER ( ti82_port_0011_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
    UINT8* ti82_video;
	UINT8 ti82_port11;

	if(state->ti82_video_dir == 4)
		++state->ti82_video_y;
	if(state->ti82_video_dir == 5)
		--state->ti82_video_y;
	if(state->ti82_video_dir == 6)
		++state->ti82_video_x;
	if(state->ti82_video_dir == 7)
		--state->ti82_video_x;

	if(state->ti82_video_mode)
		ti82_port11 = state->ti82_video_buffer[(12*state->ti82_video_y)+state->ti82_video_x];
	else
	{
		state->ti82_video_col = state->ti82_video_x * 6;
		state->ti82_video_bit = state->ti82_video_col & 7;
		ti82_video = &state->ti82_video_buffer[(state->ti82_video_y*12)+(state->ti82_video_col>>3)];
		ti82_port11 = ((((*ti82_video)<<8)+ti82_video[1])>>(10-state->ti82_video_bit));
	}

	if(state->ti82_video_dir == 4)
		state->ti82_video_y-=2;
	if(state->ti82_video_dir == 5)
		state->ti82_video_y+=2;
	if(state->ti82_video_dir == 6)
		state->ti82_video_x-=2;
	if(state->ti82_video_dir == 7)
		state->ti82_video_x+=2;

	state->ti82_video_x &= 15;
	state->ti82_video_y &= 63;

	return ti82_port11;
}

READ8_HANDLER ( ti86_port_0005_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return state->ti8x_memory_page_1;
}

 READ8_HANDLER ( ti86_port_0006_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return state->ti8x_memory_page_2;
}

READ8_HANDLER ( ti83_port_0000_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return ((state->ti8x_memory_page_1 & 0x08) << 1) | 0x0C;
}

 READ8_HANDLER ( ti83_port_0002_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return state->ti8x_port2;
}

 READ8_HANDLER ( ti83_port_0003_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	int data = 0;

	if (state->LCD_status)
		data |= state->LCD_mask;
	if (state->ON_interrupt_status)
		data |= 0x01;
	if (!state->ON_pressed)
		data |= 0x08;
	state->ON_interrupt_status = 0;
	state->timer_interrupt_status = 0;
	return data;
}

 READ8_HANDLER ( ti83p_port_0002_r )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	return state->ti8x_port2|3;
}

WRITE8_HANDLER ( ti81_port_0007_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti81_port_7_data = data;
}

WRITE8_HANDLER ( ti85_port_0000_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->LCD_memory_base = data;
}

WRITE8_HANDLER ( ti8x_keypad_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->keypad_mask = data&0x7f;
}

WRITE8_HANDLER ( ti85_port_0002_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->LCD_contrast = data&0x1f;
}

WRITE8_HANDLER ( ti85_port_0003_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	if (state->LCD_status && !(data&0x08))	state->timer_interrupt_mask = 0;
	state->ON_interrupt_mask = data&0x01;
//  state->timer_interrupt_mask = data&0x04;
	state->LCD_mask = data&0x02;
	state->LCD_status = data&0x08;
}

WRITE8_HANDLER ( ti85_port_0004_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->video_buffer_width = (data>>3)&0x03;
	state->interrupt_speed = (data>>1)&0x03;
	state->port4_bit0 = data&0x01;
}

WRITE8_HANDLER ( ti85_port_0005_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_1 = data;
	update_ti85_memory(space->machine);
}

WRITE8_HANDLER ( ti85_port_0006_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->power_mode = data;
}

WRITE8_HANDLER ( ti85_port_0007_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	device_t *speaker = space->machine->device("speaker");
	device_t *ti85serial = space->machine->device("ti85serial");

	speaker_level_w(speaker, ( (data>>2)|(data>>3) ) & 0x01);
	state->red_out=(data>>2)&0x01;
	state->white_out=(data>>3)&0x01;
	ti85serial_red_out( ti85serial, 0, state->red_out );
	ti85serial_white_out( ti85serial, 0, state->white_out );
	ti85_update_serial(ti85serial);
	state->PCR = data&0xf0;
}

WRITE8_HANDLER ( ti86_port_0005_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_1 = data&((data&0x40)?0x47:0x4f);
	update_ti86_memory(space->machine);
}

WRITE8_HANDLER ( ti86_port_0006_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_2 = data&((data&0x40)?0x47:0x4f);
	update_ti86_memory(space->machine);
}

WRITE8_HANDLER ( ti82_port_0000_w )
{
	// TODO
}

WRITE8_HANDLER ( ti82_port_0002_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_1 = (data & 0x07);
	update_ti85_memory(space->machine);
	state->ti8x_port2 = data;
}

WRITE8_HANDLER ( ti82_port_0010_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	if (data == 0x00 || data == 0x01)
		state->ti82_video_mode = data;
	if (data >= 0x04 && data <= 0x07)
		state->ti82_video_dir = data;
	if (data >= 0x20 && data <= 0x30)
		state->ti82_video_x = data - 0x20;
	if (data >= 0x40 && data <= 0x7F)
		state->ti82_video_scroll = data - 0x40;
	if (data >= 0x80 && data <= 0xBF)
		state->ti82_video_y = data - 0x80;
	if (data >= 0xD8)
		state->LCD_contrast = data - 0xD8;
}

WRITE8_HANDLER ( ti82_port_0011_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	UINT8* ti82_video;

	if(state->ti82_video_mode)
		state->ti82_video_buffer[(12*state->ti82_video_y)+state->ti82_video_x] = data;
	else
	{
		data = data<<0x02;
		state->ti82_video_col = state->ti82_video_x * 6;
		state->ti82_video_bit = state->ti82_video_col & 0x07;
		ti82_video = &state->ti82_video_buffer[(state->ti82_video_y*12)+(state->ti82_video_col>>3)];
		*ti82_video = (*ti82_video & ~(0xFC>>state->ti82_video_bit)) | (data>>state->ti82_video_bit);
		if(state->ti82_video_bit>0x02)
			ti82_video[1] = (ti82_video[1] & ~(0xFC<<(8-state->ti82_video_bit))) | (data<<(8-state->ti82_video_bit));
	}

	if(state->ti82_video_dir == 0x04)
		--state->ti82_video_y;
	if(state->ti82_video_dir == 0x05)
		++state->ti82_video_y;
	if(state->ti82_video_dir == 0x06)
		--state->ti82_video_x;
	if(state->ti82_video_dir == 0x07)
		++state->ti82_video_x;

	state->ti82_video_x &= 15;
	state->ti82_video_y &= 63;
}

WRITE8_HANDLER ( ti83_port_0000_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_1 = (state->ti8x_memory_page_1 & 7) | ((data & 16) >> 1);
	update_ti85_memory(space->machine);
}

WRITE8_HANDLER ( ti83_port_0002_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_1 = (state->ti8x_memory_page_1 & 8) | (data & 7);
	update_ti85_memory(space->machine);
	state->ti8x_port2 = data;
}

WRITE8_HANDLER ( ti83_port_0003_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
		if (state->LCD_status && !(data&0x08))	state->timer_interrupt_mask = 0;
		state->ON_interrupt_mask = data&0x01;
		//state->timer_interrupt_mask = data&0x04;
		state->LCD_mask = data&0x02;
		state->LCD_status = data&0x08;
}

WRITE8_HANDLER ( ti83p_port_0002_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_port2 = data;
}

WRITE8_HANDLER ( ti83p_port_0003_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->LCD_mask = (data&0x08) >> 2;
	state->ON_interrupt_mask = data & 0x01;
}

WRITE8_HANDLER ( ti83p_port_0004_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	if ((data & 1) && !(state->ti83p_port4 & 1))
	{
		state->ti8x_memory_page_1 = 0x1f;
		state->ti8x_memory_page_2 = 0x1f;
	}
	else if (!(data & 1) && (state->ti83p_port4 & 1))
	{
		state->ti8x_memory_page_1 = 0x1f;
		state->ti8x_memory_page_2 = 0x40;
	}
	update_ti83p_memory(space->machine);
	state->ti83p_port4 = data;
}

WRITE8_HANDLER ( ti83p_port_0006_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_1 = data & ((data&0x40) ? 0x41 : 0x5f);
	update_ti83p_memory(space->machine);
}

WRITE8_HANDLER ( ti83p_port_0007_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	state->ti8x_memory_page_2 = data & ((data&0x40) ? 0x41 : 0x5f);
	update_ti83p_memory(space->machine);
}

WRITE8_HANDLER ( ti83p_port_0010_w )
{
	ti85_state *state = space->machine->driver_data<ti85_state>();
	if (data == 0x00 || data == 0x01)
		state->ti82_video_mode = data;
	if (data == 0x02 || data == 0x03)
		state->LCD_status = data - 0x02;
	if (data >= 0x04 && data <= 0x07)
		state->ti82_video_dir = data;
	if (data >= 0x20 && data <= 0x30)
		state->ti82_video_x = data - 0x20;
	if (data >= 0x40 && data <= 0x7F)
		state->ti82_video_scroll = data - 0x40;
	if (data >= 0x80 && data <= 0xBF)
		state->ti82_video_y = data - 0x80;
	if (data >= 0xc8)
		state->LCD_contrast = data - 0xc8;

}

/* NVRAM functions */
NVRAM_HANDLER( ti83p )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	if (read_or_write)
	{
		mame_fwrite(file, state->ti8x_ram, sizeof(unsigned char)*32*1024);
	}
	else
	{
			if (file)
			{
				mame_fread(file, state->ti8x_ram, sizeof(unsigned char)*32*1024);
				cpu_set_reg(machine->device("maincpu"), Z80_PC,0x0c59);
			}
			else
				memset(state->ti8x_ram, 0, sizeof(unsigned char)*32*1024);
	}
}

NVRAM_HANDLER( ti86 )
{
	ti85_state *state = machine->driver_data<ti85_state>();
	if (read_or_write)
	{
		mame_fwrite(file, state->ti8x_ram, sizeof(unsigned char)*128*1024);
	}
	else
	{
			if (file)
			{
				mame_fread(file, state->ti8x_ram, sizeof(unsigned char)*128*1024);
				cpu_set_reg(machine->device("maincpu"), Z80_PC,0x0c59);
			}
			else
				memset(state->ti8x_ram, 0, sizeof(unsigned char)*128*1024);
	}
}

/***************************************************************************
  TI calculators snapshot files (SAV)
***************************************************************************/

static void ti8x_snapshot_setup_registers (running_machine *machine, UINT8 * data)
{
	unsigned char lo,hi;
	unsigned char * reg = data + 0x40;

	/* Set registers */
	lo = reg[0x00] & 0x0ff;
	hi = reg[0x01] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_AF, (hi << 8) | lo);
	lo = reg[0x04] & 0x0ff;
	hi = reg[0x05] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_BC, (hi << 8) | lo);
	lo = reg[0x08] & 0x0ff;
	hi = reg[0x09] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_DE, (hi << 8) | lo);
	lo = reg[0x0c] & 0x0ff;
	hi = reg[0x0d] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_HL, (hi << 8) | lo);
	lo = reg[0x10] & 0x0ff;
	hi = reg[0x11] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_IX, (hi << 8) | lo);
	lo = reg[0x14] & 0x0ff;
	hi = reg[0x15] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_IY, (hi << 8) | lo);
	lo = reg[0x18] & 0x0ff;
	hi = reg[0x19] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_PC, (hi << 8) | lo);
	lo = reg[0x1c] & 0x0ff;
	hi = reg[0x1d] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_SP, (hi << 8) | lo);
	lo = reg[0x20] & 0x0ff;
	hi = reg[0x21] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_AF2, (hi << 8) | lo);
	lo = reg[0x24] & 0x0ff;
	hi = reg[0x25] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_BC2, (hi << 8) | lo);
	lo = reg[0x28] & 0x0ff;
	hi = reg[0x29] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_DE2, (hi << 8) | lo);
	lo = reg[0x2c] & 0x0ff;
	hi = reg[0x2d] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_HL2, (hi << 8) | lo);
	cpu_set_reg(machine->device("maincpu"), Z80_IFF1, reg[0x30]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_IFF2, reg[0x34]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_HALT, reg[0x38]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_IM, reg[0x3c]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_I, reg[0x40]&0x0ff);

	cpu_set_reg(machine->device("maincpu"), Z80_R, (reg[0x44]&0x7f) | (reg[0x48]&0x80));

	cputag_set_input_line(machine, "maincpu", 0, 0);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, 0);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);
}

static void ti85_setup_snapshot (running_machine *machine, UINT8 * data)
{
	ti85_state *state = machine->driver_data<ti85_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int i;
	unsigned char lo,hi;
	unsigned char * hdw = data + 0x8000 + 0x94;

	ti8x_snapshot_setup_registers (machine, data);

	/* Memory dump */
	for (i = 0; i < 0x8000; i++)
	   space->write_byte(i + 0x8000, data[i+0x94]);

	state->keypad_mask = hdw[0x00]&0x7f;

	state->ti8x_memory_page_1 = hdw[0x08]&0xff;
	update_ti85_memory(machine);

	state->power_mode = hdw[0x14]&0xff;

	lo = hdw[0x28] & 0x0ff;
	hi = hdw[0x29] & 0x0ff;
	state->LCD_memory_base = (((hi << 8) | lo)-0xc000)>>8;

	state->LCD_status = hdw[0x2c]&0xff ? 0 : 0x08;
	if (state->LCD_status) state->LCD_mask = 0x02;

	state->LCD_contrast = hdw[0x30]&0xff;

	state->timer_interrupt_mask = !hdw[0x38];
	state->timer_interrupt_status = 0;

	state->ON_interrupt_mask = !state->LCD_status && state->timer_interrupt_status;
	state->ON_interrupt_status = 0;
	state->ON_pressed = 0;

	state->video_buffer_width = 0x02;
	state->interrupt_speed = 0x03;
}

static void ti86_setup_snapshot (running_machine *machine, UINT8 * data)
{
	ti85_state *state = machine->driver_data<ti85_state>();
	unsigned char lo,hi;
	unsigned char * hdw = data + 0x20000 + 0x94;

	ti8x_snapshot_setup_registers (machine, data);

	/* Memory dump */
	memcpy(state->ti8x_ram, data+0x94, 0x20000);

	state->keypad_mask = hdw[0x00]&0x7f;

	state->ti8x_memory_page_1 = hdw[0x04]&0xff ? 0x40 : 0x00;
	state->ti8x_memory_page_1 |= hdw[0x08]&0x0f;

	state->ti8x_memory_page_2 = hdw[0x0c]&0xff ? 0x40 : 0x00;
	state->ti8x_memory_page_2 |= hdw[0x10]&0x0f;

	update_ti86_memory(machine);

	lo = hdw[0x2c] & 0x0ff;
	hi = hdw[0x2d] & 0x0ff;
	state->LCD_memory_base = (((hi << 8) | lo)-0xc000)>>8;

	state->LCD_status = hdw[0x30]&0xff ? 0 : 0x08;
	if (state->LCD_status) state->LCD_mask = 0x02;

	state->LCD_contrast = hdw[0x34]&0xff;

	state->timer_interrupt_mask = !hdw[0x3c];
	state->timer_interrupt_status = 0;

	state->ON_interrupt_mask = !state->LCD_status && state->timer_interrupt_status;
	state->ON_interrupt_status = 0;
	state->ON_pressed = 0;

	state->video_buffer_width = 0x02;
	state->interrupt_speed = 0x03;
}

SNAPSHOT_LOAD( ti8x )
{
	ti85_state *state = image.device().machine->driver_data<ti85_state>();
	int expected_snapshot_size = 0;
	UINT8 *ti8x_snapshot_data;

	switch (state->ti_calculator_model)
	{
		case TI_85: expected_snapshot_size = TI85_SNAPSHOT_SIZE; break;
		case TI_86: expected_snapshot_size = TI86_SNAPSHOT_SIZE; break;
	}

	logerror("Snapshot loading\n");

	if (snapshot_size != expected_snapshot_size)
	{
		logerror ("Incomplete snapshot file\n");
		return IMAGE_INIT_FAIL;
	}

	if (!(ti8x_snapshot_data = (UINT8*)malloc(snapshot_size)))
		return IMAGE_INIT_FAIL;

	image.fread( ti8x_snapshot_data, snapshot_size);

	switch (state->ti_calculator_model)
	{
		case TI_85: ti85_setup_snapshot(image.device().machine, ti8x_snapshot_data); break;
		case TI_86: ti86_setup_snapshot(image.device().machine, ti8x_snapshot_data); break;
	}
	free(ti8x_snapshot_data);
	return IMAGE_INIT_PASS;
}


