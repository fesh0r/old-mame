/******************************************************************

Volfied (c) 1989 Taito Corporation
==================================

  Original driver from RAINE

  68000 (8MHz?) + Z80 (4MHz?) + YM-2203 (4MHz?) + C-Chip

  VIDEO RAM: 12 * MB-81461 (256k VRAM)

  Known issues:

    - color information for the enemy sprites is missing (c-chip)
    - some bits in video RAM are not understood
    - sound seems a bit rough at times

********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

WRITE16_HANDLER( volfied_sprite_ctrl_w );
WRITE16_HANDLER( volfied_video_ram_w );
WRITE16_HANDLER( volfied_video_ctrl_w );
WRITE16_HANDLER( volfied_video_mask_w );
WRITE16_HANDLER( volfied_cchip_w );

READ16_HANDLER( volfied_video_ram_r );
READ16_HANDLER( volfied_video_ctrl_r );
READ16_HANDLER( volfied_cchip_r );

VIDEO_UPDATE( volfied );

VIDEO_START( volfied );

void volfied_cchip_init(void);


/***********************************************************
				MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( volfied_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },    /* program */
	{ 0x080000, 0x0fffff, MRA16_ROM },    /* tiles   */
	{ 0x100000, 0x103fff, MRA16_RAM },    /* main    */
	{ 0x200000, 0x203fff, PC090OJ_word_0_r },
	{ 0x400000, 0x47ffff, volfied_video_ram_r },
	{ 0x500000, 0x503fff, paletteram16_word_r },
	{ 0xd00000, 0xd00001, volfied_video_ctrl_r },
	{ 0xe00002, 0xe00003, taitosound_comm16_lsb_r },
	{ 0xf00000, 0xf00803, volfied_cchip_r },
MEMORY_END

static MEMORY_WRITE16_START( volfied_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },    /* program */
	{ 0x080000, 0x0fffff, MWA16_ROM },    /* tiles   */
	{ 0x100000, 0x103fff, MWA16_RAM },    /* main    */
	{ 0x200000, 0x203fff, PC090OJ_word_0_w },
	{ 0x400000, 0x47ffff, volfied_video_ram_w },
	{ 0x500000, 0x503fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x600000, 0x600001, volfied_video_mask_w },
	{ 0x700000, 0x700001, volfied_sprite_ctrl_w },
	{ 0xd00000, 0xd00001, volfied_video_ctrl_w },
	{ 0xe00000, 0xe00001, taitosound_port16_lsb_w },
	{ 0xe00002, 0xe00003, taitosound_comm16_lsb_w },
	{ 0xf00000, 0xf00c01, volfied_cchip_w },
MEMORY_END

static MEMORY_READ_START( z80_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, taitosound_slave_comm_r },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, taitosound_slave_port_w },
	{ 0x8801, 0x8801, taitosound_slave_comm_w },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0x9800, 0x9800, MWA_NOP },    /* ? */
MEMORY_END


/***********************************************************
				INPUT PORTS
***********************************************************/

#define VOLFIED_INPUT_BITS                                                          \
	PORT_START                                                                    \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )                                   \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )                                   \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )                                 \
	                                                                              \
	PORT_START                                                                    \
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )                        \
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )                        \
	                                                                              \
	PORT_START                                                                    \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )                                     \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )                \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )                \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )                \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )                \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )                                  \
	                                                                              \
	PORT_START                                                                    \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )


