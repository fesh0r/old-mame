/***************************************************************************

Sega Genesis/MegaDrive Memory Map (preliminary)

Main CPU (68k)

$000000 - $3fffff : Cartridge ROM
$400000 - $9fffff : Reserved by Sega (unused?)
$a00000 - $a0ffff : shared RAM w/Z80
$a10000 - $a10fff : IO
$a11000 - $a11fff : Controls
	$a11000 : memory mode (16-bit)
	$a11100 : Z80 bus request (16)
	$a11200 : Z80 reset (16)
$c00000 - $dfffff : Video Display Processor (VDP)
	$c00000 : data (16-bit, mirrored)
	$c00004 : control (16, m)
	$c00008 : hv counter (16, m)
	$c00011 : psg 76489 (8)
$ff0000 - $ffffff : RAM

Interrupts:
	level 6 - vertical
	level 4 - horizontal
	level 2 - external

Sound CPU (Z80)

$0000 - $1fff : RAM
$2000 - $3fff : Reserved (RAM?)
$4001 - $4003 : ym2612 registers
$6000         : bank register
$7f11         : psg 76489
$8000 - $ffff : shared RAM w/68k


Gareth S. Long

In Memory Of Haruki Ikeda

***************************************************************************/
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#include "driver.h"
#include "vidhrdw/generic.h"

#include "machine/genesis.h"
#include "vidhrdw/genesis.h"
#include "sound/2612intf.h"

#define ARMLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))

int deb = 0;


unsigned int	z80_68000_latch			= 0;
unsigned int	z80_latch_bitcount		= 0;

#define EASPORTS_HACK
UINT16 cartridge_ram[0x10000]; /* any cartridge RAM */


#if LSB_FIRST
	#define BYTE_XOR(a) ((a) ^ 1)
#else
	#define BYTE_XOR(a) (a)
#endif


static READ_HANDLER ( genesis_vdp_76489_r )
{
	return 0;
}
static WRITE16_HANDLER (genesis_vdp_76489_w )
{
	SN76496_0_w(0, data);
}

static WRITE_HANDLER ( genesis_ramlatch_w ) /* note value will be meaningless unless all bits are correctly set in */
{
	if (offset !=0 ) return;
	if (!z80running) logerror("undead Z80 latch write!\n");
//	cpu_halt(0,0);
	if (z80_latch_bitcount == 0) z80_68000_latch = 0;
/*	logerror("latch update\n");*/
	z80_68000_latch = z80_68000_latch | ((( ((unsigned char)data) & 0x01) << (15+z80_latch_bitcount)));
 	logerror("value %x written to latch\n", data);
	z80_latch_bitcount++;
	if (z80_latch_bitcount == 9)
	{
	//	cpu_halt(0,1);
		z80_latch_bitcount = 0;
		logerror("latch set, value %x\n", z80_68000_latch);
	}
}

static WRITE16_HANDLER ( genesis_ramlatch_68000_w )
{
	genesis_ramlatch_w(offset, data);
}

static WRITE_HANDLER ( genesis_s_68000_ram_w )
{
	unsigned int address = (z80_68000_latch) + (offset & 0x7fff);
	if (!z80running) logerror("undead Z80->68000 write!\n");
	if (z80_latch_bitcount != 0) logerror("writing whilst latch being set!\n");

	if (address > 0xff0000) genesis_sharedram[BYTE_XOR(offset)] = data;
	logerror("z80 poke to address %x\n", address);
}

static READ_HANDLER ( genesis_s_68000_ram_r )
{
	int address = (z80_68000_latch) + (offset & 0x7fff);

	if (!z80running) logerror("undead Z80->68000 read!\n");

	if (z80_latch_bitcount != 0) logerror("reading whilst latch being set!\n");

	logerror("z80 read from address %x\n", address);

	/* Read the data out of the 68k ROM */
	if (address < 0x400000) return memory_region(REGION_CPU1)[BYTE_XOR(address)];
	/* else read the data out of the 68k RAM */
 	else if (address > 0xff0000) return genesis_sharedram[BYTE_XOR(offset)];

	return -1;
}

static WRITE16_HANDLER ( genesis_soundram_w )
{
	if (z80running) logerror("Z80 written whilst running!\n");
	logerror("68000->z80 sound write, %x to %x\n", data, offset);

	if (ACCESSING_LSB) genesis_soundram[(offset<<1)+1] = data & 0xff;
	if (ACCESSING_MSB) genesis_soundram[offset<<1] = (data >> 8) & 0xff;
}

