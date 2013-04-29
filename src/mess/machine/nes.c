/*****************************************************************************

    nes.c

    Nintendo Entertainment System (Famicom)

 ****************************************************************************/

#include "emu.h"
#include "crsshair.h"
#include "cpu/m6502/m6502.h"
#include "includes/nes.h"
#include "imagedev/cartslot.h"
#include "imagedev/flopdrv.h"
#include "hashfile.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

/* Set to dump info about the inputs to the errorlog */
#define LOG_JOY     0

/* Set to generate prg & chr files when the cart is loaded */
#define SPLIT_PRG   0
#define SPLIT_CHR   0

/***************************************************************************
    FUNCTIONS
***************************************************************************/

// to be probably removed (it does nothing since a long time)
int nes_state::nes_ppu_vidaccess( int address, int data )
{
	return data;
}

void nes_state::machine_reset()
{
	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	if (m_disk_expansion && m_cartslot && !m_cartslot->m_cart)
		m_ppu->set_hblank_callback(ppu2c0x_hblank_delegate(FUNC(nes_state::fds_irq),this));
	else if (m_cartslot)
		m_cartslot->pcb_reset();

	/* Reset the serial input ports */
	m_in_0.shift = 0;
	m_in_1.shift = 0;

	m_maincpu->reset();
}

static void nes_state_register( running_machine &machine )
{
	nes_state *state = machine.driver_data<nes_state>();

	state->save_item(NAME(state->m_last_frame_flip));

	state->save_item(NAME(state->m_fds_motor_on));
	state->save_item(NAME(state->m_fds_door_closed));
	state->save_item(NAME(state->m_fds_current_side));
	state->save_item(NAME(state->m_fds_head_position));
	state->save_item(NAME(state->m_fds_status0));
	state->save_item(NAME(state->m_fds_read_mode));
	state->save_item(NAME(state->m_fds_write_reg));
	state->save_item(NAME(state->m_fds_last_side));
	state->save_item(NAME(state->m_fds_count));
	state->save_item(NAME(state->m_fds_mirroring));

	state->save_pointer(NAME(state->m_ciram), 0x800);

	if (state->m_disk_expansion)
		state->save_pointer(NAME(state->m_vram), 0x800);
}


//-------------------------------------------------
//  machine_start
//-------------------------------------------------

