/***************************************************************************
   
        VTA-2000 Terminal
		
			board images : http://fotki.yandex.ru/users/lodedome/album/93699?p=0
			
		BDP-15 board only

        29/11/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

class vta2000_state : public driver_device
{
public:
	vta2000_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	const UINT8 *FNT;
	const UINT8 *videoram;
};

static ADDRESS_MAP_START(vta2000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x5fff ) AM_ROM
	AM_RANGE( 0x8000, 0xffff ) AM_RAM AM_REGION("maincpu", 0x8000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(vta2000_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vta2000 )
INPUT_PORTS_END


static MACHINE_RESET(vta2000) 
{	
}

static VIDEO_START( vta2000 )
{
	vta2000_state *state = machine->driver_data<vta2000_state>();
	state->FNT = machine->region("chargen")->base();
	state->videoram = machine->region("maincpu")->base()+0x80a0;
}

static VIDEO_UPDATE( vta2000 )
/* Video is 80A0 to B21F, this resolves to 48 lines of 132 character pairs.
There should therefore be hardware scroll registers, for down and sideways.
Each character pair consists of a data byte followed by an attribute.

Here we just show the first 80x25, with no scrolling. */
{
	vta2000_state *state = screen->machine->driver_data<vta2000_state>();
	//static UINT8 framecnt=0;
	UINT8 y,ra,gfx,attr;
	UINT16 sy=0,ma=0,x,xx=0,chr;

	//framecnt++;

	//ma = (scroll-down * 132) + scroll-sideways;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 12; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			xx = ma << 1;
			for (x = ma; x < ma + 80; x++)
			{
				chr = state->videoram[xx++];
				attr = state->videoram[xx++];

				if ((chr & 0x60)==0x60)
					chr+=256;

				gfx = state->FNT[(chr<<4) | ra ];

				/* Display a scanline of a character */
				*p++ = ( gfx & 0x80 ) ? 1 : 0;
				*p++ = ( gfx & 0x40 ) ? 1 : 0;
				*p++ = ( gfx & 0x20 ) ? 1 : 0;
				*p++ = ( gfx & 0x10 ) ? 1 : 0;
				*p++ = ( gfx & 0x08 ) ? 1 : 0;
				*p++ = ( gfx & 0x04 ) ? 1 : 0;
				*p++ = ( gfx & 0x02 ) ? 1 : 0;
				*p++ = ( gfx & 0x01 ) ? 1 : 0;
			}
		}
		ma+=132;
	}
	return 0;
}


/* F4 Character Displayer */
static const gfx_layout vta2000_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	512,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( vta2000 )
	GFXDECODE_ENTRY( "chargen", 0x0000, vta2000_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( vta2000, vta2000_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8080, XTAL_4MHz / 4)
	MCFG_CPU_PROGRAM_MAP(vta2000_mem)
	MCFG_CPU_IO_MAP(vta2000_io)	

	MCFG_MACHINE_RESET(vta2000)
	
	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(80*8, 25*12)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 25*12-1)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
	MCFG_GFXDECODE(vta2000)

	MCFG_VIDEO_START(vta2000)
	MCFG_VIDEO_UPDATE(vta2000)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( vta2000 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bdp-15_11.rom", 0x4000, 0x2000, CRC(d4abe3e9) SHA1(ab1973306e263b0f66f2e1ede50cb5230f8d69d5) )
	ROM_LOAD( "bdp-15_12.rom", 0x2000, 0x2000, CRC(4a5fe332) SHA1(f1401c26687236184fec0558cc890e796d7d5c77) )
	ROM_LOAD( "bdp-15_13.rom", 0x0000, 0x2000, CRC(b6b89d90) SHA1(0356d7ba77013b8a79986689fb22ef4107ef885b) )

	ROM_REGION(0x2000, "chargen", ROMREGION_INVERT )
	ROM_LOAD( "bdp-15_14.rom", 0x0000, 0x2000, CRC(a1dc4f8e) SHA1(873fd211f44713b713d73163de2d8b5db83d2143) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( ????, vta2000,  0,       0, 	vta2000, 	vta2000, 	 0,  	  "<unknown>",   "VTA-2000",		GAME_NOT_WORKING | GAME_NO_SOUND)

