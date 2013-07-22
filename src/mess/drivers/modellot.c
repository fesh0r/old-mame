/***************************************************************************

        General Processor Modello T

        10/12/2012 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class modellot_state : public driver_device
{
public:
	modellot_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, "maincpu") ,
			m_video_ram(*this, "video_ram")
		{ }

	required_device<cpu_device> m_maincpu;
	required_shared_ptr<UINT8> m_video_ram;
	UINT32 screen_update_modellot(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	virtual void machine_reset();

	DECLARE_READ8_MEMBER(modellot_77);
	DECLARE_READ8_MEMBER(modellot_ff);
};

static ADDRESS_MAP_START(modellot_mem, AS_PROGRAM, 8, modellot_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xc3ff) AM_RAM AM_SHARE("video_ram")
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(modellot_io, AS_IO, 8, modellot_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x77, 0x77) AM_READ(modellot_77)
	AM_RANGE(0xff, 0xff) AM_READ(modellot_ff)
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( modellot )
INPUT_PORTS_END

READ8_MEMBER( modellot_state::modellot_77)
{
	return 0x04;
}

READ8_MEMBER( modellot_state::modellot_ff)
{
	return 0x00;
}

void modellot_state::machine_reset()
{
	m_maincpu->set_state_int(Z80_PC, 0xe000);
}

const gfx_layout modellot_charlayout =
{
	8, 16,              /* 8x16 characters */
	128,                /* 128 characters */
	1,              /* 1 bits per pixel */
	{0},                /* no bitplanes; 1 bit per pixel */
	{0,1,2,3,4,5,6,7},
	{0, 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8,
		0x400*8, 0x401*8, 0x402*8, 0x403*8, 0x404*8, 0x405*8, 0x406*8, 0x407*8},
	8*8             /* space between characters */
};

static GFXDECODE_START( modellot )
	GFXDECODE_ENTRY( "chargen", 0x0000, modellot_charlayout, 0, 1 )
GFXDECODE_END


UINT32 modellot_state::screen_update_modellot(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int x,y;

	for(y = 0; y < 16; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = m_video_ram[x + y*64];
			drawgfx_opaque(bitmap, cliprect, machine().gfx[0], code, 0, 0, 0, x*8, y*16);
		}
	}
	return 0;
}

static MACHINE_CONFIG_START( modellot, modellot_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(modellot_mem)
	MCFG_CPU_IO_MAP(modellot_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(64*8, 16*16)
	MCFG_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 16*16-1)
	MCFG_SCREEN_UPDATE_DRIVER(modellot_state, screen_update_modellot)

	MCFG_GFXDECODE( modellot )

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT_OVERRIDE(driver_device, black_and_white)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( modellot )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	//ROM_LOAD( "fdc8119.u3", 0x0000, 0x0400, CRC(a8aee944) SHA1(f2cc598ed2e7a1a620e2f3f53c1a573965f6af26))
	ROM_LOAD( "dt49-48.u1", 0xe000, 0x0400, CRC(2441c438) SHA1(832994a4214a744b7e19e5f74000c95ae65e3759))
	ROM_LOAD( "ht20.u2",    0xe400, 0x0400, CRC(497c0495) SHA1(d03beebc4c31284729f6eac3bdf1fbf44adf7fff))

	ROM_REGION( 0x0800, "chargen", ROMREGION_INVERT )
	ROM_LOAD( "gcem1.u3", 0x0000, 0x0200, CRC(e7739268) SHA1(091ef69282abe657d5f38c70a572964f5200a1d5))
	ROM_CONTINUE(0x400, 0x200)
	ROM_LOAD( "gcem2.u4", 0x0200, 0x0200, CRC(6614330e) SHA1(880a541fb0ef6f37ac89439f9ea75a313c3e53d6))
	ROM_CONTINUE(0x600, 0x200)
ROM_END

/* Driver */
COMP( 1979, modellot, 0, 0, modellot, modellot, driver_device, 0, "General Processor", "Modello T", GAME_IS_SKELETON)
