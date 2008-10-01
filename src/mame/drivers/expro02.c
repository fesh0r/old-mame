/*

   Kaneko EXPRO-02 board

  Used by the newer revisions of Gals Panic

 Notes:
  - In gfx data banking function, some strange gfx are shown. Timing issue?


Gals Panic
Kaneko, 1990

PCB Layout
----------

EXPRO-02
|-------------------------------------------------------------------------|
|     M6295 PM007E.U47 12MHz          PM000E.U74    PM004E.U86            |
|      VOL  PM008E.U46 16MHz 62256          PM002E.U76   PM109U_U88-01.U88|
|LA4460                                                                   |
|             PAL PAL  PAL                                                |
|             PAL PAL  PAL            PM001E.U73    PM005E.U85            |
|                      PAL   62256          PM003E.U75   PM110U_U87-01.U87|
|    |--------------------|                                               |
|    |       68000        |                                               |
|    |                    |  41464                                        |
|    |--------------------|  41464              PM017E.U84                |
|    GP-U27             PAL  41464  |---------|                           |
|J   PAL   GP-U41            41464  |KANEKO   |                           |
|A MC-8282  PAL              41464  |VU-002   | PM006E.U83     PM018E.U94 |
|M                     6116  41464  |         |                           |
|M                                  |         |          PM019U_U93-01.U93|
|A                     6116         |---------| PM206E.U82                |
|            HM53461                                                      |
|    PAL     HM53461   PAL    |-------|         CALC1-CHIP                |
|            HM53461   PAL    |KANEKO |                      PM016E.U92   |
|    PAL     HM53461   PAL    |VIEW2- |    6264                           |
|            HM53461   PAL    |   CHIP|                      PM015E.U91   |
|    PAL     HM53461   PAL    |-------|    6264                           |
|DSW2         6116     PAL   PAL                             PM014E.U90   |
|                      PAL                 PAL                            |
|DSW1         6116                         PAL               PM013E.U89   |
|-------------------------------------------------------------------------|

    Notes
    -----
    GP-U27/U41 - These are DIP40 chips, but are not MCUs because there is no stable
                 clock input on any of the pins of these chips. They're not ROMs either
                 because the pinout doesn't match any known EPROMs.
                 There are no markings on the chips other than 'GP-U27' & 'GP-U41'
                 If GP-U41 is removed, on bootup the PCB gives an error 'BG ERROR' and
                 a memory address. If GP-U27 is removed, the PCB works but there are no
                 background graphics.

    68000 clock - 12.0MHz
    CALC1-CHIP clock - 16.0MHz
    GP-U41 clocks - pins 21 & 22 - 12.0MHz, pins 1 & 2 - 6.0MHz, pins 8 & 9 - 15.6249kHz (HSync?)
    GP-U27 clock - none (so it's not an MCU)

    (TODO: which is correct?)
    OKI M6295 clock - 2.0MHz (12/6). pin7 = low
    OKI M6295 clock - 2.000 MHz [16/8]. Sample rate 2000000/165

    VSync - 60Hz
    HSync - 15.68kHz
    MC-8282 - Kaneko custom I/O JAMMA ceramic module
    41464 - 64k x4 DRAM
    HM53461 - 64k x4 Multiport CMOS VRAM
    6116 - 2k x8 SRAM
    6264 - 8k x8 SRAM
    62256 - 32k x8 SRAM

**************************************************************************************************

Gals Panic (Japan)
(C)1990 Kaneko

EXPRO-02 PCB
M6100575A GALS PANIC (PCB manufactured by Taito)

CPU: MC68000
Sound: M6295
OSC: 12.0000MHz, 16.0000MHz
Custom: VU-002, VIEW2, CACL1

ROMs:
PM109J.U88 (OKI M271000ZB) - Main programs
PM110J.U87 (OKI M271000ZB)
PM004E.U86
PM005E.U85
PM-002E.U76
PM-003E.U75
PM-000E.U74
PM-001E.U73

PM006E.U83 - Sprites
PM206E.U82
PM018E.U94

PM-013E.U89 (40pin mask)
PM-014E.U90
PM-015E.U91
PM-016E.U92

GP-U41.U41 (40pin mask?)
GP-U27.U27


PAL/GALs:
U10 (18CV8)
U11 (18CV8)
U12 (18CV8)
U15 (22CV10)
GP-U28 (16V8)
U29 (18CV8)
U32 (18CV8)
U33 (18CV8)
U34 (18CV8)
U35 (18CV8)
U36 (22CV10)
U37 (22CV10)
U38 (18CV8)
U42 (18CV8)
GP-U44 (16V8)
U45 (18CV8)
U48 (18CV8)
GP-U57 (16V8)
GP-U58 (16V8)
GP-U60 (16V8)
U77 (22CV10)
U78 (22CV10)
*/

