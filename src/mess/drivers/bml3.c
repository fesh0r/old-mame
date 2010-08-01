/**************************************************************************************

	Basic Master Level 3 (MB-6890) (c) 1980 Hitachi

	preliminary driver by Angelo Salese

	TODO:
	- no documentation, the entire driver is just a bunch of educated
      guesses ...
	- keyboard shift key, necessary for some key commands
	- understand how to load a tape
	- every time that you switch with NEW ON command, there's a "device i/o error" if
	  the hres_reg bit 5 is active.

	NOTES:
	- NEW ON changes the video mode, they are:
		0: 320 x 200, bit 5 active
		1: 320 x 200, bit 5 unactive
		2: 320 x 375, bit 5 active
		3: 320 x 375, bit 5 unactive
		4: 640 x 200, bit 5 active
		5: 640 x 200, bit 5 unactive
		6: 640 x 375, bit 5 active
		7: 640 x 375, bit 5 unactive
		8-15: same as above plus sets bit 4
		16-31: same as above plus shows the bar at the bottom

**************************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "video/mc6845.h"
#include "sound/beep.h"

static UINT16 cursor_addr,cursor_raster;
static UINT8 attr_latch,io_latch;
static UINT8 hres_reg, vres_reg;
static UINT8 keyb_press,keyb_press_flag;

static VIDEO_START( bml3 )
{
}

static VIDEO_UPDATE( bml3 )
{
	int x,y,count;
	int xi,yi;
	int width,height;
	static UINT8 *gfx_rom = memory_region(screen->machine, "char");
	static UINT8 *vram = memory_region(screen->machine, "vram");

	count = 0x0000;

	width = (hres_reg & 0x80) ? 80 : 40;
	height = (vres_reg & 0x08) ? 1 : 0;

//	popmessage("%02x %02x",hres_reg,vres_reg);

	for(y=0;y<25;y++)
	{
		for(x=0;x<width;x++)
		{
			int tile = vram[count+0x0000];
			int color = vram[count+0x4000] & 7;
			int reverse = vram[count+0x4000] & 8;
			//attr & 0x10 is used ... bitmap mode? (apparently bits 4 and 7 are used for that)

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;

					if(reverse)
						pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? 0 : color;
					else
						pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? color : 0;

					if(height)
					{
						*BITMAP_ADDR16(bitmap, (y*8+yi)*2+0, x*8+xi) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, (y*8+yi)*2+1, x*8+xi) = screen->machine->pens[pen];
					}
					else
						*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = screen->machine->pens[pen];
				}
			}

			if(cursor_addr-0x400 == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(screen->machine->primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(screen->machine->primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							if(height)
							{
								*BITMAP_ADDR16(bitmap, (y*8+yc+7)*2+0, x*8+xc) = screen->machine->pens[0x7];
								*BITMAP_ADDR16(bitmap, (y*8+yc+7)*2+1, x*8+xc) = screen->machine->pens[0x7];
							}
							else
								*BITMAP_ADDR16(bitmap, y*8+yc+7, x*8+xc) = screen->machine->pens[0x7];
						}
					}
				}
			}

			count++;
		}
	}

    return 0;
}

static WRITE8_HANDLER( bml3_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		if(addr_latch == 0x0a)
			cursor_raster = data;
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(space->machine->device("crtc"), 0,data);
	}
}

static READ8_HANDLER( bml3_keyboard_r )
{
	if(keyb_press_flag)
	{
		int res;
		res = keyb_press;
		keyb_press = keyb_press_flag = 0;
		return res | 0x80;
	}

	return 0;
}

/* Note: this custom code is there just for simplicity, it'll be nuked in the end */
static READ8_HANDLER( bml3_io_r )
{
	static UINT8 *rom = memory_region(space->machine, "maincpu");

	if(offset == 0x19) return io_latch;
	if(offset == 0xc4) return 0xff; //some video modes wants this to be high
//	if(offset == 0xc5 || offset == 0xca) return mame_rand(space->machine); //tape related
	if(offset == 0xc8) return 0; //??? checks bit 7, scrolls vertically if active high
	if(offset == 0xc9) return 0x11; //0x01 put 320 x 200 mode, 0x07 = 640 x 375

	/* one of these sets something, apparently available RAM space is smaller if one of these bits is zero (?) */
//	if(offset == 0x40) return 0 ? 0xff : 0x00;
//	if(offset == 0x42) return 0 ? 0xff : 0x00;
//	if(offset == 0x44) return 0 ? 0xff : 0x00;
//	if(offset == 0x46) return 0 ? 0xff : 0x00;

	if(offset == 0xd8) return attr_latch;
	if(offset == 0xe0) return bml3_keyboard_r(space,0);

//	if(offset == 0xcb)

//	if(offset == 0x40 || offset == 0x42 || offset == 0x44 || offset == 0x46)

	if(offset < 0xf0)
	{
		//if(cpu_get_pc(space->cpu) != 0xf838 && cpu_get_pc(space->cpu) != 0xfac4 && cpu_get_pc(space->cpu) != 0xf83c)
		if(offset >= 0xd0 && offset < 0xe0)
			logerror("I/O read [%02x] at PC=%04x\n",offset,cpu_get_pc(space->cpu));
		return 0;//mame_rand(space->machine);
	}

	/* TODO: pretty sure that there's a bankswitch for this */
	return rom[offset+0xff00];
}

