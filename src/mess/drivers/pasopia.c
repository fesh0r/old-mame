/***************************************************************************

    Toshiba Pasopia

    TODO:
    - just like Pasopia 7, z80pio is broken, hence it doesn't clear irqs
      after the first one (0xfe79 is the work ram buffer for keyboard).
    - machine emulation needs merging with Pasopia 7 (video emulation is
      completely different tho)

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "video/mc6845.h"
#include "machine/terminal.h"

class pasopia_state : public driver_device
{
public:
	pasopia_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_ppi0(*this, "ppi8255_0"),
	m_ppi1(*this, "ppi8255_1"),
	m_ppi2(*this, "ppi8255_2"),
	m_ctc(*this, "z80ctc"),
	m_pio(*this, "z80pio"),
	m_crtc(*this, "crtc")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<i8255_device> m_ppi0;
	required_device<i8255_device> m_ppi1;
	required_device<i8255_device> m_ppi2;
	required_device<device_t> m_ctc;
	required_device<device_t> m_pio;
	required_device<mc6845_device> m_crtc;
	DECLARE_READ8_MEMBER(pasopia_romram_r);
	DECLARE_WRITE8_MEMBER(pasopia_ram_w);
	DECLARE_WRITE8_MEMBER(pasopia_ctrl_w);
	DECLARE_WRITE8_MEMBER(pasopia_6845_address_w);
	DECLARE_WRITE8_MEMBER(pasopia_6845_data_w);
	DECLARE_WRITE8_MEMBER(vram_addr_lo_w);
	DECLARE_WRITE8_MEMBER(vram_latch_w);
	DECLARE_READ8_MEMBER(vram_latch_r);
	DECLARE_READ8_MEMBER(portb_1_r);
	DECLARE_WRITE8_MEMBER(vram_addr_hi_w);
	DECLARE_WRITE8_MEMBER(screen_mode_w);
	DECLARE_READ8_MEMBER(rombank_r);
	DECLARE_READ8_MEMBER(testa_r);
	DECLARE_READ8_MEMBER(testb_r);
	DECLARE_WRITE_LINE_MEMBER(testa_w);
	DECLARE_WRITE_LINE_MEMBER(testb_w);
	DECLARE_WRITE8_MEMBER(kbd_put);
	UINT8 m_ram_bank;
	UINT8 *m_p_rom;
	UINT8 *m_p_wram;
	UINT8 *m_p_vram;
	const UINT8 *m_p_chargen;
	UINT8 m_hblank;
	UINT16 m_vram_addr;
	UINT8 m_vram_latch,m_attr_latch;
	UINT8 m_video_wl;
	UINT8 m_gfx_mode;
	UINT8 m_crtc_vreg[0x100],m_crtc_index;
};

static VIDEO_START( pasopia )
{
	pasopia_state *state = machine.driver_data<pasopia_state>();
	state->m_p_chargen = machine.region("chargen")->base();
}

static SCREEN_UPDATE( pasopia )
{
	pasopia_state *state = screen->machine().driver_data<pasopia_state>();
	state->m_crtc->update(bitmap, cliprect);
	return 0;
}

MC6845_UPDATE_ROW( pasopia_update_row )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();
	UINT8 chr,gfx,fg=7,bg=0; // colours need to be determined
	UINT16 mem,x;
	UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)
	{
		UINT8 inv=0;
		if (x == cursor_x) inv=0xff;
		mem = (ma + x) & 0xfff;
		chr = state->m_p_vram[mem];

		/* get pattern of pixels for that character scanline */
		gfx = state->m_p_chargen[(chr<<3) | ra] ^ inv;

		/* Display a scanline of a character */
		*p++ = BIT(gfx, 7) ? fg : bg;
		*p++ = BIT(gfx, 6) ? fg : bg;
		*p++ = BIT(gfx, 5) ? fg : bg;
		*p++ = BIT(gfx, 4) ? fg : bg;
		*p++ = BIT(gfx, 3) ? fg : bg;
		*p++ = BIT(gfx, 2) ? fg : bg;
		*p++ = BIT(gfx, 1) ? fg : bg;
		*p++ = BIT(gfx, 0) ? fg : bg;
	}
}

READ8_MEMBER( pasopia_state::pasopia_romram_r )
{
	if (m_ram_bank)
		return m_p_wram[offset];

	return m_p_rom[offset];
}

WRITE8_MEMBER( pasopia_state::pasopia_ram_w )
{
	m_p_wram[offset] = data;
}

WRITE8_MEMBER( pasopia_state::pasopia_ctrl_w )
{
	m_ram_bank = data & 2;
}