#include "driver.h"
#include "deprecat.h"
#include "includes/kaneko16.h"
#include "sound/okim6295.h"

static INPUT_PORTS_START( galsnew )
	PORT_START("DSW1")
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x000C, 0x000C, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x000C, "3" )
	PORT_DIPSETTING(      0x0008, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )	/* flip screen? */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Test ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Coin B-1" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Coin B-2" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	/* Coinage - Japan (0x03ffff.b = 01) and US (0x03ffff.b = 02)
    PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
    PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
    */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("DSW3")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static INPUT_PORTS_START( galsnewa )
	PORT_START("DSW1")
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_BIT( 0x000C, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0xC000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )	/* flip screen? */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Test ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Coin B-1" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Coin B-2" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	/* Coinage - Japan (0x03ffff.b = 01) and US (0x03ffff.b = 02)
    PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
    PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
    */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("DSW3")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static WRITE16_HANDLER( galsnew_paletteram_w )
{
	data = COMBINE_DATA(&paletteram16[offset]);
	palette_set_color_rgb(machine,offset,pal5bit(data >> 6),pal5bit(data >> 11),pal5bit(data >> 1));
}

static WRITE16_HANDLER( galsnew_6295_bankswitch_w )
{
	if (ACCESSING_BITS_8_15)
	{
		UINT8 *rom = memory_region(machine, "oki");
		memcpy(&rom[0x30000],&rom[0x40000 + ((data >> 8) & 0x0f) * 0x10000],0x10000);
	}
}

static UINT16 vram_0_bank_num = 0, vram_1_bank_num = 0;

WRITE16_HANDLER(galsnew_vram_0_bank_w)
{
	int i;
	if(vram_0_bank_num != data)
	{
		for(i = 0; i < 0x1000 / 2; i += 2)
		{
			if(kaneko16_vram_0[i])
			{
				kaneko16_vram_0_w(machine, i+1, data << 8, 0xFF00);
			}
		}
		vram_0_bank_num = data;
	}
}

WRITE16_HANDLER(galsnew_vram_1_bank_w)
{
	int i;
	if(vram_1_bank_num != data)
	{
		for(i = 0; i < 0x1000 / 2; i += 2)
		{
			if(kaneko16_vram_1[i])
			{
				kaneko16_vram_1_w(machine, i+1, data << 8, 0xFF00);
			}
		}
		vram_1_bank_num = data;
	}
}

