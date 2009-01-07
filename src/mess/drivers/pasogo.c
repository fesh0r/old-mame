/******************************************************************************
 PeT mess@utanet.at march 2008


 vadem vg230 (single chip pc) based handheld

although it is very related to standard pc hardware, it is different enough
to make the standard pc driver one level more complex, so own driver

******************************************************************************/

#include "driver.h"
#include "cpu/i86/i86.h"
#include "devices/cartslot.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
/*
  rtc interrupt irq 2
 */

static struct {
	UINT8 index;
	UINT8 data[0x100];
	struct {
		UINT16 data;
	} bios_timer; // 1.19 MHz tclk signal
	struct {
		int seconds, minutes, hours, days;
		int alarm_seconds, alarm_minutes, alarm_hours, alarm_days;

		int onehertz_interrupt_on;
		int onehertz_interrupt_request;
		int alarm_interrupt_on;
		int alarm_interrupt_request;
	} rtc;
	struct {
		int write_protected;
	} pmu;
} vg230;


static TIMER_CALLBACK( vg230_timer )
{
	vg230.rtc.seconds+=1;
	if (vg230.rtc.seconds>=60)
	{
		vg230.rtc.seconds=00;
		vg230.rtc.minutes+=1;
		if (vg230.rtc.minutes>=60) 
		{
			vg230.rtc.minutes=0;
			vg230.rtc.hours+=1;
			if (vg230.rtc.hours>=24) 
			{
				vg230.rtc.hours=0;
				vg230.rtc.days=(vg230.rtc.days+1)&0xfff;
			}
		}
	}

	if (vg230.rtc.seconds==vg230.rtc.alarm_seconds
		&& vg230.rtc.minutes==vg230.rtc.alarm_minutes
		&& vg230.rtc.hours==vg230.rtc.alarm_hours
		&& (vg230.rtc.days&0x1f)==vg230.rtc.alarm_hours) 
	{
		// generate alarm
	}
}

static void vg230_reset(running_machine *machine)
{
	mame_system_time systime;

	memset(&vg230, 0, sizeof(vg230));
	vg230.pmu.write_protected=TRUE;
	timer_pulse(machine, ATTOTIME_IN_HZ(1), NULL, 0, vg230_timer);


	mame_get_base_datetime(machine, &systime);
  
	vg230.rtc.seconds= systime.local_time.second;
	vg230.rtc.minutes= systime.local_time.minute;
	vg230.rtc.hours = systime.local_time.hour;
	vg230.rtc.days = 0;

	vg230.bios_timer.data=0x7200; // HACK
}

static void vg230_init(running_machine *machine)
{
	vg230_reset(machine);
}


static READ8_HANDLER( vg230_io_r )
{
	int log=TRUE;
	UINT8 data=0;
	vg230.bios_timer.data+=0x100; //HACK
	if (offset&1) 
	{
		data=vg230.data[vg230.index];
		switch (vg230.index) 
		{
			case 0x09: break;
			case 0x0a:
				if (vg230.data[9]&1) 
				{
					data=input_port_read(space->machine, "JOY");
				}
				else 
				{
					data=0xff;
				}
				break;

			case 0x30:
				data=vg230.bios_timer.data&0xff;
				break;

			case 0x31:
				data=vg230.bios_timer.data>>8;
				log=FALSE;
				break;

			case 0x70: data=vg230.rtc.seconds; log=FALSE; break;
			case 0x71: data=vg230.rtc.minutes; log=FALSE; break;
			case 0x72: data=vg230.rtc.hours; log=FALSE; break;
			case 0x73: data=vg230.rtc.days; break;
			case 0x74: data=vg230.rtc.days>>8; break;
			case 0x79: /*rtc status*/log=FALSE; break;
			case 0x7a:
				data&=~3;
				if (vg230.rtc.alarm_interrupt_request) data|=1<<1;
				if (vg230.rtc.onehertz_interrupt_request) data|=1<<0;
				break;

			case 0xc1:
				data&=~1;
				if (vg230.pmu.write_protected) data|=1;
				vg230.pmu.write_protected=FALSE;
				log=FALSE;
				break;
		}

		if (log)
			logerror("%.5x vg230 %02x read %.2x\n",(int) cpu_get_pc(space->cpu),vg230.index,data);
      //	data=memory_region(machine, "main")[0x4000+offset];
	} 
	else 
	{      
		data=vg230.index;
    }
	return data;
}