static READ16_HANDLER ( genesis_soundram_r )
{
	if (z80running) logerror("Z80 read whilst running!\n");
	logerror("soundram_r returning %x\n",(genesis_soundram[offset<<1] << 8) + genesis_soundram[(offset<<1)+1]);
	return (genesis_soundram[offset<<1] << 8) + genesis_soundram[(offset<<1)+1];
}

/*static void genesis_sharedram_w (int offset,int data)
{
 	COMBINE_WORD_MEM(&genesis_sharedram[offset], data);
}

static int genesis_sharedram_r (int offset)
{
	return READ_WORD(&genesis_sharedram[offset]);
}*/


#ifdef EASPORTS_HACK
static READ16_HANDLER(cartridge_ram_r)
{
	logerror("cartridge ram read.. %x\n", offset);
	return cartridge_ram[(offset&0xffff)>>1];
}
static WRITE16_HANDLER(cartridge_ram_w)
{
	logerror("cartridge ram write.. %x to %x\n", data, offset);
	cartridge_ram[offset] = data;
}
#endif

static MEMORY_READ16_START(genesis_readmem)
	// { 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0xff0000, 0xffffff, MRA16_BANK2}, /* RAM */
	{ 0xc00014, 0xfeffff, MRA16_NOP },

 	{ 0xc00000, 0xc00003, genesis_vdp_data_r },
	{ 0xc00004, 0xc00007, genesis_vdp_ctrl_r },
	{ 0xc00008, 0xc0000b, genesis_vdp_hv_r },
	{ 0xc00010, 0xc00013, MRA16_NOP /*would be genesis_vdp_76489_r*/ },
	{ 0xa11204, 0xbfffff, MRA16_NOP },

	{ 0xa11000, 0xa11203, genesis_ctrl_r },
	{ 0xa10000, 0xa1001f, genesis_io_r },
	{ 0xa00000, 0xa01fff, genesis_soundram_r },
	{ 0xa04000, 0xa04003, YM2612_68000_r },

#ifdef EASPORTS_HACK
	{ 0x200000, 0x20ffff, MRA16_ROM},
#endif
MEMORY_END


static MEMORY_WRITE16_START(genesis_writemem)
	{ 0xff0000, 0xffffff, MWA16_BANK2, /*genesis_sharedram_w*/ },
 	{ 0xd00000, 0xd03fff, genesis_videoram1_w, (data16_t**)&videoram, &videoram_size }, /*this is just a fake */
	{ 0xc00014, 0xcfffff, MWA16_NOP },
	{ 0xc00000, 0xc00003, genesis_vdp_data_w },
	{ 0xc00004, 0xc00007, genesis_vdp_ctrl_w },
	{ 0xc00008, 0xc0000b, genesis_vdp_hv_w },
 	{ 0xc00010, 0xc00013, genesis_vdp_76489_w },
	{ 0xa11204, 0xbfffff, MWA16_NOP },
	{ 0xa11000, 0xa11203, genesis_ctrl_w },
	{ 0xa10000, 0xa1001f, genesis_io_w},
	{ 0xa07f10, 0xa07f13, genesis_vdp_76489_w },
	{ 0xa06000, 0xa06003, genesis_ramlatch_68000_w },
	{ 0xa00000, 0xa01fff, genesis_soundram_w },
	{ 0xa04000, 0xa04003, YM2612_68000_w },
#ifdef EASPORTS_HACK
	{ 0x200000, 0x20ffff, MWA16_RAM /*cartridge_ram_w*/ },
	{ 0x000000, 0x1fffff, MWA16_ROM },
#endif
#ifndef EASPORT_HACK
	// { 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x000000, 0x1fffff, MWA16_ROM },
#endif
MEMORY_END


static MEMORY_READ_START(genesis_s_readmem)
 	{ 0x0000, 0x1fff, MRA_BANK1, /*&genesis_soundram*//*genesis_soundram_r*/ },
	{ 0x4000, 0x4000, YM2612_status_port_0_A_r },
	{ 0x4001, 0x4001, YM2612_read_port_0_r },
	{ 0x4002, 0x4002, YM2612_status_port_0_B_r },
