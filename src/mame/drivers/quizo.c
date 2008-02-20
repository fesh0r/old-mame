/*********************************************
 Quiz Olympic (c)1985 Seoul Coin Corp.

 driver by Tomasz Slanina

 ROMs contains strings:

    QUIZ OLYMPIC Ver 1.0
    PROGRAMMED BY  K.ISHIDA
    AT 1984.10.26
    TAITO CORP.
    KUMAGAYA-TSC


--

Z80 @ 4.0MHz [8/2]
AY-3-8910 @ 1.3423MHz [21.47727/16]
1x 2016 RAM
4x 4416 RAM
Xtals 8MHz, 21.47727MHz
1x 82S123 PROM

**********************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"

#define XTAL1	 8000000
#define XTAL2	21477270


static UINT8 port60;
static UINT8 port70;

static const UINT8 rombankLookup[]={ 2, 3, 4, 4, 4, 4, 4, 5, 0, 1};

static PALETTE_INIT(quizo)
{
	int i;
	for (i = 0;i < 16;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		bit0 = 0;
		bit1 = (*color_prom >> 0) & 0x01;
		bit2 = (*color_prom >> 1) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (*color_prom >> 2) & 0x01;
		bit1 = (*color_prom >> 3) & 0x01;
		bit2 = (*color_prom >> 4) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (*color_prom >> 5) & 0x01;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(machine,i,MAKE_RGB(r,g,b));
		color_prom++;
	}
}

static VIDEO_UPDATE( quizo )
{
	int x,y;
	for(y=0;y<200;y++)
	{
		for(x=0;x<80;x++)
		{
			int data=videoram[y*80+x];
			int data1=videoram[y*80+x+0x4000];
			int pix;

			pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
			*BITMAP_ADDR16(bitmap, y, x*4+3) = pix;
			data>>=1;
			data1>>=1;
			pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
			*BITMAP_ADDR16(bitmap, y, x*4+2) = pix;
			data>>=1;
			data1>>=1;
			pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
			*BITMAP_ADDR16(bitmap, y, x*4+1) = pix;
			data>>=1;
			data1>>=1;
			pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
			*BITMAP_ADDR16(bitmap, y, x*4+0) = pix;
		}
	}
	return 0;
}

static WRITE8_HANDLER(vram_w)
{
	int bank=(port70&8)?1:0;
	videoram[offset+bank*0x4000]=data;
}

static WRITE8_HANDLER(port70_w)
{
	port70=data;
}

static WRITE8_HANDLER(port60_w)
{
	if(data>9)
	{
		logerror("ROMBANK %x @ %x\n", data, activecpu_get_pc());
		data=0;
	}
	port60=data;
	memory_set_bankptr( 1, &memory_region(REGION_USER1)[rombankLookup[data]*0x4000] );
}

static ADDRESS_MAP_START( memmap, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(vram_w)

ADDRESS_MAP_END

static ADDRESS_MAP_START( portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x10, 0x10) AM_READ(input_port_1_r)
	AM_RANGE(0x40, 0x40) AM_READ(input_port_2_r)
	AM_RANGE(0x50, 0x50) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x51, 0x51) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x60, 0x60) AM_WRITE(port60_w)
	AM_RANGE(0x70, 0x70) AM_WRITE(port70_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( quizo )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )

	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Test Mode" ) // test mode + timer freeze during game
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( quizo )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,XTAL1/2)
	MDRV_CPU_PROGRAM_MAP(memmap, 0)
	MDRV_CPU_IO_MAP(portmap, 0)

	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 320-1, 0*8, 200-1)

	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(quizo)

	MDRV_VIDEO_UPDATE(quizo)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, XTAL2/16 )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( quizo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom1",   0x0000, 0x4000, CRC(6731735f) SHA1(7dbf48f833c7b7cde77df2a10781e5a8b6ae0533) )
	ROM_CONTINUE(             0x0000, 0x04000 )

	ROM_REGION( 0x18000, REGION_USER1, 0 )
	ROM_LOAD( "rom2",   0x00000, 0x8000, CRC(a700eb30) SHA1(7800b3d2b7992c67c91cfb7e02c7cfc313b0ed5d) )
	ROM_LOAD( "rom3",   0x08000, 0x8000, CRC(d344f97e) SHA1(3d669a56f084f2a7a50d7d211b84a50d35de66ac) )
	ROM_LOAD( "rom4",   0x10000, 0x8000, CRC(ab1eb174) SHA1(7d7a935aa7196a814c15f13444b88e770678b672) )

	ROM_REGION( 0x0020,  REGION_PROMS, 0 )
	ROM_LOAD( "82s123",   0x0000, 0x0020, CRC(c3f15914) SHA1(19fd8e6f2a1256ae51c500a3bf1d7358810ef97e) )
ROM_END

static DRIVER_INIT(quizo)
{
	videoram=auto_malloc(0x4000*2);
}

GAME( 1985, quizo,  0,       quizo,  quizo,  quizo, ROT0, "Seoul Coin Corp.", "Quiz Olympic", 0 )