static WRITE8_HANDLER( vg230_io_w )
{
	int log=TRUE;
	if (offset&1) 
	{
		//	memory_region(machine, "main")[0x4000+offset]=data;
		vg230.data[vg230.index]=data;
		switch (vg230.index) 
		{
			case 0x09: break;
			case 0x70: vg230.rtc.seconds=data&0x3f; break;
			case 0x71: vg230.rtc.minutes=data&0x3f; break;
			case 0x72: vg230.rtc.hours=data&0x1f;break;
			case 0x73: vg230.rtc.days=(vg230.rtc.days&~0xff)|data; break;
			case 0x74: vg230.rtc.days=(vg230.rtc.days&0xff)|((data&0xf)<<8); break;
			case 0x75: vg230.rtc.alarm_seconds=data&0x3f; break;
			case 0x76: vg230.rtc.alarm_minutes=data&0x3f; break;
			case 0x77: vg230.rtc.alarm_hours=data&0x1f; break;
			case 0x78: vg230.rtc.days=data&0x1f; break;
			case 0x79:
				vg230.rtc.onehertz_interrupt_on=data&1;
				vg230.rtc.alarm_interrupt_on=data&2;
				log=FALSE;
				break;

			case 0x7a:
				if (data&2) 
				{ 
					vg230.rtc.alarm_interrupt_request=FALSE; vg230.rtc.onehertz_interrupt_request=FALSE; /* update interrupt */ 
				}
				break;
		}
		
		if (log)
			logerror("%.5x vg230 %02x write %.2x\n",(int)cpu_get_pc(space->cpu),vg230.index,data);
	} 
	else 
	{
		vg230.index=data;
	}
}

static struct {
	UINT8 data;
	int index;
	struct {
		UINT8 data[2];
		int address;
		int type;
		int on;
	} mapper[26];
} ems/*?*/;

static READ8_HANDLER( ems_r )
{
	UINT8 data=0;
	switch (offset) 
	{
		case 0: data=ems.data; break;
		case 2: case 3: data=ems.mapper[ems.index].data[offset&1]; break;
	}
	return data;
}

static WRITE8_HANDLER( ems_w )
{
	switch (offset) 
	{
	case 0:
		ems.data=data;
		switch (data&~3) 
		{
		case 0x80: ems.index=0; break;
		case 0x84: ems.index=1; break;
		case 0x88: ems.index=2; break;
		case 0x8c: ems.index=3; break;
		case 0x90: ems.index=4; break;
		case 0x94: ems.index=5; break;
		case 0x98: ems.index=6; break;
		case 0x9c: ems.index=7; break;
		case 0xa0: ems.index=8; break;
		case 0xa4: ems.index=9; break;
		case 0xa8: ems.index=10; break;
		case 0xac: ems.index=11; break;
		case 0xb0: ems.index=12; break;
		case 0xb4: ems.index=13; break;
      //  case 0xb8: ems.index=14; break;
      //  case 0xbc: ems.index=15; break;
		case 0xc0: ems.index=14; break;
		case 0xc4: ems.index=15; break;
		case 0xc8: ems.index=16; break;
		case 0xcc: ems.index=17; break;
		case 0xd0: ems.index=18; break;
		case 0xd4: ems.index=19; break;
		case 0xd8: ems.index=20; break;
		case 0xdc: ems.index=21; break;
		case 0xe0: ems.index=22; break;
		case 0xe4: ems.index=23; break;
		case 0xe8: ems.index=24; break;
		case 0xec: ems.index=25; break;
		}
		break;
	
	case 2:
	case 3:
		ems.mapper[ems.index].data[offset&1]=data;
		ems.mapper[ems.index].address=(ems.mapper[ems.index].data[0]<<14)|((ems.mapper[ems.index].data[1]&0xf)<<22);
		ems.mapper[ems.index].on=ems.mapper[ems.index].data[1]&0x80;
		ems.mapper[ems.index].type=(ems.mapper[ems.index].data[1]&0x70)>>4;
		logerror("%.5x ems mapper %d(%05x)on:%d type:%d address:%07x\n",(int)cpu_get_pc(space->cpu),ems.index, ems.data<<12,
			ems.mapper[ems.index].on, ems.mapper[ems.index].type, ems.mapper[ems.index].address );
		switch (ems.mapper[ems.index].type) 
		{
		case 0: /*external*/ 
		case 1: /*ram*/
		memory_set_bankptr( space->machine, ems.index+1, memory_region(space->machine, "main") + (ems.mapper[ems.index].address&0xfffff) );
		break;
		case 3: /* rom 1 */
		case 4: /* pc card a */
		case 5: /* pc card b */
		default:
		break;
		case 2:
		memory_set_bankptr( space->machine, ems.index+1, memory_region(space->machine, "user1") + (ems.mapper[ems.index].address&0xfffff) );
		break;
		}
		break;
	}
}

