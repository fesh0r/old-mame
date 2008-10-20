/***************************************************************************

		Bashkiria-2M machine driver by Miodrag Milanovic

		28/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "deprecat.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "machine/wd17xx.h"
#include "machine/pic8259.h"
#include "machine/msm8251.h"
#include "sound/custom.h"
#include "streams.h"
#include "includes/b2m.h"

static int b2m_8255_porta;
UINT16 b2m_video_scroll;
static int b2m_8255_portc;

UINT8 b2m_video_page;
static UINT8 b2m_drive;
static UINT8 b2m_side;

static UINT8 b2m_romdisk_lsb;
static UINT8 b2m_romdisk_msb;

static UINT8 b2m_color[4];
static UINT8 b2m_localmachine;

static int	b2m_sound_input;
static sound_stream *mixer_channel;


static READ8_HANDLER (b2m_keyboard_r )
{		
	UINT8 key = 0x00;
	if (offset < 0x100) {
		if ((offset & 0x01)!=0) { key |= input_port_read(machine,"LINE0"); }
		if ((offset & 0x02)!=0) { key |= input_port_read(machine,"LINE1"); }
		if ((offset & 0x04)!=0) { key |= input_port_read(machine,"LINE2"); }
		if ((offset & 0x08)!=0) { key |= input_port_read(machine,"LINE3"); }
		if ((offset & 0x10)!=0) { key |= input_port_read(machine,"LINE4"); }
		if ((offset & 0x20)!=0) { key |= input_port_read(machine,"LINE5"); }
		if ((offset & 0x40)!=0) { key |= input_port_read(machine,"LINE6"); }
		if ((offset & 0x80)!=0) { key |= input_port_read(machine,"LINE7"); }					
	} else {
		if ((offset & 0x01)!=0) { key |= input_port_read(machine,"LINE8"); }
		if ((offset & 0x02)!=0) { key |= input_port_read(machine,"LINE9"); }
		if ((offset & 0x04)!=0) { key |= input_port_read(machine,"LINE10"); }
	}
	return key;
}


static void b2m_set_bank(running_machine *machine,int bank) 
{
	UINT8 *rom;
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x27ff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x7000, 0xdfff, 0, 0, SMH_BANK4);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_BANK5);

	rom = memory_region(machine, "main");
	switch(bank) {
		case 0 :
		case 1 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, mess_ram + 0x3000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, rom + 0x10000);						
						break;
#if 0
		case 1 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, rom + 0x12000);
						memory_set_bankptr(4, rom + 0x16000);
						memory_set_bankptr(5, rom + 0x10000);						
						break;
#endif
		case 2 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x10000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, rom + 0x10000);						
						break;
		case 3 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x14000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, rom + 0x10000);						
						break;
		case 4 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x18000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, rom + 0x10000);						
					
						break;
		case 5 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x1c000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, rom + 0x10000);						
					
						break;
		case 6 :
						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, mess_ram + 0x3000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, mess_ram + 0xe000);					
						break;
		case 7 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x27ff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x7000, 0xdfff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(1, rom + 0x10000);	
						memory_set_bankptr(2, rom + 0x10000);	
						memory_set_bankptr(3, rom + 0x10000);	
						memory_set_bankptr(4, rom + 0x10000);	
						memory_set_bankptr(5, rom + 0x10000);						
						break;
	}
}


static PIT8253_OUTPUT_CHANGED(bm2_pit_irq)
{
	pic8259_set_irq_line((device_config*)device_list_find_by_tag( device->machine->config->devicelist, PIC8259, "pic8259"),1,state);		
}


static PIT8253_OUTPUT_CHANGED(bm2_pit_out1)
{
	if (mixer_channel!=NULL) {
		stream_update( mixer_channel );
	}
	b2m_sound_input = state;
}

static PIT8253_OUTPUT_CHANGED(bm2_pit_out2)
{
	pit8253_set_clock_signal( device, 0, state );
}


const struct pit8253_config b2m_pit8253_intf =
{
	{
		{
			0,
			bm2_pit_irq
		},
		{
			0,
			bm2_pit_out1
		},
		{
			2000000,
			bm2_pit_out2
		}
	}
};

static WRITE8_DEVICE_HANDLER (b2m_8255_porta_w )
{	
	b2m_8255_porta = data;
}
static WRITE8_DEVICE_HANDLER (b2m_8255_portb_w )
{	
	b2m_video_scroll = data;
}

static WRITE8_DEVICE_HANDLER (b2m_8255_portc_w )
{	
	b2m_8255_portc = data;
	b2m_set_bank(device->machine, b2m_8255_portc & 7);
	b2m_video_page = (b2m_8255_portc >> 7) & 1;
}

static READ8_DEVICE_HANDLER (b2m_8255_porta_r )
{
	return 0xff;
}
static READ8_DEVICE_HANDLER (b2m_8255_portb_r )
{
	return b2m_video_scroll;
}
static READ8_DEVICE_HANDLER (b2m_8255_portc_r )
{
	return 0xff;
}
const ppi8255_interface b2m_ppi8255_interface_1 =
{
	b2m_8255_porta_r,
	b2m_8255_portb_r,
	b2m_8255_portc_r,
	b2m_8255_porta_w,
	b2m_8255_portb_w,
	b2m_8255_portc_w
};


static WRITE8_DEVICE_HANDLER (b2m_ext_8255_porta_w )
{	
}
static WRITE8_DEVICE_HANDLER (b2m_ext_8255_portb_w )
{	
}

static WRITE8_DEVICE_HANDLER (b2m_ext_8255_portc_w )
{		
	UINT8 drive = ((data >> 1) & 1) ^ 1;
	UINT8 side  = (data  & 1) ^ 1;
	
	if (b2m_drive!=drive) {
		wd17xx_set_drive(drive);
		b2m_drive = drive;
	}
	if (b2m_side!=side) {
		wd17xx_set_side(side);
		b2m_side = side;
	}
}

static READ8_DEVICE_HANDLER (b2m_ext_8255_porta_r )
{
	return 0xff;
}
static READ8_DEVICE_HANDLER (b2m_ext_8255_portb_r )
{
	return 0xff;
}
static READ8_DEVICE_HANDLER (b2m_ext_8255_portc_r )
{
	return 0xff;
}

const ppi8255_interface b2m_ppi8255_interface_2 =
{
	b2m_ext_8255_porta_r,
	b2m_ext_8255_portb_r,
	b2m_ext_8255_portc_r,
	b2m_ext_8255_porta_w,
	b2m_ext_8255_portb_w,
	b2m_ext_8255_portc_w
};

static READ8_DEVICE_HANDLER (b2m_romdisk_porta_r )
{
	UINT8 *romdisk = memory_region(device->machine, "main") + 0x12000;		
	return romdisk[b2m_romdisk_msb*256+b2m_romdisk_lsb];	
}

static WRITE8_DEVICE_HANDLER (b2m_romdisk_portb_w )
{	
	b2m_romdisk_lsb = data;
}

static WRITE8_DEVICE_HANDLER (b2m_romdisk_portc_w )
{			
	b2m_romdisk_msb = data & 0x7f;	
}

const ppi8255_interface b2m_ppi8255_interface_3 =
{
	b2m_romdisk_porta_r,
	NULL,
	NULL,
	NULL,
	b2m_romdisk_portb_w,
	b2m_romdisk_portc_w
};


READ8_HANDLER( b2m_8255_2_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_3" ),offset);
}
WRITE8_HANDLER( b2m_8255_2_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_3" ),offset,data);
}

READ8_HANDLER( b2m_8255_1_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_2" ),offset);
}
WRITE8_HANDLER( b2m_8255_1_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_2" ),offset,data);
}

READ8_HANDLER( b2m_8255_0_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_1" ),offset);
}
WRITE8_HANDLER( b2m_8255_0_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_1" ),offset,data);
}

static PIC8259_SET_INT_LINE( b2m_pic_set_int_line )
{		
	cpunum_set_input_line(device->machine, 0, 0,interrupt ?  HOLD_LINE : CLEAR_LINE);  
} 
static UINT8 vblank_state = 0;


/* Driver initialization */
DRIVER_INIT(b2m)
{
	memset(mess_ram,0,128*1024);	
	vblank_state = 0;
}