void nes_state::machine_start()
{
	m_maincpu           = machine().device<cpu_device>("maincpu");
	m_ppu               = machine().device<ppu2c0x_device>("ppu");
	m_sound             = machine().device("nessound");
	m_io_ctrlsel        = ioport("CTRLSEL");
	m_io_fckey[0]       = ioport("FCKEY0");
	m_io_fckey[1]       = ioport("FCKEY1");
	m_io_fckey[2]       = ioport("FCKEY2");
	m_io_fckey[3]       = ioport("FCKEY3");
	m_io_fckey[4]       = ioport("FCKEY4");
	m_io_fckey[5]       = ioport("FCKEY5");
	m_io_fckey[6]       = ioport("FCKEY6");
	m_io_fckey[7]       = ioport("FCKEY7");
	m_io_fckey[8]       = ioport("FCKEY8");
	m_io_subkey[0 ]     = ioport("SUBKEY0");
	m_io_subkey[1 ]     = ioport("SUBKEY1");
	m_io_subkey[2 ]     = ioport("SUBKEY2");
	m_io_subkey[3 ]     = ioport("SUBKEY3");
	m_io_subkey[4 ]     = ioport("SUBKEY4");
	m_io_subkey[5 ]     = ioport("SUBKEY5");
	m_io_subkey[6 ]     = ioport("SUBKEY6");
	m_io_subkey[7 ]     = ioport("SUBKEY7");
	m_io_subkey[8 ]     = ioport("SUBKEY8");
	m_io_subkey[9 ]     = ioport("SUBKEY9");
	m_io_subkey[10]     = ioport("SUBKEY10");
	m_io_subkey[11]     = ioport("SUBKEY11");
	m_io_subkey[12]     = ioport("SUBKEY12");
	m_io_pad[0]         = ioport("PAD1");
	m_io_pad[1]         = ioport("PAD2");
	m_io_pad[2]         = ioport("PAD3");
	m_io_pad[3]         = ioport("PAD4");
	m_io_cc_left        = ioport("CC_LEFT");
	m_io_cc_right       = ioport("CC_RIGHT");
	m_io_zapper1_t      = ioport("ZAPPER1_T");
	m_io_zapper1_x      = ioport("ZAPPER1_X");
	m_io_zapper1_y      = ioport("ZAPPER1_Y");
	m_io_zapper2_t      = ioport("ZAPPER2_T");
	m_io_zapper2_x      = ioport("ZAPPER2_X");
	m_io_zapper2_y      = ioport("ZAPPER2_Y");
	m_io_paddle         = ioport("PADDLE");
	m_io_mahjong[0]     = ioport("MAH0");
	m_io_mahjong[1]     = ioport("MAH1");
	m_io_mahjong[2]     = ioport("MAH2");
	m_io_mahjong[3]     = ioport("MAH3");

	address_space &space = m_maincpu->space(AS_PROGRAM);

	// CIRAM (Character Internal RAM)
	// NES has 2KB of internal RAM which can be used to fill the 4x1KB banks of PPU RAM at $2000-$2fff
	// Line A10 is exposed to the carts, so that games can change CIRAM mapping in PPU (we emulate this with the set_nt_mirroring
	// function). CIRAM can also be disabled by the game (if e.g. VROM or cart RAM has to be used in PPU...
	m_ciram = auto_alloc_array(machine(), UINT8, 0x800);
	// other pointers got set in the loading routine, because they 'belong' to the cart itself

	if (m_cartslot && m_cartslot->m_cart)
	{
		// Set up memory handlers
		space.install_read_handler(0x4100, 0x5fff, read8_delegate(FUNC(nes_cart_slot_device::read_l), (nes_cart_slot_device *)m_cartslot));
		space.install_write_handler(0x4100, 0x5fff, write8_delegate(FUNC(nes_cart_slot_device::write_l), (nes_cart_slot_device *)m_cartslot));
		space.install_read_handler(0x6000, 0x7fff, read8_delegate(FUNC(nes_cart_slot_device::read_m), (nes_cart_slot_device *)m_cartslot));
		space.install_write_handler(0x6000, 0x7fff, write8_delegate(FUNC(nes_cart_slot_device::write_m), (nes_cart_slot_device *)m_cartslot));
		space.install_read_bank(0x8000, 0x9fff, "prg0");
		space.install_read_bank(0xa000, 0xbfff, "prg1");
		space.install_read_bank(0xc000, 0xdfff, "prg2");
		space.install_read_bank(0xe000, 0xffff, "prg3");
		space.install_write_handler(0x8000, 0xffff, write8_delegate(FUNC(nes_cart_slot_device::write_h), (nes_cart_slot_device *)m_cartslot));

		m_cartslot->pcb_start(m_ciram);
		m_cartslot->m_cart->pcb_reg_postload(machine());
		m_ppu->space(AS_PROGRAM).install_readwrite_handler(0, 0x1fff, read8_delegate(FUNC(device_nes_cart_interface::chr_r),m_cartslot->m_cart), write8_delegate(FUNC(device_nes_cart_interface::chr_w),m_cartslot->m_cart));
		m_ppu->space(AS_PROGRAM).install_readwrite_handler(0x2000, 0x3eff, read8_delegate(FUNC(device_nes_cart_interface::nt_r),m_cartslot->m_cart), write8_delegate(FUNC(device_nes_cart_interface::nt_w),m_cartslot->m_cart));
		m_ppu->set_scanline_callback(ppu2c0x_scanline_delegate(FUNC(device_nes_cart_interface::scanline_irq),m_cartslot->m_cart));
		m_ppu->set_hblank_callback(ppu2c0x_hblank_delegate(FUNC(device_nes_cart_interface::hblank_irq),m_cartslot->m_cart));
		m_ppu->set_latch(ppu2c0x_latch_delegate(FUNC(device_nes_cart_interface::ppu_latch),m_cartslot->m_cart));

		// install additional handlers (read_h, read_ex, write_ex)
		if (m_cartslot->get_pcb_id() == GG_NROM || m_cartslot->get_pcb_id() == SUNSOFT_DCS
			|| m_cartslot->get_pcb_id() == AVE_MAXI15 || m_cartslot->get_pcb_id() == KAISER_KS7022 || m_cartslot->get_pcb_id() == BMC_VT5201
			|| m_cartslot->get_pcb_id() == UNL_LH32 || m_cartslot->get_pcb_id() == UNL_LH10 || m_cartslot->get_pcb_id() == UNL_2708
			|| m_cartslot->get_pcb_id() == UNL_43272 || m_cartslot->get_pcb_id() == BMC_G63IN1 || m_cartslot->get_pcb_id() == BMC_8157
			|| m_cartslot->get_pcb_id() == BMC_GOLD150 || m_cartslot->get_pcb_id() == BMC_CH001
			|| m_cartslot->get_pcb_id() == BMC_70IN1 || m_cartslot->get_pcb_id() == BMC_800IN1)
		{
			logerror("read_h installed!\n");
			space.install_read_handler(0x8000, 0xffff, read8_delegate(FUNC(nes_cart_slot_device::read_h), (nes_cart_slot_device *)m_cartslot));
		}

		if (m_cartslot->get_pcb_id() == BTL_SMB2JB || m_cartslot->get_pcb_id() == UNL_AC08 || m_cartslot->get_pcb_id() == UNL_SMB2J || m_cartslot->get_pcb_id() == BTL_09034A)
		{
			logerror("write_ex installed!\n");
			space.install_write_handler(0x4020, 0x40ff, write8_delegate(FUNC(nes_cart_slot_device::write_ex), (nes_cart_slot_device *)m_cartslot));
		}

		if (m_cartslot->get_pcb_id() == KAISER_KS7017 || m_cartslot->get_pcb_id() == UNL_603_5052)
		{
			logerror("write_ex & read_ex installed!\n");
			space.install_read_handler(0x4020, 0x40ff, read8_delegate(FUNC(nes_cart_slot_device::read_ex), (nes_cart_slot_device *)m_cartslot));
			space.install_write_handler(0x4020, 0x40ff, write8_delegate(FUNC(nes_cart_slot_device::write_ex), (nes_cart_slot_device *)m_cartslot));
		}
	}
	else if (m_disk_expansion)  // if there is Disk Expansion and no cart has been loaded, setup memory accordingly
	{
		m_ppu->space(AS_PROGRAM).install_readwrite_handler(0, 0x1fff, read8_delegate(FUNC(nes_state::fds_chr_r),this), write8_delegate(FUNC(nes_state::fds_chr_w),this));
		m_ppu->space(AS_PROGRAM).install_readwrite_handler(0x2000, 0x3eff, read8_delegate(FUNC(nes_state::fds_nt_r),this), write8_delegate(FUNC(nes_state::fds_nt_w),this));

		if (!m_fds_ram)
			m_fds_ram = auto_alloc_array(machine(), UINT8, 0x8000);
		if (!m_vram)
			m_vram = auto_alloc_array(machine(), UINT8, 0x2000);

		space.install_read_handler(0x4030, 0x403f, read8_delegate(FUNC(nes_state::nes_fds_r),this));
		space.install_read_bank(0x6000, 0xdfff, "fdsram");
		space.install_read_bank(0xe000, 0xffff, "bank1");

		space.install_write_handler(0x4020, 0x402f, write8_delegate(FUNC(nes_state::nes_fds_w),this));
		space.install_write_bank(0x6000, 0xdfff, "fdsram");

		membank("bank1")->set_base(machine().root_device().memregion("maincpu")->base() + 0xe000);
		membank("fdsram")->set_base(m_fds_ram);
	}

	nes_state_register(machine());
}


