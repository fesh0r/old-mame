/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "driver.h"

#include "cpu/f8/f8.h"
#include "cpu/f8/f3853.h"
#include "includes/mk1.h"

/*
chess champion mk i


Signetics 7916E C48091 82S210-1 COPYRIGHT
2 KB rom? 2716 kompatible?

4 11 segment displays (2. point up left, verticals in the mid )

2 x 2111 256x4 SRAM

MOSTEK MK 3853N 7915 Philippines ( static memory interface for f8)

MOSTEK MK 3850N-3 7917 Philipines (fairchild f8 cpu)

16 keys (4x4 matrix)

switch s/l (dark, light) pure hardware?
(power on switch)

speaker?

 */

static UINT8 mk1_f8[2];

READ_HANDLER(mk1_f8_r)
{
    UINT8 data=mk1_f8[offset];
//    logerror ("f8 %.6f r %x %x\n", timer_get_time(), offset, data);
    if (offset==0) {
	if (data&1) data|=readinputport(1);
	if (data&2) data|=readinputport(2);
	if (data&4) data|=readinputport(3);
	if (data&8) data|=readinputport(4);
	if (data&0x10) {
	    if (readinputport(1)&0x10) data|=1;
	    if (readinputport(2)&0x10) data|=2;
	    if (readinputport(3)&0x10) data|=4;
	    if (readinputport(4)&0x10) data|=8;
	}
	if (data&0x20) {
	    if (readinputport(1)&0x20) data|=1;
	    if (readinputport(2)&0x20) data|=2;
	    if (readinputport(3)&0x20) data|=4;
	    if (readinputport(4)&0x20) data|=8;
	}
	if (data&0x40) {
	    if (readinputport(1)&0x40) data|=1;
	    if (readinputport(2)&0x40) data|=2;
	    if (readinputport(3)&0x40) data|=4;
	    if (readinputport(4)&0x40) data|=8;
	}
	if (data&0x80) {
	    if (readinputport(1)&0x80) data|=1;
	    if (readinputport(2)&0x80) data|=2;
	    if (readinputport(3)&0x80) data|=4;
	    if (readinputport(4)&0x80) data|=8;
	}
    } else {
    }

    return data;
}

WRITE_HANDLER(mk1_f8_w)
{
/* 0 is high and allows also input */
    mk1_f8[offset]=data;
//    logerror("f8 %.6f w %x %x\n", timer_get_time(), offset, data);
    if (offset==0) {	
    } else {
    }
    if (!(mk1_f8[1]&1)) mk1_led[0]=mk1_f8[0];
    if (!(mk1_f8[1]&2)) mk1_led[1]=mk1_f8[0];
    if (!(mk1_f8[1]&4)) mk1_led[2]=mk1_f8[0];
    if (!(mk1_f8[1]&8)) mk1_led[3]=mk1_f8[0];
}

static MEMORY_READ_START( mk1_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x1800, 0x18ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( mk1_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x1800, 0x18ff, MWA_RAM },
MEMORY_END


static PORT_READ_START( mk1_readport )
{ 0x0, 0x1, mk1_f8_r },
{ 0xc, 0xf, f3853_r },
PORT_END

static PORT_WRITE_START( mk1_writeport )
{ 0x0, 0x1, mk1_f8_w },
{ 0xc, 0xf, f3853_w },
PORT_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( mk1 )
	PORT_START
	PORT_DIPNAME ( 0x01, 0x01, "Switch")
	PORT_DIPSETTING(  0, "L" )
	PORT_DIPSETTING(  1, "S" )
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "White A    King", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x40, "White B    Queen", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x20, "White C    Bishop", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x10, "White D    PLAY", KEYCODE_D, CODE_NONE)
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "White E    Knight", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x40, "White F    Castle", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x20, "White G    Pawn", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x10, "White H    md", KEYCODE_H, CODE_NONE)
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "Black 1    King", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x40, "Black 2    Queen", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x20, "Black 3    Bishop", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x10, "Black 4    fp", KEYCODE_4, CODE_NONE)
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "Black 5    Knight", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x40, "Black 6    Castle", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x20, "Black 7    Pawn", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x10, "Black 8    ep", KEYCODE_8, CODE_NONE)
INPUT_PORTS_END

static int mk1_frame_int(void)
{
	return ignore_interrupt();
}

static void mk1_machine_init(void)
{
    f3853_reset();
}

static struct MachineDriver machine_driver_mk1 =
{
	/* basic machine hardware */
	{
		{
			CPU_F8, //MK3850
			1000000,
			mk1_readmem,mk1_writemem,
			mk1_readport,mk1_writeport,
			mk1_frame_int, 1,
        }
	},
	/* frames per second, VBL duration */
	30, DEFAULT_60HZ_VBLANK_DURATION, // led!
	1,				/* single CPU */
	mk1_machine_init,
	0,//pc1401_machine_stop,

	626, 323, { 0, 626 - 1, 0, 323 - 1},
	0,			   /* graphics decode info */
	sizeof (mk1_palette) / sizeof (mk1_palette[0]) + 32768,
	sizeof (mk1_colortable) / sizeof(mk1_colortable[0][0]),
	mk1_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    mk1_vh_start,
	mk1_vh_stop,
	mk1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ 0 }
    }
};

ROM_START(mk1)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("82c210-1", 0x0000, 0x800, 0x278f7bf3)
ROM_END

static void mk1_interrupt(UINT16 addr, bool level)
{
    cpu_irq_line_vector_w(0, 0, addr);
    cpu_set_irq_line(0, F8_INT_INTR, level);
}

void init_mk1(void)
{
    F3853_CONFIG config;
    config.frequency=machine_driver_mk1.cpu[0].cpu_clock;
    config.interrupt_request=mk1_interrupt;

    f3853_init(&config);
}

static const struct IODevice io_mk1[] = {
    { IO_END }
};

// seams to be developed by mostek (MK)
/*    YEAR  NAME    PARENT  MACHINE INPUT   INIT      COMPANY   FULLNAME */
CONSX( 1979,	mk1,	0, 		mk1,	mk1,	mk1,	"Computer Electronic",  "Chess Champion MK I", GAME_NOT_WORKING)

#ifdef RUNTIME_LOADER
extern void mk1_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"mk1")==0) drivers[i]=&driver_mk1;
	}
}
#endif
