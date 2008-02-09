/******************************************************************************
 watara supervision handheld

 PeT mess@utanet.at in december 2000
******************************************************************************/

#include <assert.h>
#include "driver.h"
#include "deprecat.h"
#include "mslegacy.h"
#include "cpu/m6502/m6502.h"

#include "includes/svision.h"
#include "devices/cartslot.h"
#include "svision.lh"

#define MAKE8_RGB15(red3, green3, blue2) ( ( (red3)<<(10+2)) | ( (green3)<<(5+2)) | ( (blue2)<<(0+3)) )
#define MAKE9_RGB15(red3, green3, blue3) ( ( (red3)<<(10+2)) | ( (green3)<<(5+2)) | ( (blue3)<<(0+2)) )
#define MAKE12_RGB15(red4, green4, blue4) ( ( (red4)<<(10+1)) | ((green4)<<(5+1)) | ((blue4)<<(0+1)) )
#define MAKE24_RGB15(red8, green8, blue8) ( (((red8)&0xf8)<<(10-3)) | (((green8)&0xf8)<<(5-3)) | (((blue8)&0xf8)>>3) )


// in pixel
#define XSIZE (svision_reg[0]&~3)
#define XPOS svision_reg[2]
#define YPOS svision_reg[3]
#define BANK svision_reg[0x26]

static UINT8 *svision_reg;

static struct
{
	emu_timer *timer1;
	int timer_shot;
} svision;

static struct
{
	int state;
	int on, clock, data;
	UINT8 input;
	emu_timer *timer;
} svision_pet;

static struct
{
	UINT16 palette[4/*0x40?*/]; /* rgb8 */
	int palette_on;
} tvlink;

static TIMER_CALLBACK(svision_pet_timer)
{
	switch (svision_pet.state)
	{
		case 0:
			svision_pet.input = readinputport(1);
			/* fall through */

		case 2: case 4: case 6: case 8:
		case 10: case 12: case 14:
			svision_pet.clock=svision_pet.state&2;
			svision_pet.data=svision_pet.input&1;
			svision_pet.input>>=1;
			svision_pet.state++;
			break;

		case 16+15:
			svision_pet.state = 0;
			break;

		default:
			svision_pet.state++;
			break;
	}
}

