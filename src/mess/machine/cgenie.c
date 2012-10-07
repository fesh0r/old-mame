/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine
  (RAM, ROM, interrupts, I/O ports)

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/cgenie.h"
#include "machine/wd17xx.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"

#define AYWriteReg(chip,port,value) \
	ay8910_address_w(ay8910, space, 0,port);  \
	ay8910_data_w(ay8910, space, 0,value)

#define TAPE_HEADER "Colour Genie - Virtual Tape File"




#define IRQ_TIMER		0x80
#define IRQ_FDC 		0x40



TIMER_CALLBACK_MEMBER(cgenie_state::handle_cassette_input)
{
	UINT8 new_level = ( (machine().device<cassette_image_device>(CASSETTE_TAG)->input()) > 0.0 ) ? 1 : 0;

	if ( new_level != m_cass_level )
	{
		m_cass_level = new_level;
		m_cass_bit ^= 1;
	}
}


void cgenie_state::machine_reset()
{
	address_space &space = machine().device("maincpu")->memory().space(AS_PROGRAM);
	device_t *ay8910 = machine().device("ay8910");
	UINT8 *ROM = memregion("maincpu")->base();

	/* reset the AY8910 to be quiet, since the cgenie BIOS doesn't */
	AYWriteReg(0, 0, 0);
	AYWriteReg(0, 1, 0);
	AYWriteReg(0, 2, 0);
	AYWriteReg(0, 3, 0);
	AYWriteReg(0, 4, 0);
	AYWriteReg(0, 5, 0);
	AYWriteReg(0, 6, 0);
	AYWriteReg(0, 7, 0x3f);
	AYWriteReg(0, 8, 0);
	AYWriteReg(0, 9, 0);
	AYWriteReg(0, 10, 0);

	/* wipe out color RAM */
	memset(&ROM[0x0f000], 0x00, 0x0400);

	/* wipe out font RAM */
	memset(&ROM[0x0f400], 0xff, 0x0400);

	if( machine().root_device().ioport("DSW0")->read() & 0x80 )
	{
		logerror("cgenie floppy discs enabled\n");
	}
	else
	{
		logerror("cgenie floppy discs disabled\n");
	}

	/* copy DOS ROM, if enabled or wipe out that memory area */
	if( machine().root_device().ioport("DSW0")->read() & 0x40 )
	{
		if ( machine().root_device().ioport("DSW0")->read() & 0x80 )
		{
			space.install_read_bank(0xc000, 0xdfff, "bank10");
			space.nop_write(0xc000, 0xdfff);
			membank("bank10")->set_base(&ROM[0x0c000]);
			logerror("cgenie DOS enabled\n");
			memcpy(&ROM[0x0c000],&ROM[0x10000], 0x2000);
		}
		else
		{
			space.nop_readwrite(0xc000, 0xdfff);
			logerror("cgenie DOS disabled (no floppy image given)\n");
		}
	}
	else
	{
		space.nop_readwrite(0xc000, 0xdfff);
		logerror("cgenie DOS disabled\n");
		memset(&machine().root_device().memregion("maincpu")->base()[0x0c000], 0x00, 0x2000);
	}

	/* copy EXT ROM, if enabled or wipe out that memory area */
	if( machine().root_device().ioport("DSW0")->read() & 0x20 )
	{
		space.install_rom(0xe000, 0xefff, 0); // mess 0135u3 need to check
		logerror("cgenie EXT enabled\n");
		memcpy(&machine().root_device().memregion("maincpu")->base()[0x0e000],
			   &machine().root_device().memregion("maincpu")->base()[0x12000], 0x1000);
	}
	else
	{
		space.nop_readwrite(0xe000, 0xefff);
		logerror("cgenie EXT disabled\n");
		memset(&machine().root_device().memregion("maincpu")->base()[0x0e000], 0x00, 0x1000);
	}

	m_cass_level = 0;
	m_cass_bit = 1;
}