static ADDRESS_MAP_START( galsnew, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM // main program
	AM_RANGE(0x080000, 0x0fffff) AM_ROM AM_REGION("user2",0) // other data
	AM_RANGE(0x100000, 0x3fffff) AM_ROM AM_REGION("user1",0) // main data
	AM_RANGE(0x400000, 0x400001) AM_READWRITE(okim6295_status_0_lsb_r,okim6295_data_0_lsb_w)


	AM_RANGE(0x500000, 0x51ffff) AM_RAM AM_BASE(&galsnew_bg_pixram)
	AM_RANGE(0x520000, 0x53ffff) AM_RAM AM_BASE(&galsnew_fg_pixram)

	AM_RANGE(0x580000, 0x580fff) AM_READWRITE(SMH_RAM,kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x581000, 0x581fff) AM_READWRITE(SMH_RAM,kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0x582000, 0x582fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0x583000, 0x583fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//

	AM_RANGE(0x600000, 0x600fff) AM_READWRITE(SMH_RAM,galsnew_paletteram_w) AM_BASE(&paletteram16) // palette?

	AM_RANGE(0x680000, 0x68001f) AM_READWRITE(SMH_RAM,kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs) // sprite regs? tileregs?

	AM_RANGE(0x700000, 0x700fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)	 // sprites? 0x72f words tested

	AM_RANGE(0x780000, 0x78001f) AM_READWRITE(SMH_RAM,kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs) // sprite regs? tileregs?

	AM_RANGE(0x800000, 0x800001) AM_READ_PORT("DSW1")
	AM_RANGE(0x800002, 0x800003) AM_READ_PORT("DSW2")
	AM_RANGE(0x800004, 0x800005) AM_READ_PORT("DSW3")

	AM_RANGE(0x900000, 0x900001) AM_WRITE(galsnew_6295_bankswitch_w)

	AM_RANGE(0xa00000, 0xa00001) AM_WRITENOP	/* ??? */

	AM_RANGE(0xc80000, 0xc8ffff) AM_RAM

	AM_RANGE(0xd80000, 0xd80001) AM_WRITE(galsnew_vram_1_bank_w)	/* ??? */

	AM_RANGE(0xe00000, 0xe00015) AM_READWRITE(galpanib_calc_r,galpanib_calc_w) /* CALC1 MCU interaction (simulated) */

	AM_RANGE(0xe80000, 0xe80001) AM_WRITE(galsnew_vram_0_bank_w)	/* ??? */
ADDRESS_MAP_END


static INTERRUPT_GEN( galsnew_interrupt )
{
	cpunum_set_input_line(machine, 0, cpu_getiloops() + 3, HOLD_LINE);	/* IRQs 5, 4, and 3 */
}

static MACHINE_RESET( galsnew )
{
	kaneko16_sprite_type  = 0;

	kaneko16_sprite_xoffs = 0;
	kaneko16_sprite_yoffs = -1*0x40; // align testgrid with bitmap in service mode

	// priorities not verified
	kaneko16_priority.sprite[0] = 8;	// above all
	kaneko16_priority.sprite[1] = 8;	// above all
	kaneko16_priority.sprite[2] = 8;	// above all
	kaneko16_priority.sprite[3] = 8;	// above all
}

static const gfx_layout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(8*8*4*0,4),   STEP8(8*8*4*1,4)   },
	{ STEP8(8*8*4*0,8*4), STEP8(8*8*4*2,8*4) },
	16*16*4
};


static GFXDECODE_START( 1x4bit_1x4bit )
	GFXDECODE_ENTRY( "gfx1", 0, layout_16x16x4, 0x100,	    0x40 ) // [0] Sprites
	GFXDECODE_ENTRY( "gfx2", 0, layout_16x16x4, 0x400,		0x40 ) // [0] bg tiles
GFXDECODE_END

static MACHINE_DRIVER_START( galsnew )

	/* basic machine hardware */
	MDRV_CPU_ADD("main", M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(galsnew,0)
	MDRV_CPU_VBLANK_INT_HACK(galsnew_interrupt,3)

	/* CALC01 MCU @ 16Mhz (unknown type, simulated) */

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-32-1)

	MDRV_GFXDECODE(1x4bit_1x4bit)
	MDRV_PALETTE_LENGTH(2048 + 32768)
	MDRV_MACHINE_RESET( galsnew )

	MDRV_VIDEO_START(galsnew)
	MDRV_VIDEO_UPDATE(galsnew)
	MDRV_PALETTE_INIT(berlwall)

	/* arm watchdog */
	MDRV_WATCHDOG_TIME_INIT(UINT64_ATTOTIME_IN_SEC(3))	/* a guess, and certainly wrong */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("oki", OKIM6295, 12000000/6)
	MDRV_SOUND_CONFIG(okim6295_interface_pin7low)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