void svision_irq(void)
{
	int irq = svision.timer_shot && (BANK & 2);
	irq = irq || (svision_dma.finished && (BANK & 4));

	cpunum_set_input_line(Machine, 0, M6502_IRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static TIMER_CALLBACK(svision_timer)
{
    svision.timer_shot = TRUE;
    timer_enable(svision.timer1, FALSE);
    svision_irq();
}

static READ8_HANDLER(svision_r)
{
	int data = svision_reg[offset];
	switch (offset)
	{
		case 0x20:
			data = readinputport(0);
			break;
		case 0x21:
			data &= ~0xf;
			data |= svision_reg[0x22] & 0xf;
			if (svision_pet.on)
			{
				if (!svision_pet.clock)
					data &= ~4;
				if (!svision_pet.data)
					data &= ~8;
			}
			break;
		case 0x27:
			data &= ~3;
			if (svision.timer_shot)
				data|=1;
			if (svision_dma.finished)
				data|=2;
			break;
		case 0x24:
			svision.timer_shot = FALSE;
			svision_irq();
			break;
		case 0x25:
			svision_dma.finished = FALSE;
			svision_irq();
			break;
		default:
			logerror("%.6f svision read %04x %02x\n", attotime_to_double(timer_get_time()),offset,data);
			break;
	}

	return data;
}

static WRITE8_HANDLER(svision_w)
{
	int value;
	int delay;

	svision_reg[offset] = data;

	switch (offset)
	{
		case 2:
		case 3:
			break;
		case 0x26: /* bits 5,6 memory management for a000? */
			logerror("%.6f svision write %04x %02x\n", attotime_to_double(timer_get_time()),offset,data);
			memory_set_bankptr(1, memory_region(REGION_USER1) + ((svision_reg[0x26] & 0xe0) << 9));
			svision_irq();
			break;
		case 0x23: /* delta hero irq routine write */
			value = data;
			if (!data)
				value = 0x100;
			if (BANK & 0x10)
				delay = 16384;
			else
				delay = 256;
			timer_enable(svision.timer1, TRUE);
			timer_reset(svision.timer1, ATTOTIME_IN_CYCLES(value * delay, 0));
			break;
		case 0x10: case 0x11: case 0x12: case 0x13:
			svision_soundport_w(svision_channel + 0, offset & 3, data);
			break;
		case 0x14: case 0x15: case 0x16: case 0x17:
			svision_soundport_w(svision_channel + 1, offset & 3, data);
			break;
		case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c:
			svision_sounddma_w(offset - 0x18, data);
			break;
		case 0x28: case 0x29: case 0x2a:
			svision_noise_w(offset - 0x28, data);
			break;
		default:
			logerror("%.6f svision write %04x %02x\n", attotime_to_double(timer_get_time()), offset, data);
			break;
	}
}

static READ8_HANDLER(tvlink_r)
{
	switch(offset)
	{
		default:
			if (offset >= 0x800 && offset < 0x840)
			{
				/* strange effects when modifying palette */
				return svision_r(offset);
			}
			else
			{
				return svision_r(offset);
			}
	}
}

static WRITE8_HANDLER(tvlink_w)
{
	switch (offset)
	{
		case 0x0e:
			svision_reg[offset] = data;
			tvlink.palette_on = data & 1;
			if (tvlink.palette_on)
			{
				// hack, normally initialising with palette from ram
				tvlink.palette[0] = MAKE12_RGB15(163/16,172/16,115/16); // these are the tron colors messured from screenshot
				tvlink.palette[1] = MAKE12_RGB15(163/16,155/16,153/16);
				tvlink.palette[2] = MAKE12_RGB15(77/16,125/16,73/16);
				tvlink.palette[3] = MAKE12_RGB15(59/16,24/16,20/16);
			}
			else
			{
				// cleaner to use colors from compile time palette, or compose from "fixed" palette values
				tvlink.palette[0]=MAKE12_RGB15(0,0,0);
				tvlink.palette[1]=MAKE12_RGB15(5*16/256,18*16/256,9*16/256);
				tvlink.palette[2]=MAKE12_RGB15(48*16/256,76*16/256,100*16/256);
				tvlink.palette[3]=MAKE12_RGB15(190*16/256,190*16/256,190*16/256);
			}
			break;
		default:
			svision_w(offset,data);
			if (offset >= 0x800 && offset < 0x840)
			{
				UINT16 c;
				if (offset == 0x803 && data == 0x07)
				{
					/* tron hack */
					svision_reg[0x0804]=0x00;
					svision_reg[0x0805]=0x01;
					svision_reg[0x0806]=0x00;
					svision_reg[0x0807]=0x00;
				}
				c = svision_reg[0x800] | (svision_reg[0x804] << 8);
				tvlink.palette[0] = MAKE9_RGB15( (c>>0)&7, (c>>3)&7, (c>>6)&7);
				c = svision_reg[0x801] | (svision_reg[0x805] << 8);
				tvlink.palette[1] = MAKE9_RGB15( (c>>0)&7, (c>>3)&7, (c>>6)&7);
				c = svision_reg[0x802] | (svision_reg[0x806]<<8);
				tvlink.palette[2]=MAKE9_RGB15( (c>>0)&7, (c>>3)&7, (c>>6)&7);
				c = svision_reg[0x803] | (svision_reg[0x807]<<8);
				tvlink.palette[3]=MAKE9_RGB15( (c>>0)&7, (c>>3)&7, (c>>6)&7);
				/* writes to palette effect video color immediately */
				/* some writes modify other registers, */
				/* encoding therefor not known (rgb8 or rgb9) */
			}
	}
}

static ADDRESS_MAP_START( svision_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM
	AM_RANGE( 0x2000, 0x3fff) AM_READWRITE(svision_r, svision_w) AM_BASE(&svision_reg)
	AM_RANGE( 0x4000, 0x5fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE( 0x6000, 0x7fff) AM_NOP
	AM_RANGE( 0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE( 0xc000, 0xffff) AM_ROMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tvlink_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM
	AM_RANGE( 0x2000, 0x3fff) AM_READWRITE(tvlink_r, tvlink_w) AM_BASE(&svision_reg)
	AM_RANGE( 0x4000, 0x5fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE( 0x6000, 0x7fff) AM_NOP
	AM_RANGE( 0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE( 0xc000, 0xffff) AM_ROMBANK(2)
ADDRESS_MAP_END

static INPUT_PORTS_START( svision )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("B")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("A")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT) PORT_NAME("Select")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START) PORT_NAME("Start/Pause")
INPUT_PORTS_END

static INPUT_PORTS_START( svisions )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("B") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("A") PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT) PORT_NAME("Select") PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START) PORT_NAME("Start/Pause") PORT_PLAYER(1)
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("2nd B") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("2nd A") PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT) PORT_NAME("2nd Select") PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START) PORT_NAME("2nd Start/Pause") PORT_PLAYER(2)
INPUT_PORTS_END

/* most games contain their graphics in roms, and have hardware to
   draw complete rectangular objects */

#define PALETTE_START 0

/* palette in red, green, blue triples */
static const unsigned char svision_palette[] =
{
#if 0
    /* greens grabbed from a scan of a handheld
     * in its best adjustment for contrast
     */
	86, 121, 86,
	81, 115, 90,
	74, 107, 101,
	54, 78, 85
#else
	/* grabbed from chris covell's black white pics */
	0xe0, 0xe0, 0xe0,
	0xb9, 0xb9, 0xb9,
	0x54, 0x54, 0x54,
	0x12, 0x12, 0x12
#endif
};

/* palette in red, green, blue tribles */
static const unsigned char svisionp_palette[] =
{
	// pal
	1, 1, 3,
	5, 18, 9,
	48, 76, 100,
	190, 190, 190
};

/* palette in red, green, blue tribles */
static const unsigned char svisionn_palette[] =
{
	0, 0, 0,
	188, 242, 244, // darker
	129, 204, 255,
	245, 249, 248
};

static PALETTE_INIT( svision )
{
	palette_set_colors_rgb(machine, 0, svision_palette, sizeof(svision_palette) / 3);
}
static PALETTE_INIT( svisionn )
{
	palette_set_colors_rgb(machine, 0, svisionn_palette, sizeof(svisionn_palette) / 3);
}
static PALETTE_INIT( svisionp )
{
	palette_set_colors_rgb(machine, 0, svisionp_palette, sizeof(svisionp_palette) / 3);
}

static VIDEO_UPDATE( svision )
{
	int x, y, i, j=XPOS/4+YPOS*0x30;
	UINT8 *vram= videoram;

	if (BANK&8)
	{
		for (y=0; y<160; y++)
		{
			UINT16 *line = BITMAP_ADDR16(bitmap, y, 3 - (XPOS & 3));
			for (x=3-(XPOS&3),i=0; x<160+3 && x<XSIZE+3; x+=4,i++)
			{
				UINT8 b=vram[j+i];
				line[3]=((b>>6)&3)+PALETTE_START;
				line[2]=((b>>4)&3)+PALETTE_START;
				line[1]=((b>>2)&3)+PALETTE_START;
				line[0]=((b>>0)&3)+PALETTE_START;
				line+=4;
			}
			j += 0x30;
			if (j >= 8160)
				j = 0; //sssnake
		}
	}
	else
	{
		plot_box(bitmap, 3, 0, 162, 159, machine->pens[PALETTE_START]);
	}
	return 0;
}

static VIDEO_UPDATE( tvlink )
{
	int x, y, i, j = XPOS/4+YPOS*0x30;
	UINT8 *vram = videoram;

	if (BANK & 8)
	{
		for (y = 0; y < 160; y++)
		{
			UINT16 *line = BITMAP_ADDR16(bitmap, y, 3 - (XPOS & 3));
			for (x = 3 - (XPOS & 3), i = 0; x < 160 + 3 && x < XSIZE + 3; x += 4, i++)
			{
				UINT8 b=vram[j+i];
				line[3]=tvlink.palette[(b>>6)&3];
				line[2]=tvlink.palette[(b>>4)&3];
				line[1]=tvlink.palette[(b>>2)&3];
				line[0]=tvlink.palette[(b>>0)&3];
				line+=4;
			}
			j += 0x30;
			if (j >= 8160)
				j = 0; //sssnake
		}
	}
	else
	{
		plot_box(bitmap, 3, 0, 162, 159, machine->pens[PALETTE_START]);
	}
	return 0;
}

static INTERRUPT_GEN( svision_frame_int )
{
	if (BANK&1)
		cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, PULSE_LINE);

	if (svision_channel->count)
		svision_channel->count--;
	if (svision_channel[1].count)
		svision_channel[1].count--;
	if (svision_noise.count)
		svision_noise.count--;
}