WRITE8_HANDLER ( b2m_palette_w ) 
{
	UINT8 b = (3 - ((data >> 6) & 3)) * 0x55;
	UINT8 g = (3 - ((data >> 4) & 3)) * 0x55;
	UINT8 r = (3 - ((data >> 2) & 3)) * 0x55;
	
	UINT8 bw = (3 - (data & 3)) * 0x55;
	
	b2m_color[offset] = data;		
	
	if (input_port_read(machine,"MONITOR")==1) {
		palette_set_color_rgb(machine,offset, r, g, b);
	} else {
		palette_set_color_rgb(machine,offset, bw, bw, bw);
	}
}

READ8_HANDLER ( b2m_palette_r )
{
	return b2m_color[offset];
}

WRITE8_HANDLER ( b2m_localmachine_w ) 
{
	b2m_localmachine = data;
}

READ8_HANDLER ( b2m_localmachine_r ) 
{
	return b2m_localmachine;
}

static const struct msm8251_interface b2m_msm8251_interface =
{
	NULL,
	NULL,
	NULL
};

MACHINE_START(b2m)
{
	wd17xx_init(machine, WD_TYPE_1793, NULL , NULL);	
	wd17xx_set_pause_time(10);
	msm8251_init(&b2m_msm8251_interface);
}