//-------------------------------------------------
//  INPUTS
//-------------------------------------------------

READ8_MEMBER(nes_state::nes_IN0_r)
{
	int cfg = m_io_ctrlsel->read();
	int ret;

	if ((cfg & 0x000f) >= 0x08) // for now we treat the FC keyboard separately from other inputs!
	{
		// here we should have the tape input
		ret = 0;
	}
	else
	{
		/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
		/* in the unused upper 3 bits, so typically a read from $4016 leaves 0x40 there. */
		ret = 0x40;

		ret |= ((m_in_0.i0 >> m_in_0.shift) & 0x01);

		/* zapper */
		if ((cfg & 0x000f) == 0x0002)
		{
			int x = m_in_0.i1;  /* read Zapper x-position */
			int y = m_in_0.i2;  /* read Zapper y-position */
			UINT32 pix, color_base;

			/* get the pixel at the gun position */
			pix = m_ppu->get_pixel(x, y);

			/* get the color base from the ppu */
			color_base = m_ppu->get_colorbase();

			/* look at the screen and see if the cursor is over a bright pixel */
			if ((pix == color_base + 0x20) || (pix == color_base + 0x30) ||
				(pix == color_base + 0x33) || (pix == color_base + 0x34))
			{
				ret &= ~0x08; /* sprite hit */
			}
			else
				ret |= 0x08;  /* no sprite hit */

			/* If button 1 is pressed, indicate the light gun trigger is pressed */
			ret |= ((m_in_0.i0 & 0x01) << 4);
		}

		if (LOG_JOY)
			logerror("joy 0 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", ret, space.device().safe_pc(), m_in_0.shift, m_in_0.i0);

		m_in_0.shift++;
	}

	return ret;
}

