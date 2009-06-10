/***************************************************************************

        PP-01 driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/pp01.h"

static UINT8 memory_block[16];
UINT8 pp01_video_scroll;
UINT8 pp01_video_write_mode;

WRITE8_HANDLER (pp01_video_write_mode_w)
{
	pp01_video_write_mode = data & 0x0f;
}

/* Driver initialization */
DRIVER_INIT(pp01)
{
	memset(mess_ram, 0, 64 * 1024);
}


static void pp01_video_w(UINT8 block,UINT16 offset,UINT8 data,UINT8 part)
{
	UINT16 addroffset = part ? 0x1000  : 0x0000;
	
	if (BIT(pp01_video_write_mode,3)) {
		// Copy mode
		if(BIT(pp01_video_write_mode,0)) {
			mess_ram[0x6000+offset+addroffset] = data;
		} else {
			mess_ram[0x6000+offset+addroffset] = 0;
		}
		if(BIT(pp01_video_write_mode,1)) {
			mess_ram[0xa000+offset+addroffset] = data;
		} else {
			mess_ram[0xa000+offset+addroffset] = 0;
		}
		if(BIT(pp01_video_write_mode,2)) {
			mess_ram[0xe000+offset+addroffset] = data;
		} else {
			mess_ram[0xe000+offset+addroffset] = 0;
		}
	} else {
		if (block==0) {
			mess_ram[0x6000+offset+addroffset] = data;
		}
		if (block==1) {
			mess_ram[0xa000+offset+addroffset] = data;
		}
		if (block==2) {
			mess_ram[0xe000+offset+addroffset] = data;
		}
	}	
}

static WRITE8_HANDLER (pp01_video_r_1_w)
{
	pp01_video_w(0,offset,data,0);
}
static WRITE8_HANDLER (pp01_video_g_1_w)
{
	pp01_video_w(1,offset,data,0);
}
static WRITE8_HANDLER (pp01_video_b_1_w)
{
	pp01_video_w(2,offset,data,0);
}

static WRITE8_HANDLER (pp01_video_r_2_w)
{
	pp01_video_w(0,offset,data,1);
}
static WRITE8_HANDLER (pp01_video_g_2_w)
{
	pp01_video_w(1,offset,data,1);
}
static WRITE8_HANDLER (pp01_video_b_2_w)
{
	pp01_video_w(2,offset,data,1);
}


static void pp01_set_memory(running_machine *machine,UINT8 block, UINT8 data)
{	
	UINT8 *mem = memory_region(machine, "maincpu");	
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 startaddr = block*0x1000;
	UINT16 endaddr   = ((block+1)*0x1000)-1;
	UINT8  blocknum  = block + 1;
	if (data>=0xE0 && data<=0xEF) {
		// This is RAM
		memory_install_read8_handler (space, startaddr, endaddr, 0, 0, SMH_BANK(blocknum));
		switch(data) {
			case 0xe6 : 
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_r_1_w);	
					break;
			case 0xe7 : 
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_r_2_w);	
					break;
			case 0xea : 
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_g_1_w);	
					break;
			case 0xeb : 
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_g_2_w);	
					break;
			case 0xee : 
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_b_1_w);	
					break;
			case 0xef : 
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_b_2_w);	
					break;
			
			default :
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, SMH_BANK(blocknum));	
					break;
		}
		
		memory_set_bankptr(machine, blocknum, mess_ram + (data & 0x0F)* 0x1000);
	} else if (data>=0xF8) {
		memory_install_read8_handler (space, startaddr, endaddr, 0, 0, SMH_BANK(blocknum));
		memory_install_write8_handler(space, startaddr, endaddr, 0, 0, SMH_UNMAP);	
		memory_set_bankptr(machine, blocknum, mem + ((data & 0x0F)-8)* 0x1000+0x10000);
	} else {
		logerror("%02x %02x\n",block,data);
		memory_install_read8_handler (space, startaddr, endaddr, 0, 0, SMH_UNMAP);	
		memory_install_write8_handler(space, startaddr, endaddr, 0, 0, SMH_UNMAP);	
	}
}
	
MACHINE_RESET( pp01 )
{
	int i;
	memset(memory_block,0xff,16);
	for(i=0;i<16;i++) {
		memory_block[i] = 0xff;
		pp01_set_memory(machine, i, 0xff);
	}
}

WRITE8_HANDLER (pp01_mem_block_w) 
{
	memory_block[offset] = data;
	pp01_set_memory(space->machine, offset, data);
}

READ8_HANDLER (pp01_mem_block_r)
{
	return 	memory_block[offset];
}

MACHINE_START(pp01)
{
}


static PIT8253_OUTPUT_CHANGED(pp01_pit_out0)
{
}

static PIT8253_OUTPUT_CHANGED(pp01_pit_out1)
{
}

static PIT8253_OUTPUT_CHANGED(pp01_pit_out2)
{
	pit8253_set_clock_signal( device, 0, state );
}


const struct pit8253_config pp01_pit8253_intf =
{
	{
		{
			0,
			pp01_pit_out0
		},
		{
			2000000,
			pp01_pit_out1
		},
		{
			2000000,
			pp01_pit_out2
		}
	}
};

static READ8_DEVICE_HANDLER (pp01_8255_porta_r )
{
	return pp01_video_scroll;
}
static WRITE8_DEVICE_HANDLER (pp01_8255_porta_w )
{	
	pp01_video_scroll = data;
}

UINT8 pp01_key_line = 0;
static READ8_DEVICE_HANDLER (pp01_8255_portb_r )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
											"LINE8", "LINE9", "LINEA", "LINEB", "LINEC", "LINED", "LINEE", "LINEF" };
	
	return (input_port_read(device->machine,keynames[pp01_key_line]) & 0x3F) | (input_port_read(device->machine,"LINEALL") & 0xC0);
}
static WRITE8_DEVICE_HANDLER (pp01_8255_portb_w )
{	
	//logerror("pp01_8255_portb_w %02x\n",data);
	
}

static WRITE8_DEVICE_HANDLER (pp01_8255_portc_w )
{	
	pp01_key_line = data & 0x0f;
}

static READ8_DEVICE_HANDLER (pp01_8255_portc_r )
{	
	return 0xff;
}



const ppi8255_interface pp01_ppi8255_interface =
{
	DEVCB_HANDLER(pp01_8255_porta_r),
	DEVCB_HANDLER(pp01_8255_portb_r),
	DEVCB_HANDLER(pp01_8255_portc_r),
	DEVCB_HANDLER(pp01_8255_porta_w),
	DEVCB_HANDLER(pp01_8255_portb_w),
	DEVCB_HANDLER(pp01_8255_portc_w)
};