static IRQ_CALLBACK(b2m_irq_callback)
{	
	return pic8259_acknowledge((device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259"));	
} 


const struct pic8259_interface b2m_pic8259_config = {
	b2m_pic_set_int_line
};

INTERRUPT_GEN (b2m_vblank_interrupt)
{	
	//vblank_state++;
	//if (vblank_state>1) vblank_state=0;
	//pic8259_set_irq_line((device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259"),0,vblank_state);		
}
/*static TIMER_CALLBACK (b2m_callback)
{	
	vblank_state++;
	if (vblank_state>1) vblank_state=0;
	pic8259_set_irq_line((device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259"),0,vblank_state);		
}*/

static TIMER_CALLBACK( b2m_reset )
{
	program_write_byte(0xdfff,0x00);
}

MACHINE_RESET(b2m)
{	
	device_config *pit8253 = (device_config*)device_list_find_by_tag( machine->config->devicelist, PIT8253, "pit8253" );
	
	b2m_sound_input = 0;
	
	wd17xx_reset(machine);
	msm8251_reset();
	
	b2m_side = 0;
	b2m_drive = 0;
	wd17xx_set_side(0);
	wd17xx_set_drive(0);

	cpunum_set_irq_callback(0, b2m_irq_callback);
	b2m_set_bank(machine,7);
		
	pit8253_gate_w(pit8253, 0, 0);
	pit8253_gate_w(pit8253, 1, 0);
	pit8253_gate_w(pit8253, 2, 0);
	
	timer_set(ATTOTIME_IN_SEC(2), NULL, 0, b2m_reset);
//	timer_pulse(ATTOTIME_IN_HZ(1), NULL, 0, b2m_callback);
}

DEVICE_IMAGE_LOAD( b2m_floppy )
{
	int size;

	if (! image_has_been_created(image))
		{
		size = image_length(image);

		switch (size)
			{
			case 800*1024:
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy (image) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry (image, 80, 2, 5, 1024, 1, 0, FALSE);	
	return INIT_PASS;
}

static void *b2m_sh_start(int clock, const custom_sound_interface *config);
static void b2m_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length);

const custom_sound_interface b2m_sound_interface =
{
	b2m_sh_start,
	NULL,
	NULL
};

static void *b2m_sh_start(int clock, const custom_sound_interface *config)
{
	b2m_sound_input = 0;
	mixer_channel = stream_create(0, 1, Machine->sample_rate, 0, b2m_sh_update);
	return (void *) ~0;
}

static void b2m_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **buffer,int length)
{
	INT16 channel_1_signal;

	stream_sample_t *sample_left = buffer[0];

	channel_1_signal = b2m_sound_input ? 3000 : -3000;

	while (length--)
	{
		*sample_left = channel_1_signal;		
		sample_left++;
	}
}

void b2m_sh_change_clock(double clock)
{
	if (mixer_channel!=NULL) {
		stream_update(mixer_channel);
	}
}