READ8_MEMBER(nes_state::nes_IN1_r)
{
	int cfg = m_io_ctrlsel->read();
	int ret;

	// row of the keyboard matrix are read 4-bits at time, and gets returned as bit1->bit4
	if ((cfg & 0x000f) == 0x08) // for now we treat the FC keyboard separately from other inputs!
	{
		if (m_fck_scan < 9)
			ret = ~(((m_io_fckey[m_fck_scan]->read() >> (m_fck_mode * 4)) & 0x0f) << 1) & 0x1e;
		else
			ret = 0x1e;
	}
	else if ((cfg & 0x000f) == 0x09)    // for now we treat the Subor keyboard separately from other inputs!
	{
		if (m_fck_scan < 12)
			ret = ~(((m_io_subkey[m_fck_scan]->read() >> (m_fck_mode * 4)) & 0x0f) << 1) & 0x1e;
		else
			ret = 0x1e;
	}
	else
	{
		/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
		/* in the unused upper 3 bits, so typically a read from $4017 leaves 0x40 there. */
		ret = 0x40;

		/* Handle data line 0's serial output */
		if((cfg & 0x00f0) == 0x0070)
			ret |= (((m_in_1.i0 >> m_in_1.shift) & 0x01) << 1);
		else
			ret |= ((m_in_1.i0 >> m_in_1.shift) & 0x01);

		/* zapper */
		if ((cfg & 0x00f0) == 0x0030)
		{
			int x = m_in_1.i1;  /* read Zapper x-position */
			int y = m_in_1.i2;  /* read Zapper y-position */
			UINT32 pix, color_base;

			/* get the pixel at the gun position */
			pix = m_ppu->get_pixel(x, y);

			/* get the color base from the ppu */
			color_base = m_ppu->get_colorbase();

			/* look at the screen and see if the cursor is over a bright pixel */
			if ((pix == color_base + 0x20) || (pix == color_base + 0x30) ||
				(pix == color_base + 0x33) || (pix == color_base + 0x34))
			{
				ret &= ~0x08; /* sprite hit */
			}
			else
				ret |= 0x08;  /* no sprite hit */

			/* If button 1 is pressed, indicate the light gun trigger is pressed */
			ret |= ((m_in_1.i0 & 0x01) << 4);
		}

		/* arkanoid dial */
		else if ((cfg & 0x00f0) == 0x0040)
		{
			/* Handle data line 2's serial output */
			ret |= ((m_in_1.i2 >> m_in_1.shift) & 0x01) << 3;

			/* Handle data line 3's serial output - bits are reversed */
			/* NPW 27-Nov-2007 - there is no third subscript! commenting out */
			/* ret |= ((m_in_1[3] >> m_in_1.shift) & 0x01) << 4; */
			/* ret |= ((m_in_1[3] << m_in_1.shift) & 0x80) >> 3; */
		}

		if (LOG_JOY)
			logerror("joy 1 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", ret, space.device().safe_pc(), m_in_1.shift, m_in_1.i0);

		m_in_1.shift++;
	}

	return ret;
}


