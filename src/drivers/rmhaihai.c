/***************************************************************************

Real Mahjong Haihai                (c)1985 Alba
Real Mahjong Haihai Jinji Idou Hen (c)1986 Alba
Real Mahjong Haihai Seichouhen     (c)1986 Visco

CPU:	Z80
Sound:	AY-3-8910
        M5205
OSC:	20.000MHz

driver by Nicola Salmoria

TODO:
- input handling is not well understood... it might well be handled by a
  protection device. I think it is, because rmhaijin and rmhaisei do additional
  checks which are obfuscated in a way that would make sense only for
  protection (in rmhaisei the failure is more explicit, in rmhaijin it's
  deviously delayed to a later part of the game).
  In themj the checks are patched out, maybe it's a bootleg?

- there probably is an area of NVRAM, because credits don't go away after
  a reset.

- some unknown reads and writes.

- visible area uncertain.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int gfxbank;

VIDEO_UPDATE( rmhaihai )
{
	int x,y;

	for (y = 0;y < 32;y++)
	{
		for (x = 0;x < 64;x++)
		{
			int code = videoram[64*y+x];
			int attr = colorram[64*y+x];

			drawgfx(bitmap,Machine->gfx[0],
					(gfxbank << 12) + code + ((attr & 0x07) << 8) + ((attr & 0x80) << 4),
					(gfxbank << 5) + (attr >> 3),	/* bit 7 used both for tile code and color */
					0,0,
					8*x,8*y,
					cliprect,TRANSPARENCY_NONE,0);
		}
	}
}



static int keyboard_cmd;

static READ_HANDLER( keyboard_r )
{
logerror("%04x: keyboard_r\n",activecpu_get_pc());
	switch(activecpu_get_pc())
	{
		/* read keyboard */
		case 0x0aba:	// rmhaihai, rmhaisei
		case 0x0b2a:	// rmhaihib
		case 0x0ab4:	// rmhaijin
		case 0x0aea:	// themj
		{
			int i;

			for (i = 0;i < 31;i++)
			{
				if (readinputport(2 + i/16) & (1<<(i&15))) return i+1;
			}
			if (readinputport(3) & 0x8000) return 0x80;	// coin
			return 0;
		}
		case 0x5c7b:	// rmhaihai, rmhaisei, rmhaijin
		case 0x5950:	// rmhaihib
		case 0x5bf3:	// themj, but the test is NOPed out!
			return 0xcc;	/* keyboard_cmd = 0xcb */


		case 0x13a:	// additional checks done by rmhaijin
			if (keyboard_cmd == 0x3b) return 0xdd;
			if (keyboard_cmd == 0x85) return 0xdc;
			if (keyboard_cmd == 0xf2) return 0xd6;
			if (keyboard_cmd == 0xc1) return 0x8f;
			if (keyboard_cmd == 0xd0) return 0x08;
			return 0;

		case 0x140:	// additional checks done by rmhaisei
		case 0x155:	// additional checks done by themj, but they are patched out!
			if (keyboard_cmd == 0x11) return 0x57;
			if (keyboard_cmd == 0x3e) return 0xda;
			if (keyboard_cmd == 0x48) return 0x74;
			if (keyboard_cmd == 0x5d) return 0x46;
			if (keyboard_cmd == 0xd0) return 0x08;
			return 0;
	}

	/* there are many more reads whose function is unknown, returning 0 seems fine */
	return 0;
}

static WRITE_HANDLER( keyboard_w )
{
logerror("%04x: keyboard_w %02x\n",activecpu_get_pc(),data);
	keyboard_cmd = data;
}

static READ_HANDLER( samples_r )
{
	return memory_region(REGION_SOUND1)[offset];
}

static WRITE_HANDLER( adpcm_w )
{
	MSM5205_data_w(0,data);         /* bit0..3  */
	MSM5205_reset_w(0,(data>>5)&1); /* bit 5    */
	MSM5205_vclk_w (0,(data>>4)&1); /* bit4     */
}

static WRITE_HANDLER( ctrl_w )
{
	gfxbank = (data & 0x40) >> 6;	/* rmhaisei only */

	coin_counter_w(0,data & 0x08);

	/* bit 2 used, meaning unknown */
}