INPUT_PORTS_START( volfied )
	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "20k,40k,120k,480k,2400k" )
	PORT_DIPSETTING(    0x03, "50k,150k,600k,3000k" )
	PORT_DIPSETTING(    0x01, "70k,280k,1400k" )
	PORT_DIPSETTING(    0x00, "100k,500k" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

	VOLFIED_INPUT_BITS
INPUT_PORTS_END

INPUT_PORTS_START( volfiedu )
	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "20k,40k,120k,480k,2400k" )
	PORT_DIPSETTING(    0x03, "50k,150k,600k,3000k" )
	PORT_DIPSETTING(    0x01, "70k,280k,1400k" )
	PORT_DIPSETTING(    0x00, "100k,500k" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

	VOLFIED_INPUT_BITS
INPUT_PORTS_END

INPUT_PORTS_START( volfiedj )
	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "20k,40k,120k,480k,2400k" )
	PORT_DIPSETTING(    0x03, "50k,150k,600k,3000k" )
	PORT_DIPSETTING(    0x01, "70k,280k,1400k" )
	PORT_DIPSETTING(    0x00, "100k,500k" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

	VOLFIED_INPUT_BITS
INPUT_PORTS_END


/**************************************************************
				GFX DECODING
**************************************************************/

static struct GfxLayout tilelayout =
{
	16, 16,
	0x1800,
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 4096, 256 },
	{ -1 } /* end of array */
};


/**************************************************************
				YM2203 (SOUND)
**************************************************************/

/* handler called by the YM2203 emulator when the internal timers cause an IRQ */

static void irqhandler(int irq)
{
	cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,         /* 1 chip */
	4000000,   /* 4 MHz? */
	{ YM2203_VOL(60,15) },
	{ input_port_0_r },    /* DSW A */
	{ input_port_1_r },    /* DSW B */
	{ 0 },
	{ 0 },
	{ irqhandler }
};


/***********************************************************
				MACHINE DRIVERS
***********************************************************/

static DRIVER_INIT( volfied )
{
	volfied_cchip_init();
}