// FIXME: this is getting messier and messier (no pun intended). inputs reading should be simplified and port_categories cleaned up
// to also emulate the fact that nothing should be in Port 2 if there is a Crazy Climber pad, etc.
static void nes_read_input_device( running_machine &machine, int cfg, nes_input *vals, int pad_port, int supports_zapper, int mux_data )
{
	nes_state *state = machine.driver_data<nes_state>();

	vals->i0 = 0;
	vals->i1 = 0;
	vals->i2 = 0;

	switch (cfg & 0x0f)
	{
		case 0x01:  /* gamepad */
			if (pad_port >= 0)
				vals->i0 = state->m_io_pad[pad_port]->read();
			break;

		case 0x02:  /* zapper 1 */
			if (supports_zapper)
			{
				vals->i0 = state->m_io_zapper1_t->read();
				vals->i1 = state->m_io_zapper1_x->read();
				vals->i2 = state->m_io_zapper1_y->read();
			}
			break;

		case 0x03:  /* zapper 2 */
			if (supports_zapper)
			{
				vals->i0 = state->m_io_zapper2_t->read();
				vals->i1 = state->m_io_zapper2_x->read();
				vals->i2 = state->m_io_zapper2_y->read();
			}
			break;

		case 0x04:  /* arkanoid paddle */
			if (pad_port == 1)
				vals->i0 = (UINT8) ((UINT8) state->m_io_paddle->read() + (UINT8)0x52) ^ 0xff;
			break;

		case 0x06:  /* crazy climber controller */
			if (pad_port == 0)
			{
				state->m_in_0.i0 = state->m_io_cc_left->read();
				state->m_in_1.i0 = state->m_io_cc_right->read();
			}
			break;

		case 0x07: /* Mahjong Panel */
			if(mux_data & 0xf8)
				logerror("Error: Mahjong panel read with mux data %02x\n",mux_data);
			else
				vals->i0 = state->m_io_mahjong[mux_data >> 1]->read();
			break;
	}
}


TIMER_CALLBACK_MEMBER(nes_state::lightgun_tick)
{
	if ((m_io_ctrlsel->read() & 0x000f) == 0x0002)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine(), 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine(), 0, CROSSHAIR_SCREEN_NONE);
	}

	if ((m_io_ctrlsel->read() & 0x00f0) == 0x0030)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine(), 1, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine(), 1, CROSSHAIR_SCREEN_NONE);
	}
}