static WRITE_HANDLER( themj_rombank_w )
{
	data8_t *rom = memory_region(REGION_CPU1) + 0x10000;
	int bank = data & 0x03;
logerror("banksw %d\n",bank);
	cpu_setbank(1, rom + bank*0x4000);
	cpu_setbank(2, rom + bank*0x4000 + 0x2000);
}

static MACHINE_INIT( themj )
{
	themj_rombank_w(0,0);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xa800, 0xb7ff, MRA_RAM },
	{ 0xc000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM },	/* rmhaisei only */
MEMORY_END

static MEMORY_READ_START( themj_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xa800, 0xb7ff, MRA_RAM },
	{ 0xc000, 0xdfff, MRA_BANK2 },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa7ff, MWA_RAM },
	{ 0xa800, 0xafff, MWA_RAM, &colorram },
	{ 0xb000, 0xb7ff, MWA_RAM, &videoram },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_ROM },	/* rmhaisei only */
MEMORY_END

static PORT_READ_START( readport )
	{ 0x0000, 0x7fff, samples_r },
	{ 0x8000, 0x8000, keyboard_r },
	{ 0x8020, 0x8020, AY8910_read_port_0_r },
MEMORY_END

static PORT_WRITE_START( writeport )
	{ 0x8001, 0x8001, keyboard_w },
	{ 0x8020, 0x8020, AY8910_control_port_0_w },
	{ 0x8021, 0x8021, AY8910_write_port_0_w },
	{ 0x8040, 0x8040, adpcm_w },
	{ 0x8060, 0x8060, ctrl_w },
MEMORY_END

static PORT_WRITE_START( themj_writeport )
	{ 0x8001, 0x8001, keyboard_w },
	{ 0x8020, 0x8020, AY8910_control_port_0_w },
	{ 0x8021, 0x8021, AY8910_write_port_0_w },
	{ 0x8040, 0x8040, adpcm_w },
	{ 0x8060, 0x8060, ctrl_w },
	{ 0x80a0, 0x80a0, themj_rombank_w },
MEMORY_END