//	{ 0x4003, 0x4003, YM2612_3_r },
	{ 0x8000, 0xffff, genesis_s_68000_ram_r },
	{ 0x7f11, 0x7f11, genesis_vdp_76489_r },
MEMORY_END

static MEMORY_WRITE_START(genesis_s_writemem)
	{ 0x0000, 0x1fff, MWA_BANK1 /*genesis_soundram_w*/ },
	{ 0x4000, 0x4000, YM2612_control_port_0_A_w },
	{ 0x4001, 0x4001, YM2612_data_port_0_A_w },
	{ 0x4002, 0x4002, YM2612_control_port_0_B_w },
	{ 0x4003, 0x4003, YM2612_data_port_0_B_w },
	{ 0x6000, 0x6000, genesis_ramlatch_w },
 	{ 0x7f11, 0x7f11, SN76496_0_w },
 	{ 0x8000, 0xffff, genesis_s_68000_ram_w },
MEMORY_END

static PORT_WRITE_START(writeport)
	{ 0x7f, 0x7f, SN76496_0_w },
PORT_END


INPUT_PORTS_START( genesis )
	PORT_START	/* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3   | IPF_PLAYER1, "P1 Button C" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2   | IPF_PLAYER1, "P1 Button B" )
	PORT_START	/* IN1 player 1 controller, part 2 */
	PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1   | IPF_PLAYER1, "P1 Button A" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_LOW, IPT_START1    | IPF_PLAYER1, "P1 Start" )

	PORT_START	/* IN2 player 2 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3   | IPF_PLAYER2, "P2 Button C" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2   | IPF_PLAYER2, "P2 Button B" )
	PORT_START	/* IN3 player 2 controller, part 2 */
	PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1   | IPF_PLAYER2, "P2 Button A" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_LOW, IPT_START1    | IPF_PLAYER2, "P2 Start" )

	PORT_START	/* IN4 - internal switches, and fake 'Auto' */
	PORT_DIPNAME( 0x03, 0x00, "Country")
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPSETTING(    0x01, "USA" )
	PORT_DIPSETTING(    0x02, "Japan" )
	PORT_DIPSETTING(    0x03, "Europe" )
INPUT_PORTS_END


/* Genesis doesn't have a color PROM, it uses VDP 'CRAM' to generate colours */
/* and change them during the game. */

static struct SN76496interface sn76496_interface =
{
	1,	/* 1 chip */
	{53693100/10.40},
	{ 255*2, 255*2 }
};

static struct YM2612interface ym2612_interface =
{
	1,	/* 1 chip */
	53693100 / 7,
	{ 0x7fffffff,0x7fffffff },
	{ 0 },
};

static MACHINE_DRIVER_START( genesis )
	/*basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 53693100 / 7) /* 8Mhz..ish */
	MDRV_CPU_MEMORY(genesis_readmem, genesis_writemem)
	MDRV_CPU_VBLANK_INT(genesis_interrupt, 1) /* upto 224 int's per frame */

	MDRV_CPU_ADD_TAG("sound", Z80, 53693100 / 15) /* 4Mhz..ish */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(genesis_s_readmem, genesis_s_writemem)
	MDRV_CPU_PORTS(0, writeport)
	MDRV_CPU_VBLANK_INT(genesis_s_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10) /* 80 CPU slices per frame */

	MDRV_MACHINE_INIT(genesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER /*| VIDEO_MODIFIES_PALETTE*/)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(0)
	MDRV_PALETTE_LENGTH(64) /* 4 color schemes of 16 colors each, 0 of each bank is transparent*/
	MDRV_COLORTABLE_LENGTH(64 / sizeof(unsigned short))

	MDRV_PALETTE_INIT(genesis)
	MDRV_VIDEO_START(genesis)
	MDRV_VIDEO_STOP(genesis)
	MDRV_VIDEO_UPDATE(genesis)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2612, ym2612_interface)
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

ROM_START(genesis)
	ROM_REGION(0x415000,REGION_CPU1,0)
ROM_END

SYSTEM_CONFIG_START(genesis)
	CONFIG_DEVICE_CARTSLOT( 1, "smd\0bin\0md\0", genesis_init_cart, NULL, genesis_partialcrc)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	CONFIG		COMPANY	FULLNAME */
CONS( 1988, genesis,  0,		genesis,  genesis,	0,		genesis,	"Sega",   "Megadrive / Genesis" )