WRITE8_MEMBER(nes_state::nes_IN0_w)
{
	int cfg = m_io_ctrlsel->read();

	/* Check if lightgun has been chosen as input: if so, enable crosshair */
	machine().scheduler().timer_set(attotime::zero, timer_expired_delegate(FUNC(nes_state::lightgun_tick),this));

	if ((cfg & 0x000f) >= 0x08) // for now we treat the FC keyboard separately from other inputs!
	{
		// here we should also have the tape output

		if (BIT(data, 2))   // keyboard active
		{
			int lines = ((cfg & 0x000f) == 0x04) ? 9 : 12;
			UINT8 out = BIT(data, 1);   // scan

			if (m_fck_mode && !out && ++m_fck_scan > lines)
				m_fck_scan = 0;

			m_fck_mode = out;   // access lower or upper 4 bits

			if (BIT(data, 0))   // reset
				m_fck_scan = 0;
		}
	}
	else
	{
		if (data & 0x01)
			return;

		if (LOG_JOY)
			logerror("joy 0 bits read: %d\n", m_in_0.shift);

		/* Toggling bit 0 high then low resets both controllers */
		m_in_0.shift = 0;
		m_in_1.shift = 0;

		/* Read the input devices */
		if ((cfg & 0x000f) != 0x06)
		{
			nes_read_input_device(machine(), cfg >>  0, &m_in_0, 0,  TRUE,data & 0xfe);
			nes_read_input_device(machine(), cfg >>  4, &m_in_1, 1,  TRUE,data & 0xfe);
			nes_read_input_device(machine(), cfg >>  8, &m_in_2, 2, FALSE,data & 0xfe);
			nes_read_input_device(machine(), cfg >> 12, &m_in_3, 3, FALSE,data & 0xfe);
		}
		else // crazy climber pad
		{
			nes_read_input_device(machine(), 0, &m_in_1, 1,  TRUE,data & 0xfe);
			nes_read_input_device(machine(), 0, &m_in_2, 2, FALSE,data & 0xfe);
			nes_read_input_device(machine(), 0, &m_in_3, 3, FALSE,data & 0xfe);
			nes_read_input_device(machine(), cfg >>  0, &m_in_0, 0,  TRUE,data & 0xfe);
		}

		if (cfg & 0x0f00)
			m_in_0.i0 |= (m_in_2.i0 << 8) | (0x08 << 16);

		if (cfg & 0xf000)
			m_in_1.i0 |= (m_in_3.i0 << 8) | (0x04 << 16);
	}
}


WRITE8_MEMBER(nes_state::nes_IN1_w)
{
}


/**************************

 Disk System emulation

**************************/

void nes_state::fds_irq( int scanline, int vblank, int blanked )
{
	if (m_IRQ_enable_latch)
		m_maincpu->set_input_line(M6502_IRQ_LINE, HOLD_LINE);

	if (m_IRQ_enable)
	{
		if (m_IRQ_count <= 114)
		{
			m_maincpu->set_input_line(M6502_IRQ_LINE, HOLD_LINE);
			m_IRQ_enable = 0;
			m_fds_status0 |= 0x01;
		}
		else
			m_IRQ_count -= 114;
	}
}

READ8_MEMBER(nes_state::nes_fds_r)
{
	UINT8 ret = 0x00;

	switch (offset)
	{
		case 0x00: /* $4030 - disk status 0 */
			ret = m_fds_status0;
			/* clear the disk IRQ detect flag */
			m_fds_status0 &= ~0x01;
			break;
		case 0x01: /* $4031 - data latch */
			/* don't read data if disk is unloaded */
			if (!m_fds_data)
				ret = 0;
			else if (m_fds_current_side)
			{
				// a bunch of games (e.g. bshashsc and fairypin) keep reading beyond the last track
				// what is the expected behavior?
				if (m_fds_head_position > 65500)
					m_fds_head_position = 0;
				ret = m_fds_data[(m_fds_current_side - 1) * 65500 + m_fds_head_position++];
			}
			else
				ret = 0;
			break;
		case 0x02: /* $4032 - disk status 1 */
			/* return "no disk" status if disk is unloaded */
			if (!m_fds_data)
				ret = 1;
			else if (m_fds_last_side != m_fds_current_side)
			{
				/* If we've switched disks, report "no disk" for a few reads */
				ret = 1;
				m_fds_count++;
				if (m_fds_count == 50)
				{
					m_fds_last_side = m_fds_current_side;
					m_fds_count = 0;
				}
			}
			else
				ret = (m_fds_current_side == 0); /* 0 if a disk is inserted */
			break;
		case 0x03: /* $4033 */
			ret = 0x80;
			break;
		default:
			ret = 0x00;
			break;
	}

	return ret;
}