static ADDRESS_MAP_START(pasopia_map, AS_PROGRAM, 8, pasopia_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x7fff) AM_READWRITE(pasopia_romram_r,pasopia_ram_w)
	AM_RANGE(0x8000,0xffff) AM_RAM
ADDRESS_MAP_END


WRITE8_MEMBER( pasopia_state::pasopia_6845_address_w )
{
	data &= 0x1f;
	m_crtc_index = data;
	m_crtc->address_w( space, offset, data );
}

WRITE8_MEMBER( pasopia_state::pasopia_6845_data_w )
{
	m_crtc_vreg[m_crtc_index] = data;
	m_crtc->register_w( space, offset, data );
}

static ADDRESS_MAP_START(pasopia_io, AS_IO, 8, pasopia_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00,0x03) AM_DEVREADWRITE("ppi8255_0", i8255_device, read, write)
	AM_RANGE(0x08,0x0b) AM_DEVREADWRITE("ppi8255_1", i8255_device, read, write)
	AM_RANGE(0x10,0x10) AM_WRITE(pasopia_6845_address_w)
	AM_RANGE(0x11,0x11) AM_WRITE(pasopia_6845_data_w)
//  0x18 - 0x1b pac2
	AM_RANGE(0x20,0x23) AM_DEVREADWRITE("ppi8255_2", i8255_device, read, write)
	AM_RANGE(0x28,0x2b) AM_DEVREADWRITE_LEGACY("z80ctc", z80ctc_r, z80ctc_w)
	AM_RANGE(0x30,0x33) AM_DEVREADWRITE_LEGACY("z80pio", z80pio_ba_cd_r, z80pio_cd_ba_w)
//  0x38 printer
	AM_RANGE(0x3c,0x3c) AM_WRITE(pasopia_ctrl_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pasopia )
INPUT_PORTS_END

static MACHINE_START(pasopia)
{
	pasopia_state *state = machine.driver_data<pasopia_state>();

	state->m_p_rom = machine.region("ipl")->base();
	state->m_p_wram = machine.region("wram")->base();
	state->m_p_vram = machine.region("vram")->base();
}

static MACHINE_RESET(pasopia)
{
}

WRITE8_MEMBER( pasopia_state::vram_addr_lo_w )
{
	m_vram_addr = (m_vram_addr & 0xff00) | data;
}

WRITE8_MEMBER( pasopia_state::vram_latch_w )
{
	m_vram_latch = data;
}

READ8_MEMBER( pasopia_state::vram_latch_r )
{
	return m_p_vram[m_vram_addr & 0x3fff];
}

static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_DRIVER_MEMBER(pasopia_state, vram_addr_lo_w),		/* Port A write */
	DEVCB_NULL,		/* Port B read */
	DEVCB_DRIVER_MEMBER(pasopia_state, vram_latch_w),		/* Port B write */
	DEVCB_DRIVER_MEMBER(pasopia_state, vram_latch_r),		/* Port C read */
	DEVCB_NULL		/* Port C write */
};

READ8_MEMBER( pasopia_state::portb_1_r )
{
	/*
    x--- ---- attribute latch
    -x-- ---- hblank
    --x- ---- vblank
    ---x ---- LCD system mode, active low
    */
	UINT8 grph_latch,lcd_mode;

	m_hblank ^= 0x40; //TODO
	grph_latch = (m_p_vram[(m_vram_addr & 0x3fff)+0x4000] & 0x80);
	lcd_mode = 0x10;

	return m_hblank | lcd_mode | grph_latch; //bit 4: LCD mode
}


WRITE8_MEMBER( pasopia_state::vram_addr_hi_w )
{
	m_attr_latch = (data & 0x80) | (m_attr_latch & 0x7f);
	if (data & 0x40 && ((m_video_wl & 0x40) == 0))
	{
		m_p_vram[m_vram_addr & 0x3fff] = m_vram_latch;
		m_p_vram[(m_vram_addr & 0x3fff)+0x4000] = m_attr_latch;
	}

	m_video_wl = data & 0x40;
	m_vram_addr = (m_vram_addr & 0xff) | ((data & 0x3f) << 8);
}

WRITE8_MEMBER( pasopia_state::screen_mode_w )
{
	m_gfx_mode = (data & 0xe0) >> 5;
	m_attr_latch = (m_attr_latch & 0x80) | (data & 7);
	printf("Screen Mode=%02x\n",data);
}