static DRIVER_INIT( svision )
{
	svision.timer1 = timer_alloc(svision_timer, NULL);
	svision_pet.on = FALSE;
	memory_set_bankptr(2, memory_region(REGION_USER1) + 0x1c000);
}

static DRIVER_INIT( svisions )
{
	svision.timer1 = timer_alloc(svision_timer, NULL);
	memory_set_bankptr(2, memory_region(REGION_USER1) + 0x1c000);
	svision.timer1 = timer_alloc(svision_timer, NULL);
	svision_pet.on = TRUE;
	svision_pet.timer = timer_alloc(svision_pet_timer, NULL);
	timer_pulse(attotime_mul(ATTOTIME_IN_SEC(8), 256/cpunum_get_clock(0)), NULL, 0, svision_pet_timer);
}

static MACHINE_RESET( svision )
{
	svision.timer_shot = FALSE;
	svision_dma.finished = FALSE;
	memory_set_bankptr(1, memory_region(REGION_USER1));
}


static MACHINE_RESET( tvlink )
{
	svision.timer_shot = FALSE;
	svision_dma.finished = FALSE;
	memory_set_bankptr(1, memory_region(REGION_USER1));
	tvlink.palette_on = FALSE;

	memset(svision_reg + 0x800, 0xff, 0x40); // normally done from tvlink microcontroller
	svision_reg[0x82a] = 0xdf;

	tvlink.palette[0] = MAKE24_RGB15(svisionp_palette[(PALETTE_START+0)*3+0], svisionp_palette[(PALETTE_START+0)*3+1], svisionp_palette[(PALETTE_START+0)*3+2]);
	tvlink.palette[1] = MAKE24_RGB15(svisionp_palette[(PALETTE_START+1)*3+0], svisionp_palette[(PALETTE_START+1)*3+1], svisionp_palette[(PALETTE_START+1)*3+2]);
	tvlink.palette[2] = MAKE24_RGB15(svisionp_palette[(PALETTE_START+2)*3+0], svisionp_palette[(PALETTE_START+2)*3+1], svisionp_palette[(PALETTE_START+2)*3+2]);
	tvlink.palette[3] = MAKE24_RGB15(svisionp_palette[(PALETTE_START+3)*3+0], svisionp_palette[(PALETTE_START+3)*3+1], svisionp_palette[(PALETTE_START+3)*3+2]);
}