static ADDRESS_MAP_START( pasogo_mem, ADDRESS_SPACE_PROGRAM, 8 )
//	AM_RANGE(0x00000, 0xfffff) AM_UNMAP AM_MASK(0xfffff)
//	AM_RANGE( 0x4000, 0x7fff) AM_READWRITE(gmaster_io_r, gmaster_io_w)
	ADDRESS_MAP_GLOBAL_MASK(0xffFFF)
	AM_RANGE(0x00000, 0x7ffff) AM_RAM
	AM_RANGE(0x80000, 0x83fff) AM_RAMBANK(1)
	AM_RANGE(0x84000, 0x87fff) AM_RAMBANK(2)
	AM_RANGE(0x88000, 0x8bfff) AM_RAMBANK(3)
	AM_RANGE(0x8c000, 0x8ffff) AM_RAMBANK(4)
	AM_RANGE(0x90000, 0x93fff) AM_RAMBANK(5)
	AM_RANGE(0x94000, 0x97fff) AM_RAMBANK(6)
	AM_RANGE(0x98000, 0x9bfff) AM_RAMBANK(7)
	AM_RANGE(0x9c000, 0x9ffff) AM_RAMBANK(8)
	AM_RANGE(0xa0000, 0xa3fff) AM_RAMBANK(9)
	AM_RANGE(0xa4000, 0xa7fff) AM_RAMBANK(10)
	AM_RANGE(0xa8000, 0xabfff) AM_RAMBANK(11)
	AM_RANGE(0xac000, 0xaffff) AM_RAMBANK(12)
	AM_RANGE(0xb0000, 0xb3fff) AM_RAMBANK(13)
	AM_RANGE(0xb4000, 0xb7fff) AM_RAMBANK(14)
//	AM_RANGE(0xb8000, 0xbffff) AM_RAM
	AM_RANGE(0xb8000, 0xbffff) AM_RAMBANK(28)
	AM_RANGE(0xc0000, 0xc3fff) AM_RAMBANK(15)
	AM_RANGE(0xc4000, 0xc7fff) AM_RAMBANK(16)
	AM_RANGE(0xc8000, 0xcbfff) AM_RAMBANK(17)
	AM_RANGE(0xcc000, 0xcffff) AM_RAMBANK(18)
	AM_RANGE(0xd0000, 0xd3fff) AM_RAMBANK(19)
	AM_RANGE(0xd4000, 0xd7fff) AM_RAMBANK(20)
	AM_RANGE(0xd8000, 0xdbfff) AM_RAMBANK(21)
	AM_RANGE(0xdc000, 0xdffff) AM_RAMBANK(22)
	AM_RANGE(0xe0000, 0xe3fff) AM_RAMBANK(23)
	AM_RANGE(0xe4000, 0xe7fff) AM_RAMBANK(24)
	AM_RANGE(0xe8000, 0xebfff) AM_RAMBANK(25)
	AM_RANGE(0xec000, 0xeffff) AM_RAMBANK(26)

	AM_RANGE(0xf0000, 0xfffff) AM_ROMBANK(27)