INPUT_PORTS_START( rmhaihai )
	PORT_START  /* dsw2 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xfe, 0xfe, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xfe, "1 (Easy)" )
	PORT_DIPSETTING(    0x7e, "2" )
	PORT_DIPSETTING(    0xbe, "3" )
	PORT_DIPSETTING(    0xde, "4" )
	PORT_DIPSETTING(    0xee, "5" )
	PORT_DIPSETTING(    0xf6, "6" )
	PORT_DIPSETTING(    0xfa, "7" )
	PORT_DIPSETTING(    0xfc, "8 (Difficult)" )

    PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 2/1 B 1/2" )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, "A 1/2 B 2/1" )
	PORT_DIPSETTING(    0x08, "A 1/3 B 3/1" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Medal" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// fake, handled by keyboard_r()
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, 0, "Small",  KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, 0, "Double", KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, 0, "Big",    KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, 0, "Take",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, 0, "Flip",   KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, 0, "Last",   KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, 0, "K",      KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, 0, "Ron",    KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, 0, "G",      KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, 0, "Chi",    KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, 0, "C",      KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, 0, "L",      KEYCODE_L,     IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, 0, "H",      KEYCODE_H,     IP_JOY_NONE )

	PORT_START	// fake, handled by keyboard_r()
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, 0, "Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, 0, "D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, 0, "I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, 0, "Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, 0, "E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, 0, "M",     KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, 0, "A",     KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT_IMPULSE( 0x8000, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( rmhaihib )
	PORT_START  /* dsw2 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Gal Bonus Bet" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x18, 0x18, "Gal Bonus" )
	PORT_DIPSETTING(    0x18, "8" )
	PORT_DIPSETTING(    0x10, "16" )
	PORT_DIPSETTING(    0x08, "24" )
	PORT_DIPSETTING(    0x00, "32" )
	PORT_DIPNAME( 0xe0, 0xe0, "Pay Setting" )
	PORT_DIPSETTING(    0xe0, "90%" )
	PORT_DIPSETTING(    0xc0, "80%" )
	PORT_DIPSETTING(    0xa0, "70%" )
	PORT_DIPSETTING(    0x80, "60%" )
	PORT_DIPSETTING(    0x60, "50%" )
	PORT_DIPSETTING(    0x40, "40%" )
	PORT_DIPSETTING(    0x20, "30%" )
	PORT_DIPSETTING(    0x00, "20%" )

	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x03, 0x03, "Bet Max" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "10" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, "A 1/1 B 1/10" )
	PORT_DIPSETTING(    0x08, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 5/1" )
	PORT_DIPSETTING(    0x0c, "A 1/1 B 10/1" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// fake, handled by keyboard_r()
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, 0, "Small",  KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, 0, "Double", KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, 0, "Big",    KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, 0, "Take",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, 0, "Flip",   KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, 0, "Last",   KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, 0, "K",      KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, 0, "Ron",    KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, 0, "G",      KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, 0, "Chi",    KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, 0, "C",      KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, 0, "L",      KEYCODE_L,     IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, 0, "H",      KEYCODE_H,     IP_JOY_NONE )

	PORT_START	// fake, handled by keyboard_r()
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, 0, "Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, 0, "D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, 0, "I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, 0, "Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, 0, "E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, 0, "M",     KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, 0, "A",     KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT_IMPULSE( 0x8000, IP_ACTIVE_HIGH, IPT_COIN1, 1 )

//	PORT_START // 11
//	PORT_BITX(    0x01, IP_ACTIVE_LOW, 0, "Pay Out", KEYCODE_3, IP_JOY_NONE )
//	PORT_BIT(     0x02, IP_ACTIVE_LOW, IPT_SERVICE4 ) /* RAM clear */
//	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
//	PORT_BIT(     0x08, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Analyzer */
//	PORT_BIT(     0xF0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	3,
	{ RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo1[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo2[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	20000000/16,	/* 1.25 MHz ??? */
	{ 30 },
	{ input_port_0_r },
	{ input_port_1_r },
	{ 0 },
	{ 0 }
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	500000,				/* 500KHz ?? (I don't know what I'm doing, really) */
	{ 0 },				/* interrupt function */
	{ MSM5205_SEX_4B },	/* vclk input mode    */
	{ MIXERG(100,MIXER_GAIN_2x,MIXER_PAN_CENTER) }
};



static MACHINE_DRIVER_START( rmhaihai )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,20000000/4)	/* 5 MHz ??? */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(4*8, 60*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_UPDATE(rmhaihai)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( rmhaisei )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(rmhaihai)

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(0x200)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( themj )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(rmhaihai)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(themj_readmem,writemem)
	MDRV_CPU_PORTS(readport,themj_writeport)

	MDRV_MACHINE_INIT(themj)

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(0x200)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rmhaihai )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "s3-6.11g",     0x00000, 0x2000, 0xe7af7ba2 )
	ROM_CONTINUE(             0x06000, 0x2000 )
	ROM_LOAD( "s3-4.8g",      0x04000, 0x2000, 0xf849e75c )
	ROM_CONTINUE(             0x02000, 0x2000 )
	ROM_LOAD( "s3-2.6g",      0x08000, 0x2000, 0xd614532b )
	ROM_CONTINUE(             0x0c000, 0x2000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s0-10.8a",     0x00000, 0x4000, 0x797c63d1 )
	ROM_LOAD( "s0-9.7a",      0x04000, 0x4000, 0xb2526747 )
	ROM_LOAD( "s0-8.6a",      0x08000, 0x4000, 0x146eaa31 )
	ROM_LOAD( "s1-7.5a",      0x0c000, 0x4000, 0xbe59e742 )
	ROM_LOAD( "s0-12.11a",    0x10000, 0x4000, 0xe4229389 )
	ROM_LOAD( "s1-11.10a",    0x14000, 0x4000, 0x029ef909 )
	/* 0x18000-0x1ffff empty space filled by the init function */

	ROM_REGION( 0x0300, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "s2.13b",       0x0000, 0x0100, 0x911d32a5 )
	ROM_LOAD( "s1.13a",       0x0100, 0x0100, 0xe9be978a )
	ROM_LOAD( "s3.13c",       0x0200, 0x0100, 0x609775a6 )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples, read directly by the main CPU */
	ROM_LOAD( "s0-1.5g",      0x00000, 0x8000, 0x65e55b7e )
ROM_END

ROM_START( rmhaihib )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "s-30-6.11g",   0x00000,  0x2000, 0xf3e13cc8 )
	ROM_CONTINUE(             0x06000,  0x2000 )
	ROM_LOAD( "s-30-4.8g",    0x04000,  0x2000, 0xf6642584 )
	ROM_CONTINUE(             0x02000,  0x2000 )
	ROM_LOAD( "s-30-2.6g",    0x08000,  0x2000, 0xe5959703 )
	ROM_CONTINUE(             0x0c000,  0x2000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s0-10.8a",     0x00000, 0x4000, 0x797c63d1 )
	ROM_LOAD( "s0-9.7a",      0x04000, 0x4000, 0xb2526747 )
	ROM_LOAD( "s0-8.6a",      0x08000, 0x4000, 0x146eaa31 )
	ROM_LOAD( "s1-7.5a",      0x0c000, 0x4000, 0xbe59e742 )
	ROM_LOAD( "s0-12.11a",    0x10000, 0x4000, 0xe4229389 )
	ROM_LOAD( "s1-11.10a",    0x14000, 0x4000, 0x029ef909 )
	/* 0x18000-0x1ffff empty space filled by the init function */

	ROM_REGION( 0x0300, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "s2.13b",       0x0000, 0x0100, 0x911d32a5 )
	ROM_LOAD( "s1.13a",       0x0100, 0x0100, 0xe9be978a )
	ROM_LOAD( "s3.13c",       0x0200, 0x0100, 0x609775a6 )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples, read directly by the main CPU */
	ROM_LOAD( "s0-1.5g",      0x00000, 0x8000, 0x65e55b7e )
ROM_END

ROM_START( rmhaijin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "s-4-6.11g",    0x00000, 0x2000, 0x474c9ace )
	ROM_CONTINUE(             0x06000, 0x2000 )
	ROM_LOAD( "s-4-4.8g",     0x04000, 0x2000, 0xc76ab584 )
	ROM_CONTINUE(             0x02000, 0x2000 )
	ROM_LOAD( "s-4-2.6g",     0x08000, 0x2000, 0x77b16f5b )
	ROM_CONTINUE(             0x0c000, 0x2000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s-1-10.8a",    0x00000, 0x4000, 0x797c63d1 )
	ROM_LOAD( "s-1-9.7a",     0x04000, 0x4000, 0x5d3793d4 )
	ROM_LOAD( "s-1-8.6a",     0x08000, 0x4000, 0x6fcd990b )
	ROM_LOAD( "s-2-7.5a",     0x0c000, 0x4000, 0xe92658bd )
	ROM_LOAD( "s-1-12.11a",   0x10000, 0x4000, 0x7502a191 )
	ROM_LOAD( "s-2-11.10a",   0x14000, 0x4000, 0x9ebbc607 )
	/* 0x18000-0x1ffff empty space filled by the init function */

	ROM_REGION( 0x0300, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "s5.13b",       0x0000, 0x0100, 0x153aa7bf )
	ROM_LOAD( "s4.13a",       0x0100, 0x0100, 0x5d643e6e )
	ROM_LOAD( "s6.13c",       0x0200, 0x0100, 0xfd6ff344 )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples, read directly by the main CPU */
	ROM_LOAD( "s-0-1.5g",     0x00000, 0x8000, 0x65e55b7e )
ROM_END

ROM_START( rmhaisei )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sei-11.h11",   0x00000, 0x2000, 0x7c35692b )
	ROM_CONTINUE(             0x06000, 0x2000 )
	ROM_LOAD( "sei-10.h8",    0x04000, 0x2000, 0xcbd58124 )
	ROM_CONTINUE(             0x02000, 0x2000 )
	ROM_LOAD( "sei-8.h6",     0x08000, 0x2000, 0x8c8dc2fd )
	ROM_CONTINUE(             0x0c000, 0x2000 )
	ROM_LOAD( "sei-9.h7",     0x0e000, 0x2000, 0x9132368d )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sei-4.a8",     0x00000, 0x8000, 0x6a0234bf )
	ROM_LOAD( "sei-3.a7",     0x08000, 0x8000, 0xc48bc39f )
	ROM_LOAD( "sei-2.a6",     0x10000, 0x8000, 0xe479ba47 )
	ROM_LOAD( "sei-1.a5",     0x18000, 0x8000, 0xfe6555f8 )
	ROM_LOAD( "sei-6.a11",    0x20000, 0x8000, 0x86f1b462 )
	ROM_LOAD( "sei-5.a9",     0x28000, 0x8000, 0x8bf780bc )
	/* 0x30000-0x3ffff empty space filled by the init function */

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bpr",        0x0000, 0x0200, 0x9ad2afcd )
	ROM_LOAD( "1.bpr",        0x0200, 0x0200, 0x9b036f82 )
	ROM_LOAD( "3.bpr",        0x0400, 0x0200, 0x0fa1a50a )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples, read directly by the main CPU */
	ROM_LOAD( "sei-7.h5",     0x00000, 0x8000, 0x3e412c1a )
ROM_END

ROM_START( themj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* CPU */
	ROM_LOAD( "t7.bin",       0x00000,  0x02000, 0xa58563c3 )
	ROM_CONTINUE(             0x06000,  0x02000 )
	ROM_LOAD( "t8.bin",       0x04000,  0x02000, 0xbdf29475 )
	ROM_CONTINUE(             0x02000,  0x02000 )
	ROM_LOAD( "t9.bin",       0x0e000,  0x02000, 0xd5537d03 )
	ROM_LOAD( "no1.bin",      0x10000,  0x10000, 0xa67dd977 ) /* banked */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE ) /* gfx */
	ROM_LOAD( "t3.bin",       0x00000,  0x8000, 0xf0735c62 )
	ROM_LOAD( "t4.bin",       0x08000,  0x8000, 0x952227fa )
	ROM_LOAD( "t5.bin",       0x10000,  0x8000, 0x3deea9b4 )
	ROM_LOAD( "t6.bin",       0x18000,  0x8000, 0x47717958 )
	ROM_LOAD( "t1.bin",       0x20000,  0x8000, 0x9b9a458e )
	ROM_LOAD( "t2.bin",       0x28000,  0x8000, 0x4702375f )
	/* 0x30000-0x3ffff empty space filled by the init function */

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "5.bin",        0x0000,  0x0200, 0x062fb055 )
	ROM_LOAD( "4.bin",        0x0200,  0x0200, 0x9f81a6d7 )
	ROM_LOAD( "6.bin",        0x0400,  0x0200, 0x61373ec7 )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples, read directly by the main CPU */
	ROM_LOAD( "t0.bin",       0x00000,  0x8000, 0x3e412c1a )
ROM_END


static DRIVER_INIT( rmhaihai )
{
	data8_t *rom = memory_region(REGION_GFX1);
	int size = memory_region_length(REGION_GFX1);
	int a,b;

	size /= 2;
	rom += size;

	/* unpack the high bit of gfx */
	for (b = size - 0x4000;b >= 0;b -= 0x4000)
	{
		if (b) memcpy(rom + b,rom + b/2,0x2000);

		for (a = 0;a < 0x2000;a++)
		{
			rom[a + b + 0x2000] = rom[a + b] >> 4;
		}
	}
}


GAMEX( 1985, rmhaihai, 0,        rmhaihai, rmhaihai, rmhaihai, ROT0, "Alba",  "Real Mahjong Haihai (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1985, rmhaihib, rmhaihai, rmhaihai, rmhaihib, rmhaihai, ROT0, "Alba",  "Real Mahjong Haihai [BET] (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1986, rmhaijin, 0,        rmhaihai, rmhaihai, rmhaihai, ROT0, "Alba",  "Real Mahjong Haihai Jinji Idou Hen (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1986, rmhaisei, 0,        rmhaisei, rmhaihai, rmhaihai, ROT0, "Visco", "Real Mahjong Haihai Seichouhen (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1987, themj,    0,        themj,    rmhaihai, rmhaihai, ROT0, "Visco", "The Mah-jong (Japan)", GAME_NO_COCKTAIL )