static MACHINE_DRIVER_START( volfied )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)   /* 8MHz? */
	MDRV_CPU_MEMORY(volfied_readmem,volfied_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)   /* sound CPU, required to run the game */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(20)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(volfied)
	MDRV_VIDEO_UPDATE(volfied)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( volfied )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 68000 code and tile data */
	ROM_LOAD16_BYTE( "c04-12-1.bin", 0x00000, 0x10000, 0xafb6a058 )
	ROM_LOAD16_BYTE( "c04-08-1.bin", 0x00001, 0x10000, 0x19f7e66b )
	ROM_LOAD16_BYTE( "c04-11-1.bin", 0x20000, 0x10000, 0x1aaf6e9b )
	ROM_LOAD16_BYTE( "volf-w.512",   0x20001, 0x10000, 0xb39e04f9 )
	ROM_LOAD16_BYTE( "c04-20.bin",   0x80000, 0x20000, 0x0aea651f )
	ROM_LOAD16_BYTE( "c04-22.bin",   0x80001, 0x20000, 0xf405d465 )
	ROM_LOAD16_BYTE( "c04-19.bin",   0xc0000, 0x20000, 0x231493ae )
	ROM_LOAD16_BYTE( "c04-21.bin",   0xc0001, 0x20000, 0x8598d38e )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites 16x16 */
	ROM_LOAD16_BYTE( "c04-16.bin",   0x00000, 0x20000, 0x8c2476ef )
	ROM_LOAD16_BYTE( "c04-18.bin",   0x00001, 0x20000, 0x7665212c )
	ROM_LOAD16_BYTE( "c04-15.bin",   0x40000, 0x20000, 0x7c50b978 )
	ROM_LOAD16_BYTE( "c04-17.bin",   0x40001, 0x20000, 0xc62fdeb8 )
	ROM_LOAD16_BYTE( "c04-10.bin",   0x80000, 0x10000, 0x429b6b49 )
	ROM_RELOAD     (                 0xa0000, 0x10000 )
	ROM_LOAD16_BYTE( "c04-09.bin",   0x80001, 0x10000, 0xc78cf057 )
	ROM_RELOAD     (                 0xa0001, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* sound cpu */
	ROM_LOAD( "c04-06.bin", 0x0000, 0x8000, 0xb70106b2 )
ROM_END

ROM_START( volfiedu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 68000 code and tile data */
	ROM_LOAD16_BYTE( "c04-12-1.bin", 0x00000, 0x10000, 0xafb6a058 )
	ROM_LOAD16_BYTE( "c04-08-1.bin", 0x00001, 0x10000, 0x19f7e66b )
	ROM_LOAD16_BYTE( "c04-11-1.bin", 0x20000, 0x10000, 0x1aaf6e9b )
	ROM_LOAD16_BYTE( "volf-usa.512", 0x20001, 0x10000, 0xc499346f )
	ROM_LOAD16_BYTE( "c04-20.bin",   0x80000, 0x20000, 0x0aea651f )
	ROM_LOAD16_BYTE( "c04-22.bin",   0x80001, 0x20000, 0xf405d465 )
	ROM_LOAD16_BYTE( "c04-19.bin",   0xc0000, 0x20000, 0x231493ae )
	ROM_LOAD16_BYTE( "c04-21.bin",   0xc0001, 0x20000, 0x8598d38e )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites 16x16 */
	ROM_LOAD16_BYTE( "c04-16.bin",   0x00000, 0x20000, 0x8c2476ef )
	ROM_LOAD16_BYTE( "c04-18.bin",   0x00001, 0x20000, 0x7665212c )
	ROM_LOAD16_BYTE( "c04-15.bin",   0x40000, 0x20000, 0x7c50b978 )
	ROM_LOAD16_BYTE( "c04-17.bin",   0x40001, 0x20000, 0xc62fdeb8 )
	ROM_LOAD16_BYTE( "c04-10.bin",   0x80000, 0x10000, 0x429b6b49 )
	ROM_RELOAD     (                 0xa0000, 0x10000 )
	ROM_LOAD16_BYTE( "c04-09.bin",   0x80001, 0x10000, 0xc78cf057 )
	ROM_RELOAD     (                 0xa0001, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* sound cpu */
	ROM_LOAD( "c04-06.bin", 0x0000, 0x8000, 0xb70106b2 )
ROM_END

ROM_START( volfiedj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 68000 code and tile data */
	ROM_LOAD16_BYTE( "c04-12-1.bin", 0x00000, 0x10000, 0xafb6a058 )
	ROM_LOAD16_BYTE( "c04-08-1.bin", 0x00001, 0x10000, 0x19f7e66b )
	ROM_LOAD16_BYTE( "c04-11-1.bin", 0x20000, 0x10000, 0x1aaf6e9b )
	ROM_LOAD16_BYTE( "c04-07-1.bin", 0x20001, 0x10000, 0x5d9065d5 )
	ROM_LOAD16_BYTE( "c04-20.bin",   0x80000, 0x20000, 0x0aea651f )
	ROM_LOAD16_BYTE( "c04-22.bin",   0x80001, 0x20000, 0xf405d465 )
	ROM_LOAD16_BYTE( "c04-19.bin",   0xc0000, 0x20000, 0x231493ae )
	ROM_LOAD16_BYTE( "c04-21.bin",   0xc0001, 0x20000, 0x8598d38e )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites 16x16 */
	ROM_LOAD16_BYTE( "c04-16.bin",   0x00000, 0x20000, 0x8c2476ef )
	ROM_LOAD16_BYTE( "c04-18.bin",   0x00001, 0x20000, 0x7665212c )
	ROM_LOAD16_BYTE( "c04-15.bin",   0x40000, 0x20000, 0x7c50b978 )
	ROM_LOAD16_BYTE( "c04-17.bin",   0x40001, 0x20000, 0xc62fdeb8 )
	ROM_LOAD16_BYTE( "c04-10.bin",   0x80000, 0x10000, 0x429b6b49 )
	ROM_RELOAD     (                 0xa0000, 0x10000 )
	ROM_LOAD16_BYTE( "c04-09.bin",   0x80001, 0x10000, 0xc78cf057 )
	ROM_RELOAD     (                 0xa0001, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* sound cpu */
	ROM_LOAD( "c04-06.bin", 0x0000, 0x8000, 0xb70106b2 )
ROM_END


GAMEX( 1989, volfied,  0,       volfied, volfied,  volfied, ROT270, "Taito Corporation Japan", "Volfied (World)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1989, volfiedu, volfied, volfied, volfiedu, volfied, ROT270, "Taito America Corporation", "Volfied (US)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1989, volfiedj, volfied, volfied, volfiedj, volfied, ROT270, "Taito Corporation", "Volfied (Japan)", GAME_UNEMULATED_PROTECTION )