void cgenie_state::machine_start()
{
	address_space &space = machine().device("maincpu")->memory().space(AS_PROGRAM);
	UINT8 *gfx = memregion("gfx2")->base();
	int i;

	/* initialize static variables */
	m_irq_status = 0;
	m_motor_drive = 0;
	m_head = 0;
	m_tv_mode = -1;
	m_port_ff = 0xff;

	/*
     * Every fifth cycle is a wait cycle, so I reduced
     * the overlocking by one fitfth
     * Underclocking causes the tape loading to not work.
     */
//  cpunum_set_clockscale(machine(), 0, 0.80);

	/* Initialize some patterns to be displayed in graphics mode */
	for( i = 0; i < 256; i++ )
		memset(gfx + i * 8, i, 8);

	/* set up RAM */
	space.install_read_bank(0x4000, 0x4000 + machine().device<ram_device>(RAM_TAG)->size() - 1, "bank1");
	space.install_legacy_write_handler(0x4000, 0x4000 + machine().device<ram_device>(RAM_TAG)->size() - 1, FUNC(cgenie_videoram_w));
	m_videoram = machine().device<ram_device>(RAM_TAG)->pointer();
	membank("bank1")->set_base(machine().device<ram_device>(RAM_TAG)->pointer());
	machine().scheduler().timer_pulse(attotime::from_hz(11025), timer_expired_delegate(FUNC(cgenie_state::handle_cassette_input),this));
}

/*************************************
 *
 *              Port handlers.
 *
 *************************************/

/* used bits on port FF */
#define FF_CAS	0x01		   /* tape output signal */
#define FF_BGD0 0x04		   /* background color enable */
#define FF_CHR1 0x08		   /* charset 0xc0 - 0xff 1:fixed 0:defined */
#define FF_CHR0 0x10		   /* charset 0x80 - 0xbf 1:fixed 0:defined */
#define FF_CHR	(FF_CHR0 | FF_CHR1)
#define FF_FGR	0x20		   /* 1: "hi" resolution graphics, 0: text mode */
#define FF_BGD1 0x40		   /* background color select 1 */
#define FF_BGD2 0x80		   /* background color select 2 */
#define FF_BGD	(FF_BGD0 | FF_BGD1 | FF_BGD2)

WRITE8_HANDLER( cgenie_port_ff_w )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	int port_ff_changed = state->m_port_ff ^ data;

	space.machine().device<cassette_image_device>(CASSETTE_TAG)->output(data & 0x01 ? -1.0 : 1.0 );

	/* background bits changed ? */
	if( port_ff_changed & FF_BGD )
	{
		unsigned char r, g, b;

		if( data & FF_BGD0 )
		{
			r = 112;
			g = 0;
			b = 112;
		}
		else
		{
			if( state->m_tv_mode == 0 )
			{
				switch( data & (FF_BGD1 + FF_BGD2) )
				{
				case FF_BGD1:
					r = 112;
					g = 40;
					b = 32;
					break;
				case FF_BGD2:
					r = 40;
					g = 112;
					b = 32;
					break;
				case FF_BGD1 + FF_BGD2:
					r = 72;
					g = 72;
					b = 72;
					break;
				default:
					r = 0;
					g = 0;
					b = 0;
					break;
				}
			}
			else
			{
				r = 15;
				g = 15;
				b = 15;
			}
		}
		palette_set_color_rgb(space.machine(), 0, r, g, b);
	}

	/* character mode changed ? */
	if( port_ff_changed & FF_CHR )
	{
		state->m_font_offset[2] = (data & FF_CHR0) ? 0x00 : 0x80;
		state->m_font_offset[3] = (data & FF_CHR1) ? 0x00 : 0x80;
	}

	/* graphics mode changed ? */
	if( port_ff_changed & FF_FGR )
	{
		cgenie_mode_select(space.machine(), data & FF_FGR);
	}

	state->m_port_ff = data;
}


READ8_HANDLER( cgenie_port_ff_r )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	UINT8	data = state->m_port_ff & ~0x01;

	data |= state->m_cass_bit;

	return data;
}

int cgenie_port_xx_r( int offset )
{
	return 0xff;
}

/*************************************
 *                                   *
 *      Memory handlers              *
 *                                   *
 *************************************/


READ8_HANDLER( cgenie_psg_port_a_r )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	return state->m_psg_a_inp;
}