static void m6845_change_clock(running_machine *machine, UINT8 setting)
{
	int m6845_clock = XTAL_3_579545MHz;

	switch(setting & 0x88)
	{
		case 0x00: m6845_clock = XTAL_3_579545MHz/4; break; //320 x 200
		case 0x08: m6845_clock = XTAL_3_579545MHz/2; break; //320 x 375
		case 0x80: m6845_clock = XTAL_3_579545MHz/2; break; //640 x 200
		case 0x88: m6845_clock = XTAL_3_579545MHz/1; break; //640 x 375
	}

	mc6845_set_clock(machine->device("crtc"), m6845_clock);
}

static WRITE8_HANDLER( bml3_hres_reg_w )
{
	/*
	x--- ---- width (1) 80 / (0) 40
	-x-- ---- used in some modes, unknown purpose
	--x- ---- used in some modes, unknown purpose (also wants $ffc4 to be 0xff), color / monochrome switch?
	*/

	hres_reg = data;

	m6845_change_clock(space->machine,(hres_reg & 0x80) | (vres_reg & 0x08));
}

static WRITE8_HANDLER( bml3_vres_reg_w )
{
	/*
	---- x--- char height
	*/
	vres_reg = data;

	m6845_change_clock(space->machine,(hres_reg & 0x80) | (vres_reg & 0x08));
}

static WRITE8_HANDLER( bml3_io_w )
{
	if(offset == 0x19)					 		{ io_latch = data; } //???
//	else if(offset == 0xc4)						{ /* system latch, writes 0x53 -> 0x51 when a tape is loaded */}
	else if(offset == 0xc6 || offset == 0xc7) 	{ bml3_6845_w(space,offset-0xc6,data); }
	else if(offset == 0xd0)						{ bml3_hres_reg_w(space,0,data);  }
	else if(offset == 0xd3)						{ beep_set_state(space->machine->device("beeper"),!(data & 0x80)); }
	else if(offset == 0xd6)						{ bml3_vres_reg_w(space,0,data); }
	else if(offset == 0xd8)				 		{ attr_latch = data; }
	else
	{
		logerror("I/O write %02x -> [%02x] at PC=%04x\n",data,offset,cpu_get_pc(space->cpu));
	}
}

static READ8_HANDLER( bml3_vram_r )
{
	static UINT8 *vram = memory_region(space->machine, "vram");

	/* TODO: this presumably also triggers an attr latch read, unsure yet */
	attr_latch = vram[offset+0x4000];

	return vram[offset];
}

static WRITE8_HANDLER( bml3_vram_w )
{
	static UINT8 *vram = memory_region(space->machine, "vram");

	vram[offset] = data;
	vram[offset+0x4000] = attr_latch;
}

static ADDRESS_MAP_START(bml3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x43ff) AM_READWRITE(bml3_vram_r,bml3_vram_w)
	AM_RANGE(0x4400, 0x9fff) AM_RAM
	AM_RANGE(0xff00, 0xffff) AM_READWRITE(bml3_io_r,bml3_io_w)
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END


/* Input ports */