static I8255A_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_DRIVER_MEMBER(pasopia_state, screen_mode_w),		/* Port A write */
	DEVCB_DRIVER_MEMBER(pasopia_state, portb_1_r),		/* Port B read */
	DEVCB_NULL,		/* Port B write */
	DEVCB_NULL,		/* Port C read */
	DEVCB_DRIVER_MEMBER(pasopia_state, vram_addr_hi_w)		/* Port C write */
};

READ8_MEMBER( pasopia_state::rombank_r )
{
	return m_ram_bank<<1;
}

static I8255A_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_NULL,		/* Port A write */
	DEVCB_NULL,		/* Port B read */
	DEVCB_NULL,		/* Port B write */
	DEVCB_DRIVER_MEMBER(pasopia_state, rombank_r),		/* Port C read */
	DEVCB_NULL		/* Port C write */
};

static Z80CTC_INTERFACE( ctc_intf )
{
	0,					// timer disables
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),		// interrupt handler
	DEVCB_LINE(z80ctc_trg1_w),		// ZC/TO0 callback
	DEVCB_LINE(z80ctc_trg2_w),		// ZC/TO1 callback, beep interface
	DEVCB_LINE(z80ctc_trg3_w)		// ZC/TO2 callback
};

READ8_MEMBER( pasopia_state::testa_r )
{
	printf("A R\n");
	return 0xff;
}

READ8_MEMBER( pasopia_state::testb_r )
{
	printf("B R\n");
	return 0xff;
}

WRITE_LINE_MEMBER( pasopia_state::testa_w )
{
	printf("A %02x\n",state);
}

WRITE_LINE_MEMBER( pasopia_state::testb_w )
{
	printf("B %02x\n",state);
}

static Z80PIO_INTERFACE( z80pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), //doesn't work?
	DEVCB_DRIVER_MEMBER(pasopia_state, testa_r),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pasopia_state, testa_w),
	DEVCB_DRIVER_MEMBER(pasopia_state, testb_r),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pasopia_state, testb_w)
};

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,		/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	pasopia_update_row,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static const gfx_layout p7_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( pasopia )
	GFXDECODE_ENTRY( "chargen", 0x0000, p7_chars_8x8, 0, 4 )
GFXDECODE_END

static const z80_daisy_config pasopia_daisy[] =
{
	{ "z80ctc" },
	{ "z80pio" },
//  { "upd765" }, /* TODO */
	{ NULL }
};

// temporary hack
WRITE8_MEMBER( pasopia_state::kbd_put )
{
	address_space *mem = m_maincpu->memory().space(AS_PROGRAM);
	mem->write_byte(0xfe79, data);
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(pasopia_state, kbd_put)
};

static MACHINE_CONFIG_START( pasopia, pasopia_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 4000000)
	MCFG_CPU_PROGRAM_MAP(pasopia_map)
	MCFG_CPU_IO_MAP(pasopia_io)
	MCFG_CPU_CONFIG(pasopia_daisy)

	MCFG_MACHINE_START(pasopia)
	MCFG_MACHINE_RESET(pasopia)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_VIDEO_START(pasopia)
	MCFG_SCREEN_UPDATE(pasopia)
	MCFG_GFXDECODE(pasopia)
	MCFG_PALETTE_LENGTH(8)

	/* Devices */
	MCFG_MC6845_ADD("crtc", H46505, XTAL_4MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */
	MCFG_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )
	MCFG_I8255A_ADD( "ppi8255_1", ppi8255_intf_1 )
	MCFG_I8255A_ADD( "ppi8255_2", ppi8255_intf_2 )
	MCFG_Z80CTC_ADD( "z80ctc", XTAL_4MHz, ctc_intf )
	MCFG_Z80PIO_ADD( "z80pio", XTAL_4MHz, z80pio_intf )

	// temporary hack
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pasopia )
	ROM_REGION( 0x8000, "ipl", 0 )
	ROM_LOAD( "tbasic.rom", 0x0000, 0x8000, CRC(f53774ff) SHA1(bbec45a3bad8d184505cc6fe1f6e2e60a7fb53f2))

	ROM_REGION( 0x0800, "chargen", 0 )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, BAD_DUMP CRC(a91c45a9) SHA1(a472adf791b9bac3dfa6437662e1a9e94a88b412)) //stolen from pasopia7

	ROM_REGION( 0x8000, "wram", ROMREGION_ERASE00 )

	ROM_REGION( 0x8000, "vram", ROMREGION_ERASE00 )
ROM_END

/* Driver */

/*    YEAR  NAME     PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY      FULLNAME       FLAGS */
COMP( 1986, pasopia, 0,      0,       pasopia,   pasopia, 0,     "Toshiba",   "Pasopia", GAME_NOT_WORKING | GAME_NO_SOUND)