ADDRESS_MAP_END

static ADDRESS_MAP_START(pasogo_io, ADDRESS_SPACE_IO, 8)
//	ADDRESS_MAP_GLOBAL_MASK(0xfFFF)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE(PIC8259, "pic8259", pic8259_r, pic8259_w)
	AM_RANGE(0x26, 0x27) AM_READWRITE(vg230_io_r, vg230_io_w )
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE(PIT8254, "pit8254", pit8253_r, pit8253_w)
	AM_RANGE(0x6c, 0x6f) AM_READWRITE(ems_r, ems_w )
ADDRESS_MAP_END

static INPUT_PORTS_START( pasogo )
PORT_START("JOY")
//	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT)  PORT_NAME("select")
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START) PORT_NAME("start")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("O") /*?*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("X") /*?*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("a") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("b") PORT_CODE(KEYCODE_B)
INPUT_PORTS_END

/* palette in red, green, blue tribles */
static const unsigned char pasogo_palette[][3] =
{
	{ 0, 0, 0 },
	{ 45,45,43 },
	{ 130, 159, 166 },
	{ 255,255,255 }
};

static PALETTE_INIT( pasogo )
{
	int i;

	for ( i = 0; i < ARRAY_LENGTH(pasogo_palette); i++ ) 
	{
		palette_set_color_rgb(machine, i, pasogo_palette[i][0], pasogo_palette[i][1], pasogo_palette[i][2]);
	}
}

static VIDEO_UPDATE( pasogo )
{
	static int width=-1, height=-1;
	UINT8 *rom = memory_region(screen->machine, "main")+0xb8000;
	UINT16 c[]={ 3, 0 };
	int x,y;
//	plot_box(bitmap, 0, 0, 64/*bitmap->width*/, bitmap->height, 0);
	int w=640;
	int h=240;
	if (0) 
	{
		w=320;
		h=240;
		for (y=0;y<h; y++) 
		{
			for (x=0;x<w;x+=4) 
			{
				int a=(y&1)*0x2000;
				UINT8 d=rom[a+(y&~1)*80/2+x/4];
				UINT16 *line=BITMAP_ADDR16(bitmap, y, x);
				*line++=(d>>6)&3;
				*line++=(d>>4)&3;
				*line++=(d>>2)&3;
				*line++=(d>>0)&3;
			}
		}
	} 
	else 
	{
		for (y=0;y<h; y++) 
		{
			for (x=0;x<w;x+=8) 
			{
				int a=(y&3)*0x2000;
				UINT8 d=rom[a+(y&~3)*80/4+x/8];
				UINT16 *line=BITMAP_ADDR16(bitmap, y, x);
				*line++=c[(d>>7)&1];
				*line++=c[(d>>6)&1];
				*line++=c[(d>>5)&1];
				*line++=c[(d>>4)&1];
				*line++=c[(d>>3)&1];
				*line++=c[(d>>2)&1];
				*line++=c[(d>>1)&1];
				*line++=c[(d>>0)&1];
			}
		}
	}
	if (w!=width || h!=height) 
	{
		width=w; height=h;
//		video_screen_set_visarea(machine->primary_screen, 0, width-1, 0, height-1);
		video_screen_set_visarea(screen, 0, width-1, 0, height-1);
	}
	return 0;
}

static INTERRUPT_GEN( pasogo_interrupt )
{
//	cpu_set_input_line(machine->cpu[0], UPD7810_INTFE1, PULSE_LINE);
}

#if 0
static const custom_sound_interface gmaster_sound_interface =
{
	gmaster_custom_start
};
#endif