static INPUT_PORTS_START( bml3 )
	PORT_START("key1") //0x00-0x1f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("?")
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Caps Lock?")// (changes the items at the bottom, caps lock?)
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("*")
	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_UNKNOWN) //8
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^")
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNKNOWN) //backspace
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xC2\xA5") //PORT_NAME("�")

	PORT_START("key2") //0x20-0x3f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[")
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@")
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNKNOWN) //0
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(".")
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME) //or cls?
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";")
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]")
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":")
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_UNKNOWN) //4
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_UNKNOWN) //5
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNKNOWN) //6
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)

	PORT_START("key3") //0x40-0x5f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNKNOWN) // /
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("_")
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNKNOWN) //1
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNKNOWN)
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0xffe00000,IP_ACTIVE_HIGH,IPT_UNKNOWN)
INPUT_PORTS_END

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static TIMER_CALLBACK( keyboard_callback )
{
	const char* portnames[3] = { "key1","key2","key3" };
	int i,port_i,scancode;
	scancode = 0;

	for(port_i=0;port_i<3;port_i++)
	{
		for(i=0;i<32;i++)
		{
			if((input_port_read(machine,portnames[port_i])>>i) & 1)
			{
				keyb_press = scancode;
				keyb_press_flag = 1;
				cputag_set_input_line(machine, "maincpu", M6809_IRQ_LINE, HOLD_LINE);
				return;
			}

			scancode++;
		}
	}
}

#if 0
static INTERRUPT_GEN( bml3_irq )
{
	cputag_set_input_line(device->machine, "maincpu", M6809_IRQ_LINE, HOLD_LINE);
}

static INTERRUPT_GEN( bml3_firq )
{
	/* almost surely used to load the tapes */
	cputag_set_input_line(device->machine, "maincpu", M6809_FIRQ_LINE, HOLD_LINE);
}
#endif

static PALETTE_INIT( bml3 )
{
	int i;

	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, i, pal1bit(i >> 1),pal1bit(i >> 2),pal1bit(i >> 0));
}

static MACHINE_START(bml3)
{
	timer_pulse(machine, ATTOTIME_IN_HZ(240/8), NULL, 0, keyboard_callback);
	beep_set_frequency(machine->device("beeper"),1200); //guesswork
	beep_set_state(machine->device("beeper"),0);
}

static MACHINE_RESET(bml3)
{
}

static MACHINE_DRIVER_START( bml3 )
    /* basic machine hardware */
	MDRV_CPU_ADD("maincpu",M6809, XTAL_1MHz)
	MDRV_CPU_PROGRAM_MAP(bml3_mem)
//	MDRV_CPU_VBLANK_INT("screen", bml3_irq )
//	MDRV_CPU_PERIODIC_INT(bml3_firq,45)

	MDRV_MACHINE_START(bml3)
 	MDRV_MACHINE_RESET(bml3)

 	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
    MDRV_PALETTE_INIT(bml3)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

	MDRV_VIDEO_START(bml3)
	MDRV_VIDEO_UPDATE(bml3)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("beeper", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.50)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bml3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
//	ROM_LOAD( "l3bas.rom", 0xa000, 0x6000, BAD_DUMP CRC(d81baa07) SHA1(a8fd6b29d8c505b756dbf5354341c48f9ac1d24d)) //original, 24k isn't a proper rom size!
	/* Handcrafted ROMs, rom labels and contents might not match */
	ROM_LOAD( "598 p16611.ic3", 0xa000, 0x2000, BAD_DUMP CRC(954b9bad) SHA1(047948fac6808717c60a1d0ac9205a5725362430))
	ROM_LOAD( "599 p16561.ic4", 0xc000, 0x2000, BAD_DUMP CRC(b27a48f5) SHA1(94cb616df4caa6415c5076f9acdf675acb7453e2))
	ROM_LOAD( "600 p16681.ic5", 0xe000, 0x2000, BAD_DUMP CRC(fe3988a5) SHA1(edc732f1cd421e0cf45ffcfc71c5589958ceaae7))

    ROM_REGION( 0x800, "char", 0 )
	ROM_LOAD("char.rom", 0x00000, 0x00800, BAD_DUMP CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) ) //Taken from Sharp X1
	ROM_FILL( 0x0000, 0x0008, 0x00)

    ROM_REGION( 0x8000, "vram", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, bml3,  	0,       0, 		bml3, 	bml3, 	 0,  	   "Hitachi",   "Basic Master Level 3",		GAME_NOT_WORKING | GAME_NO_SOUND)