static const struct CustomSound_interface svision_sound_interface =
{
	svision_custom_start
};


static MACHINE_DRIVER_START( svision )
	/* basic machine hardware */
	 MDRV_CPU_ADD_TAG("main", M65C02, 4000000)        /* ? stz used! speed? */
	MDRV_CPU_PROGRAM_MAP(svision_mem, 0)
	MDRV_CPU_VBLANK_INT(svision_frame_int, 1)
	MDRV_SCREEN_REFRESH_RATE(61)

	MDRV_MACHINE_RESET( svision )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)	/* lcd */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_ASPECT_RATIO(160, 160)*/
	MDRV_SCREEN_SIZE(3+160+3, 160)
	MDRV_SCREEN_VISIBLE_AREA(3+0, 3+160-1, 0, 160-1)
	MDRV_PALETTE_LENGTH(sizeof(svision_palette) / (sizeof(svision_palette[0]) * 3))
	MDRV_PALETTE_INIT( svision )

	MDRV_VIDEO_UPDATE( svision )
	MDRV_DEFAULT_LAYOUT(layout_svision)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(svision_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( svisionp )
	MDRV_IMPORT_FROM( svision )
	MDRV_CPU_REPLACE( "main", M65C02, 4430000)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_PALETTE_INIT( svisionp )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( svisionn )
	MDRV_IMPORT_FROM( svision )
	MDRV_CPU_REPLACE( "main", M65C02, 3560000/*?*/)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_PALETTE_INIT( svisionn )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tvlinkp )
	MDRV_IMPORT_FROM( svisionp )
	 MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tvlink_mem, 0)

	MDRV_MACHINE_RESET( tvlink )

	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_VIDEO_UPDATE( tvlink )

MACHINE_DRIVER_END

ROM_START(svision)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_CART_LOAD(0, "bin,ws,sv", 0x0000, 0x20000, ROM_MIRROR)
ROM_END

#define rom_svisions rom_svision
#define rom_svisionn rom_svision
#define rom_svisionp rom_svision
#define rom_tvlinkp rom_svision

SYSTEM_CONFIG_START(svision)
	CONFIG_DEVICE(cartslot_device_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG      COMPANY     FULLNAME */
// marketed under a ton of firms and names
CONS(1992,	svision,	0,	0,	svision,	svision,	svision,	svision,	"Watara",	"Supervision", 0)
// svdual 2 connected via communication port
CONS( 1992, svisions,      svision,          0,svision,  svisions,    svisions,   svision, "Watara", "Super Vision (PeT Communication Simulation)", 0 )

CONS( 1993, svisionp,      svision,          0,svisionp,  svision,    svision,   svision, "Watara", "Super Vision (PAL TV Link Colored)", 0 )
CONS( 1993, svisionn,      svision,          0,svisionn,  svision,    svision,   svision, "Watara", "Super Vision (NTSC TV Link Colored)", 0 )
// svtvlink (2 supervisions)
// tvlink (pad supervision simulated)
CONS( 199?, tvlinkp,      svision,          0,tvlinkp,  svision,    svision,   svision, "Watara", "TV Link PAL", 0 )