/* the tile roms seem lineswapped.. but I don't know how to descramble them yet */
DRIVER_INIT(galsnew)
{
	UINT32 *src = (UINT32 *)memory_region(machine, "gfx3" );
	UINT32 *dst = (UINT32 *)memory_region(machine, "gfx2" );
	int x, source, offset,inv;

	for (x=0; x<0x80000;x++)
	{
		inv = ~x;
		offset = x ^ (0xC30 | (inv >> 1 & 0x10200));
		offset = (offset & ~0x07) | ((0x02 - (offset & 0x07)) & 0x07);
		if(x&0x0010)
			offset = (offset & ~0x60) | (((offset & 0x60) + 0x20) & 0x60);
		if((x&((inv >> 1 & 0x10000) | 0x2000)) == 0x02000)
			offset ^= 0x1000;
		if(!(x&(0x1000 | (x >> 4 & 0x2000))))
		{
			offset ^= 0x100;
			if(!(x&0x20000) && (x&0x12000) == 0x02000)
			{
				offset = (offset & ~0x180) | (((offset & 0x180) + 0x180) & 0x180);
				if((x&0x180) == 0x100)
				{
					offset ^= 0x400;
					if((x&0x480) == 0x400)
						offset ^= 0x200;
				}
			}
		}
		else
		{
			offset = (offset & ~0x180) | (((offset & 0x180) + 0x80) & 0x180);
			if((x&0x180) == 0x100)
			{
				offset ^= 0x400;
				if((x&0x480) == 0x400)
					offset ^= 0x200;
			}
		}
		if((x&0x7) < 0x03)
		{
			if(!(x&0x800))
			{
				offset ^= 0x4000;
				if(x&0x4000)
				{
					offset ^= 0x08;
					if(x&0x08)
						offset = (offset & ~0x70) | (((offset & 0x70) + 0x10) & 0x70);
				}
			}
			offset ^= 0x800;
		}
		if((x & 0x30000) == 0x10000)
			source = x;
		else
			source = x ^ 0x2000;

		dst[source] = (src[offset] << 4 & 0xF0F0F0F0) | (src[offset] >> 4 & 0x0F0F0F0F);
	}
}