WRITE8_MEMBER(nes_state::nes_fds_w)
{
	switch (offset)
	{
		case 0x00:
			m_IRQ_count_latch = (m_IRQ_count_latch & 0xff00) | data;
			break;
		case 0x01:
			m_IRQ_count_latch = (m_IRQ_count_latch & 0x00ff) | (data << 8);
			break;
		case 0x02:
			m_IRQ_count = m_IRQ_count_latch;
			m_IRQ_enable = data;
			break;
		case 0x03:
			// d0 = sound io (1 = enable)
			// d1 = disk io (1 = enable)
			break;
		case 0x04:
			/* write data out to disk */
			break;
		case 0x05:
			m_fds_motor_on = BIT(data, 0);

			if (BIT(data, 1))
				m_fds_head_position = 0;

			m_fds_read_mode = BIT(data, 2);
			if (BIT(data, 3))
				m_fds_mirroring = PPU_MIRROR_HORZ;
			else
				m_fds_mirroring = PPU_MIRROR_VERT;

			if ((!(data & 0x40)) && (m_fds_write_reg & 0x40))
				m_fds_head_position -= 2; // ???

			m_IRQ_enable_latch = BIT(data, 7);
			m_fds_write_reg = data;
			break;
	}
}

WRITE8_MEMBER(nes_state::fds_chr_w)
{
	m_vram[offset] = data;
}

READ8_MEMBER(nes_state::fds_chr_r)
{
	return m_vram[offset];
}

WRITE8_MEMBER(nes_state::fds_nt_w)
{
	int page = ((offset & 0xc00) >> 10);
	int bank;

	switch (page)
	{
		case 0:
			bank = 0;
			break;
		case 1:
			bank = (m_fds_mirroring == PPU_MIRROR_VERT) ? 1 : 0;
			break;
		case 2:
			bank = (m_fds_mirroring == PPU_MIRROR_VERT) ? 0 : 1;
			break;
		case 3:
		default:
			bank = 1;
			break;
	}

	m_ciram[(bank * 0x400) + (offset & 0x3ff)] = data;
}

READ8_MEMBER(nes_state::fds_nt_r)
{
	int page = ((offset & 0xc00) >> 10);
	int bank;

	switch (page)
	{
		case 0:
			bank = 0;
			break;
		case 1:
			bank = (m_fds_mirroring == PPU_MIRROR_VERT) ? 1 : 0;
			break;
		case 2:
			bank = (m_fds_mirroring == PPU_MIRROR_VERT) ? 0 : 1;
			break;
		case 3:
		default:
			bank = 1;
			break;
	}

	return m_ciram[(bank * 0x400) + (offset & 0x3ff)];
}


// Disk interface

static void nes_load_proc( device_image_interface &image )
{
	nes_state *state = image.device().machine().driver_data<nes_state>();
	int header = 0;
	state->m_fds_sides = 0;

	if (image.length() % 65500)
		header = 0x10;

	state->m_fds_sides = (image.length() - header) / 65500;

	if (!state->m_fds_data)
		state->m_fds_data = auto_alloc_array(image.device().machine(),UINT8,state->m_fds_sides * 65500);    // I don't think we can arrive here ever, probably it can be removed...

	/* if there is an header, skip it */
	image.fseek(header, SEEK_SET);

	image.fread(state->m_fds_data, 65500 * state->m_fds_sides);
	return;
}

static void nes_unload_proc( device_image_interface &image )
{
	nes_state *state = image.device().machine().driver_data<nes_state>();

	/* TODO: should write out changes here as well */
	state->m_fds_sides =  0;
}

DRIVER_INIT_MEMBER(nes_state,famicom)
{
	/* initialize the disk system */
	m_disk_expansion = 1;

	m_fds_sides = 0;
	m_fds_last_side = 0;
	m_fds_count = 0;
	m_fds_motor_on = 0;
	m_fds_door_closed = 0;
	m_fds_current_side = 1;
	m_fds_head_position = 0;
	m_fds_status0 = 0;
	m_fds_read_mode = m_fds_write_reg = 0;

	m_fds_mirroring = PPU_MIRROR_VERT;  // correct?

	m_fds_ram = auto_alloc_array_clear(machine(), UINT8, 0x8000);
	save_pointer(NAME(m_fds_ram), 0x8000);

	floppy_install_load_proc(floppy_get_device(machine(), 0), nes_load_proc);
	floppy_install_unload_proc(floppy_get_device(machine(), 0), nes_unload_proc);
}