READ8_HANDLER( cgenie_psg_port_b_r )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	if( state->m_psg_a_out < 0xd0 )
	{
		/* comparator value */
		state->m_psg_b_inp = 0x00;

		if( space.machine().root_device().ioport("JOY0")->read() > state->m_psg_a_out )
			state->m_psg_b_inp |= 0x80;

		if( space.machine().root_device().ioport("JOY1")->read() > state->m_psg_a_out )
			state->m_psg_b_inp |= 0x40;

		if( space.machine().root_device().ioport("JOY2")->read() > state->m_psg_a_out )
			state->m_psg_b_inp |= 0x20;

		if( state->ioport("JOY3")->read() > state->m_psg_a_out )
			state->m_psg_b_inp |= 0x10;
	}
	else
	{
		/* read keypad matrix */
		state->m_psg_b_inp = 0xFF;

		if( !(state->m_psg_a_out & 0x01) )
			state->m_psg_b_inp &= ~space.machine().root_device().ioport("KP0")->read();

		if( !(state->m_psg_a_out & 0x02) )
			state->m_psg_b_inp &= ~space.machine().root_device().ioport("KP1")->read();

		if( !(state->m_psg_a_out & 0x04) )
			state->m_psg_b_inp &= ~space.machine().root_device().ioport("KP2")->read();

		if( !(state->m_psg_a_out & 0x08) )
			state->m_psg_b_inp &= ~space.machine().root_device().ioport("KP3")->read();

		if( !(state->m_psg_a_out & 0x10) )
			state->m_psg_b_inp &= ~space.machine().root_device().ioport("KP4")->read();

		if( !(state->m_psg_a_out & 0x20) )
			state->m_psg_b_inp &= ~space.machine().root_device().ioport("KP5")->read();
	}
	return state->m_psg_b_inp;
}

WRITE8_HANDLER( cgenie_psg_port_a_w )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	state->m_psg_a_out = data;
}

WRITE8_HANDLER( cgenie_psg_port_b_w )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	state->m_psg_b_out = data;
}

 READ8_HANDLER( cgenie_status_r )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, return 0 */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return 0;
	return wd17xx_status_r(fdc, space, offset);
}

 READ8_HANDLER( cgenie_track_r )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return 0xff;
	return wd17xx_track_r(fdc, space, offset);
}

 READ8_HANDLER( cgenie_sector_r )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return 0xff;
	return wd17xx_sector_r(fdc, space, offset);
}

 READ8_HANDLER(cgenie_data_r )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return 0xff;
	return wd17xx_data_r(fdc, space, offset);
}

WRITE8_HANDLER( cgenie_command_w )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, return immediately */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return;
	wd17xx_command_w(fdc, space, offset, data);
}

WRITE8_HANDLER( cgenie_track_w )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return;
	wd17xx_track_w(fdc, space, offset, data);
}

WRITE8_HANDLER( cgenie_sector_w )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return;
	wd17xx_sector_w(fdc, space, offset, data);
}

WRITE8_HANDLER( cgenie_data_w )
{
	device_t *fdc = space.machine().device("wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (space.machine().root_device().ioport("DSW0")->read() & 0x80) == 0 )
		return;
	wd17xx_data_w(fdc, space, offset, data);
}

 READ8_HANDLER( cgenie_irq_status_r )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
int result = state->m_irq_status;

	state->m_irq_status &= ~(IRQ_TIMER | IRQ_FDC);
	return result;
}

INTERRUPT_GEN_MEMBER(cgenie_state::cgenie_timer_interrupt)
{
	if( (m_irq_status & IRQ_TIMER) == 0 )
	{
		m_irq_status |= IRQ_TIMER;
		machine().device("maincpu")->execute().set_input_line(0, HOLD_LINE);
	}
}

WRITE_LINE_MEMBER(cgenie_state::cgenie_fdc_intrq_w)
{
	/* if disc hardware is not enabled, do not cause an int */
	if (!( ioport("DSW0")->read() & 0x80 ))
		return;

	if (state)
	{
		if( (m_irq_status & IRQ_FDC) == 0 )
		{
			m_irq_status |= IRQ_FDC;
			machine().device("maincpu")->execute().set_input_line(0, HOLD_LINE);
		}
	}
	else
	{
		m_irq_status &= ~IRQ_FDC;
	}
}

const wd17xx_interface cgenie_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(cgenie_state,cgenie_fdc_intrq_w),
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