ROM_START( galsnew ) /* EXPRO-02 PCB */
	ROM_REGION( 0x40000, "main", 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110u_u87-01.u87", 0x000000, 0x20000, CRC(b793a57d) SHA1(12d57b2b4add532f0d0453c25b30d34b3449d717) ) /* US region */
	ROM_LOAD16_BYTE( "pm109u_u88-01.u88", 0x000001, 0x20000, CRC(35b936f8) SHA1(d272067f10542d511a777802cafa4d72b93fa5e8) )

	ROM_REGION16_BE( 0x300000, "user1", 0 )	/* 68000 data */
	ROM_LOAD16_BYTE( "pm004e.u86", 0x000001, 0x80000, CRC(d3af52bc) SHA1(46be057106388578defecab1cdd1793ec76ebe92) )
	ROM_LOAD16_BYTE( "pm005e.u85", 0x000000, 0x80000, CRC(d7ec650c) SHA1(6c2250c74381497154bf516e0cf1db6bb56bb446) )
	ROM_LOAD16_BYTE( "pm000e.u74", 0x100001, 0x80000, CRC(5d220f3f) SHA1(7ff373e01027c8832712f7a2d732f8e49b875878) )
	ROM_LOAD16_BYTE( "pm001e.u73", 0x100000, 0x80000, CRC(90433eb1) SHA1(8688a85747ad9ecac395d782f130baa64fb9d12b) )
	ROM_LOAD16_BYTE( "pm002e.u76", 0x200001, 0x80000, CRC(713ee898) SHA1(c9f608a57fb90e5ee15eb76a74a7afcc406d5b4e) )
	ROM_LOAD16_BYTE( "pm003e.u75", 0x200000, 0x80000, CRC(6bb060fd) SHA1(4fc3946866c5a55e8340b62b5ad9beae723ce0da) )

	ROM_REGION16_BE( 0x80000, "user2", 0 )	/* contains real (non-cartoon) women, used after each 3rd round */
	ROM_LOAD16_WORD_SWAP( "pm017e.u84", 0x00000, 0x80000, CRC(bc41b6ca) SHA1(0aeaf024dd7c84550e7df27230a1d4f04cc1d61c) )

	ROM_REGION( 0x200000, "gfx1", ROMREGION_ERASEFF )	/* sprites */
	/* the 06e rom from the other type gals panic board ends up split across 2 roms here */
	ROM_LOAD( "pm006e.u83",        0x000000, 0x080000, CRC(a7555d9a) SHA1(f95821b3358d9ab03ca9ead38fd358062259d89d) )
	ROM_LOAD( "pm206e.u82",        0x080000, 0x080000, CRC(cc978baa) SHA1(59a95bcbaeca9d356f61ea42af4da116afbb1491) )
	ROM_LOAD( "pm018e.u94",        0x100000, 0x080000, CRC(f542d708) SHA1(f515cca9e96401303ed45b4372f6079f29b7a999) )
	ROM_LOAD( "pm019u_u93-01.u93", 0x180000, 0x010000, CRC(3cb79005) SHA1(05a0b993b9071467265067c3762644f46343d8de) ) // ?? seems to be an extra / replacement enemy?, not sure where it maps, or when it's used, it might load over another rom

	ROM_REGION( 0x200000, "gfx2", ROMREGION_DISPOSE )	/* sprites */

	ROM_REGION( 0x200000, "gfx3", ROMREGION_DISPOSE )	/* sprites - encrypted */
	ROM_LOAD( "pm013e.u89", 0x000000, 0x080000, CRC(10f27b05) SHA1(0f8ade713f6b430b5a23370a17326d53229951de) )
	ROM_LOAD( "pm014e.u90", 0x080000, 0x080000, CRC(2f367106) SHA1(1cd16e286e77e8e1b7668bbb6f2978101656b720) )
	ROM_LOAD( "pm015e.u91", 0x100000, 0x080000, CRC(a563f8ef) SHA1(6e4171746e4d401992bf3a7619d5bed0063d57e5) )
	ROM_LOAD( "pm016e.u92", 0x180000, 0x080000, CRC(c0b9494c) SHA1(f0b066dd78eb9fcf947da90ddb6c7b62299c5743) )


	ROM_REGION( 0x140000, "oki", 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008e.u46", 0x00000, 0x80000, CRC(d9379ba8) SHA1(5ae7c743319b1a12f2b101a9f0f8fe0728ed1476) )
	ROM_RELOAD(             0x40000, 0x80000 )
	ROM_LOAD( "pm007e.u47", 0xc0000, 0x80000, CRC(c7ed7950) SHA1(133258b058d3c562208d0d00b9fac71202647c32) )
ROM_END

ROM_START( galsnewa ) /* EXPRO-02 PCB */
	ROM_REGION( 0x40000, "main", 0 )
	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110e.u87-01",  0x000000, 0x20000, CRC(34e1ee0d) SHA1(567df65b04667a6d35725c4a131fb174acb3ad0a) ) /* Export region */
	ROM_LOAD16_BYTE( "pm109e.u88-01",  0x000001, 0x20000, CRC(c694255a) SHA1(16faf5ea5ff69a0e7a981021ea5fc09a0aefd7cf) )

	ROM_REGION16_BE( 0x300000, "user1", 0 )	/* 68000 data */
	ROM_LOAD16_BYTE( "pm004e.u86", 0x000001, 0x80000, CRC(d3af52bc) SHA1(46be057106388578defecab1cdd1793ec76ebe92) )
	ROM_LOAD16_BYTE( "pm005e.u85", 0x000000, 0x80000, CRC(d7ec650c) SHA1(6c2250c74381497154bf516e0cf1db6bb56bb446) )
	ROM_LOAD16_BYTE( "pm000e.u74", 0x100001, 0x80000, CRC(5d220f3f) SHA1(7ff373e01027c8832712f7a2d732f8e49b875878) )
	ROM_LOAD16_BYTE( "pm001e.u73", 0x100000, 0x80000, CRC(90433eb1) SHA1(8688a85747ad9ecac395d782f130baa64fb9d12b) )
	ROM_LOAD16_BYTE( "pm002e.u76", 0x200001, 0x80000, CRC(713ee898) SHA1(c9f608a57fb90e5ee15eb76a74a7afcc406d5b4e) )
	ROM_LOAD16_BYTE( "pm003e.u75", 0x200000, 0x80000, CRC(6bb060fd) SHA1(4fc3946866c5a55e8340b62b5ad9beae723ce0da) )

	ROM_REGION16_BE( 0x80000, "user2", 0 )	/* contains real (non-cartoon) women, used after each 3rd round */
	ROM_LOAD16_WORD_SWAP( "pm017e.u84", 0x00000, 0x80000, CRC(bc41b6ca) SHA1(0aeaf024dd7c84550e7df27230a1d4f04cc1d61c) )

	ROM_REGION( 0x200000, "gfx1", ROMREGION_ERASEFF )	/* sprites */
	/* the 06e rom from the other type gals panic board ends up split across 2 roms here */
	ROM_LOAD( "pm006e.u83", 0x000000, 0x080000, CRC(a7555d9a) SHA1(f95821b3358d9ab03ca9ead38fd358062259d89d) )
	ROM_LOAD( "pm206e.u82", 0x080000, 0x080000, CRC(cc978baa) SHA1(59a95bcbaeca9d356f61ea42af4da116afbb1491) )
	ROM_LOAD( "pm018e.u94", 0x100000, 0x080000, CRC(f542d708) SHA1(f515cca9e96401303ed45b4372f6079f29b7a999) )
	/* U93 is an empty socket and not used with this set */

	ROM_REGION( 0x200000, "gfx2", ROMREGION_DISPOSE )	/* sprites */

	ROM_REGION( 0x200000, "gfx3", ROMREGION_DISPOSE )	/* tiles - encrypted */
	ROM_LOAD( "pm013e.u89", 0x000000, 0x080000, CRC(10f27b05) SHA1(0f8ade713f6b430b5a23370a17326d53229951de) )
	ROM_LOAD( "pm014e.u90", 0x080000, 0x080000, CRC(2f367106) SHA1(1cd16e286e77e8e1b7668bbb6f2978101656b720) )
	ROM_LOAD( "pm015e.u91", 0x100000, 0x080000, CRC(a563f8ef) SHA1(6e4171746e4d401992bf3a7619d5bed0063d57e5) )
	ROM_LOAD( "pm016e.u92", 0x180000, 0x080000, CRC(c0b9494c) SHA1(f0b066dd78eb9fcf947da90ddb6c7b62299c5743) )

	ROM_REGION( 0x140000, "oki", 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008e.u46", 0x00000, 0x80000, CRC(d9379ba8) SHA1(5ae7c743319b1a12f2b101a9f0f8fe0728ed1476) )
	ROM_RELOAD(             0x40000, 0x80000 )
	ROM_LOAD( "pm007e.u47", 0xc0000, 0x80000, CRC(c7ed7950) SHA1(133258b058d3c562208d0d00b9fac71202647c32) )
ROM_END

ROM_START( galsnewj ) /* EXPRO-02 PCB */
	ROM_REGION( 0x40000, "main", 0 )
	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110j.u87", 0x000000, 0x20000, CRC(220b6df5) SHA1(d653b67bc66ca341bc660c2bb39b05dcf186fcb7) ) /* Japan region */
	ROM_LOAD16_BYTE( "pm109j.u88", 0x000001, 0x20000, CRC(17721444) SHA1(9d97fe1ddac99105798fc22375a0b89ab316459a) )

	ROM_REGION16_BE( 0x300000, "user1", 0 )	/* 68000 data */
	ROM_LOAD16_BYTE( "pm004e.u86", 0x000001, 0x80000, CRC(d3af52bc) SHA1(46be057106388578defecab1cdd1793ec76ebe92) )
	ROM_LOAD16_BYTE( "pm005e.u85", 0x000000, 0x80000, CRC(d7ec650c) SHA1(6c2250c74381497154bf516e0cf1db6bb56bb446) )
	ROM_LOAD16_BYTE( "pm000e.u74", 0x100001, 0x80000, CRC(5d220f3f) SHA1(7ff373e01027c8832712f7a2d732f8e49b875878) )
	ROM_LOAD16_BYTE( "pm001e.u73", 0x100000, 0x80000, CRC(90433eb1) SHA1(8688a85747ad9ecac395d782f130baa64fb9d12b) )
	ROM_LOAD16_BYTE( "pm002e.u76", 0x200001, 0x80000, CRC(713ee898) SHA1(c9f608a57fb90e5ee15eb76a74a7afcc406d5b4e) )
	ROM_LOAD16_BYTE( "pm003e.u75", 0x200000, 0x80000, CRC(6bb060fd) SHA1(4fc3946866c5a55e8340b62b5ad9beae723ce0da) )

	ROM_REGION16_BE( 0x80000, "user2", ROMREGION_ERASEFF )	/* contains real (non-cartoon) women, used after each 3rd round */
	/* U84 is an empty socket and not used with this set */

	ROM_REGION( 0x200000, "gfx1", ROMREGION_ERASEFF )	/* sprites */
	/* the 06e rom from the other type gals panic board ends up split across 2 roms here */
	ROM_LOAD( "pm006e.u83", 0x000000, 0x080000, CRC(a7555d9a) SHA1(f95821b3358d9ab03ca9ead38fd358062259d89d) )
	ROM_LOAD( "pm206e.u82", 0x080000, 0x080000, CRC(cc978baa) SHA1(59a95bcbaeca9d356f61ea42af4da116afbb1491) )
	ROM_LOAD( "pm018e.u94", 0x100000, 0x080000, CRC(f542d708) SHA1(f515cca9e96401303ed45b4372f6079f29b7a999) )
	/* U93 is an empty socket and not used with this set */

	ROM_REGION( 0x200000, "gfx2", ROMREGION_DISPOSE )	/* sprites */

	ROM_REGION( 0x200000, "gfx3", ROMREGION_DISPOSE )	/* tiles - encrypted */
	ROM_LOAD( "pm013e.u89", 0x000000, 0x080000, CRC(10f27b05) SHA1(0f8ade713f6b430b5a23370a17326d53229951de) )
	ROM_LOAD( "pm014e.u90", 0x080000, 0x080000, CRC(2f367106) SHA1(1cd16e286e77e8e1b7668bbb6f2978101656b720) )
	ROM_LOAD( "pm015e.u91", 0x100000, 0x080000, CRC(a563f8ef) SHA1(6e4171746e4d401992bf3a7619d5bed0063d57e5) )
	ROM_LOAD( "pm016e.u92", 0x180000, 0x080000, CRC(c0b9494c) SHA1(f0b066dd78eb9fcf947da90ddb6c7b62299c5743) )

	ROM_REGION( 0x140000, "oki", 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008j.u46", 0x00000, 0x80000, CRC(f394670e) SHA1(171f8dc519a13f352e6440aaadebe490c82361f0) )
	ROM_RELOAD(             0x40000, 0x80000 )
	ROM_LOAD( "pm007j.u47", 0xc0000, 0x80000, CRC(06780287) SHA1(8b9b57f6604b86d6dff42e5e51cd59a7111e1e79) )
ROM_END


GAME( 1990, galsnew,  0,       galsnew, galsnew,  galsnew, ROT90, "Kaneko", "Gals Panic (US, EXPRO-02 PCB)", 0 )
GAME( 1990, galsnewa, galsnew, galsnew, galsnewa, galsnew, ROT90, "Kaneko", "Gals Panic (Export, EXPRO-02 PCB)", 0 )
GAME( 1990, galsnewj, galsnew, galsnew, galsnewa, galsnew, ROT90, "Kaneko (Taito license)", "Gals Panic (Japan, EXPRO-02 PCB)", 0 )