static IRQ_CALLBACK(pasogo_irq_callback)
{
	return pic8259_acknowledge( (device_config*)device_list_find_by_tag( device->machine->config->devicelist, PIC8259, "pic8259"));
}

static MACHINE_RESET( pasogo )
{
	cpu_set_irq_callback(machine->cpu[0], pasogo_irq_callback);
}

//static const unsigned i86_address_mask = 0x000fffff;

static PIT8253_OUTPUT_CHANGED( pc_timer0_w )
{
	pic8259_set_irq_line((device_config*)device_list_find_by_tag( device->machine->config->devicelist, PIC8259, "pic8259"), 0, state);
}

static const struct pit8253_config pc_pit8254_config =
{
	{
		{
			4772720/4,				/* heartbeat IRQ */
			pc_timer0_w
		}, {
			4772720/4,				/* dram refresh */
			NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			NULL
		}
	}
};


static PIC8259_SET_INT_LINE( pasogo_pic8259_set_int_line ) 
{
	cpu_set_input_line(device->machine->cpu[0], 0, interrupt ? HOLD_LINE : CLEAR_LINE);
}


static const struct pic8259_interface pasogo_pic8259_config = {
	pasogo_pic8259_set_int_line
};

static DEVICE_IMAGE_LOAD( pasogo_cart )
{
	UINT8 *user = memory_region(image->machine, "user1");
	int size;
	size = image_length(image);

	if (image_fread(image, user, size) != size)
	{
		logerror("%s load error\n", image_filename(image));
		return 1;
	}

	return 0;
}

static MACHINE_DRIVER_START( pasogo )
	MDRV_CPU_ADD("main", I80188/*V30HL in vadem vg230*/, 10000000/*?*/)
	MDRV_CPU_PROGRAM_MAP(pasogo_mem, 0)
	MDRV_CPU_IO_MAP( pasogo_io, 0 )
	MDRV_CPU_VBLANK_INT("main", pasogo_interrupt)
//	MDRV_CPU_CONFIG(i86_address_mask)
	MDRV_MACHINE_RESET( pasogo )

	MDRV_PIT8254_ADD( "pit8254", pc_pit8254_config )

	MDRV_PIC8259_ADD( "pic8259", pasogo_pic8259_config )

	MDRV_SCREEN_ADD("main", LCD)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(pasogo_palette))
	MDRV_VIDEO_UPDATE(pasogo)
	MDRV_PALETTE_INIT(pasogo)
#if 0
	MDRV_SPEAKER_STANDARD_MONO("gmaster")
	MDRV_SOUND_ADD("custom", CUSTOM, 0)
	MDRV_SOUND_CONFIG(gmaster_sound_interface)
	MDRV_SOUND_ROUTE(0, "gmaster", 0.50)
#endif

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(pasogo_cart)
MACHINE_DRIVER_END


ROM_START(pasogo)
	ROM_REGION(0x100000,"main", ROMREGION_ERASEFF) // 1 megabyte dram?
//	ROM_LOAD("gmaster.bin", 0x0000, 0x1000, CRC(05cc45e5) SHA1(05d73638dea9657ccc2791c0202d9074a4782c1e) )
//	ROM_CART_LOAD(0, "bin", 0x8000, 0x8000, 0)
	ROM_REGION(0x100000,"user1", ROMREGION_ERASEFF)
ROM_END


static DRIVER_INIT( pasogo )
{
	vg230_init(machine);
	memset(&ems, 0, sizeof(ems));
	memory_set_bankptr( machine, 27, memory_region(machine, "user1") + 0x00000 );
	memory_set_bankptr( machine, 28, memory_region(machine, "main") + 0xb8000/*?*/ );
}

/*    YEAR      NAME            PARENT  MACHINE   INPUT     INIT                
	  COMPANY                 FULLNAME */
CONS( 1996, pasogo,       0,          0, pasogo,  pasogo,    pasogo,   0, "KOEI", "PasoGo", GAME_NO_SOUND|GAME_NOT_WORKING)