WRITE8_HANDLER( cgenie_motor_w )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	device_t *fdc = space.machine().device("wd179x");
	UINT8 drive = 255;

	logerror("cgenie motor_w $%02X\n", data);

	if( data & 1 )
		drive = 0;
	if( data & 2 )
		drive = 1;
	if( data & 4 )
		drive = 2;
	if( data & 8 )
		drive = 3;

	if( drive > 3 )
		return;

	/* mask head select bit */
		state->m_head = (data >> 4) & 1;

	/* currently selected drive */
	state->m_motor_drive = drive;

	wd17xx_set_drive(fdc,drive);
	wd17xx_set_side(fdc,state->m_head);
}

/*************************************
 *      Keyboard                     *
 *************************************/
 READ8_HANDLER( cgenie_keyboard_r )
{
	int result = 0;

	if( offset & 0x01 )
		result |= space.machine().root_device().ioport("ROW0")->read();

	if( offset & 0x02 )
		result |= space.machine().root_device().ioport("ROW1")->read();

	if( offset & 0x04 )
		result |= space.machine().root_device().ioport("ROW2")->read();

	if( offset & 0x08 )
		result |= space.machine().root_device().ioport("ROW3")->read();

	if( offset & 0x10 )
		result |= space.machine().root_device().ioport("ROW4")->read();

	if( offset & 0x20 )
		result |= space.machine().root_device().ioport("ROW5")->read();

	if( offset & 0x40 )
		result |= space.machine().root_device().ioport("ROW6")->read();

	if( offset & 0x80 )
		result |= space.machine().root_device().ioport("ROW7")->read();

	return result;
}

/*************************************
 *      Video RAM                    *
 *************************************/

int cgenie_videoram_r( running_machine &machine, int offset )
{
	cgenie_state *state = machine.driver_data<cgenie_state>();
	UINT8 *videoram = state->m_videoram;
	return videoram[offset];
}

WRITE8_HANDLER( cgenie_videoram_w )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	UINT8 *videoram = state->m_videoram;
	/* write to video RAM */
	if( data == videoram[offset] )
		return; 			   /* no change */
	videoram[offset] = data;
}

 READ8_HANDLER( cgenie_colorram_r )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	return state->m_colorram[offset] | 0xf0;
}

WRITE8_HANDLER( cgenie_colorram_w )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	/* only bits 0 to 3 */
	data &= 15;
	/* nothing changed ? */
	if( data == state->m_colorram[offset] )
		return;

	/* set new value */
	state->m_colorram[offset] = data;
	/* make offset relative to video frame buffer offset */
	offset = (offset + (cgenie_get_register(space.machine(), 12) << 8) + cgenie_get_register(space.machine(), 13)) & 0x3ff;
}

 READ8_HANDLER( cgenie_fontram_r )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	return state->m_fontram[offset];
}

WRITE8_HANDLER( cgenie_fontram_w )
{
	cgenie_state *state = space.machine().driver_data<cgenie_state>();
	UINT8 *dp;

	if( data == state->m_fontram[offset] )
		return; 			   /* no change */

	/* store data */
	state->m_fontram[offset] = data;

	/* convert eight pixels */
	dp = const_cast<UINT8 *>(space.machine().gfx[0]->get_data(256 + offset/8) + (offset % 8) * space.machine().gfx[0]->width());
	dp[0] = (data & 0x80) ? 1 : 0;
	dp[1] = (data & 0x40) ? 1 : 0;
	dp[2] = (data & 0x20) ? 1 : 0;
	dp[3] = (data & 0x10) ? 1 : 0;
	dp[4] = (data & 0x08) ? 1 : 0;
	dp[5] = (data & 0x04) ? 1 : 0;
	dp[6] = (data & 0x02) ? 1 : 0;
	dp[7] = (data & 0x01) ? 1 : 0;
}

/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

INTERRUPT_GEN_MEMBER(cgenie_state::cgenie_frame_interrupt)
{
	if( m_tv_mode != (machine().root_device().ioport("DSW0")->read() & 0x10) )
	{
		m_tv_mode = ioport("DSW0")->read() & 0x10;
		/* force setting of background color */
		m_port_ff ^= FF_BGD0;
		cgenie_port_ff_w(machine().device("maincpu")->memory().space(AS_PROGRAM), 0, m_port_ff ^ FF_BGD0);
	}
}


READ8_MEMBER(cgenie_state::cgenie_sh_control_port_r)
{
	return m_control_port;
}

WRITE8_MEMBER(cgenie_state::cgenie_sh_control_port_w)
{
	m_control_port = data;
	ay8910_address_w(machine().device("ay8910"), space, offset, data);
}
