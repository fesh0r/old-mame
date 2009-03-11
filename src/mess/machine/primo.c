/*******************************************************************************

	primo.c

	Functions to emulate general aspects of Microkey Primo computers
	(RAM, ROM, interrupts, I/O ports)

	Krzysztof Strzecha

*******************************************************************************/

/* Core includes */
#include "driver.h"
#include "includes/primo.h"

/* Components */
#include "cpu/z80/z80.h"
#include "includes/cbmserb.h"
#include "includes/cbmdrive.h"
#include "sound/speaker.h"

/* Devices */
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"


static UINT8 primo_port_FD = 0x00;
static int primo_nmi = 0;
static int serial_atn = 1, serial_clock = 1, serial_data = 1;

/*******************************************************************************

	Interrupt callback

*******************************************************************************/

INTERRUPT_GEN( primo_vblank_interrupt )
{
	if (primo_nmi)
		cpu_set_input_line(device, INPUT_LINE_NMI, PULSE_LINE);
}

/*******************************************************************************

	Memory banking

*******************************************************************************/

static void primo_update_memory(running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0],ADDRESS_SPACE_PROGRAM);
	switch (primo_port_FD & 0x03)
	{
		case 0x00:	/* Original ROM */
			memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
			memory_set_bankptr(machine,1, memory_region(machine, "maincpu")+0x10000);
			break;
		case 0x01:	/* EPROM extension 1 */
			memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
			memory_set_bankptr(machine,1, memory_region(machine, "maincpu")+0x14000);
			break;
		case 0x02:	/* RAM */
			memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
			memory_set_bankptr(machine,1, memory_region(machine, "maincpu"));
			break;
		case 0x03:	/* EPROM extension 2 */
			memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
			memory_set_bankptr(machine,1, memory_region(machine, "maincpu")+0x18000);
			break;
	}
	logerror ("Memory update: %02x\n", primo_port_FD);
}

/*******************************************************************************

	IO read/write handlers

*******************************************************************************/

READ8_HANDLER( primo_be_1_r )
{
	UINT8 data = 0x00;
	static const char *const portnames[] = { "IN0", "IN1", "IN2", "IN3" };

	// bit 7, 6 - not used

	// bit 5 - VBLANK
	data |= (video_screen_get_vblank(space->machine->primary_screen)) ? 0x20 : 0x00;

	// bit 4 - I4 (external bus)

	// bit 3 - I3 (external bus)

	// bit 2 - cassette
	data |= (cassette_input(devtag_get_device(space->machine, "cassette")) < 0.1) ? 0x04 : 0x00;

	// bit 1 - reset button
	data |= (input_port_read(space->machine, "RESET")) ? 0x02 : 0x00;

	// bit 0 - keyboard
	data |= (input_port_read(space->machine, portnames[(offset & 0x0030) >> 4]) >> (offset&0x000f)) & 0x0001 ? 0x01 : 0x00;

	return data;
}

READ8_HANDLER( primo_be_2_r )
{
	UINT8 data = 0xff;

	// bit 7, 6 - not used

	// bit 5 - SCLK
	if (!serial_clock || !cbm_serial_clock_read (space->machine))
		data &= ~0x20;

	// bit 4 - SDATA
	if (!serial_data || !cbm_serial_data_read (space->machine))
		data &= ~0x10;

	// bit 3 - SRQ
//	data &= (!cbm_serial_request_read ()) ? ~0x08 : ~0x00;

	// bit 2 - joystic 2 (not implemeted yet)

	// bit 1 - ATN
	if (!serial_atn || !cbm_serial_atn_read (space->machine))
		data &= ~0x02;

	// bit 0 - joystic 1 (not implemeted yet)

	logerror ("IOR BE-2 data:%02x\n", data);
	return data;
}

WRITE8_HANDLER( primo_ki_1_w )
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	// bit 7 - NMI generator enable/disable
	primo_nmi = (data & 0x80) ? 1 : 0;

	// bit 6 - joystick register shift (not emulated)

	// bit 5 - V.24 (2) / tape control (not emulated)

	// bit 4 - speaker
	speaker_level_w(speaker, (data&0x10)>>4);

	// bit 3 - display buffer
	if (data & 0x08)
		primo_video_memory_base |= 0x2000;
	else
		primo_video_memory_base &= 0xdfff;

	// bit 2 - V.24 (1) / tape control (not emulated)

	// bit 1, 0 - cassette output
	switch (data & 0x03)
	{
		case 0:
			cassette_output(devtag_get_device(space->machine, "cassette"), -1.0);
			break;
		case 1:
		case 2:
			cassette_output(devtag_get_device(space->machine, "cassette"), 0.0);
			break;
		case 3:
			cassette_output(devtag_get_device(space->machine, "cassette"), 1.0);
			break;
	}
}

WRITE8_HANDLER( primo_ki_2_w )
{
	// bit 7, 6 - not used

	// bit 5 - SCLK
	cbm_serial_clock_write (space->machine, serial_clock = !(data & 0x20));
	logerror ("W - SCLK: %d ", serial_clock ? 1 : 0);

	// bit 4 - SDATA
	cbm_serial_data_write (space->machine, serial_data = !(data & 0x10));
	logerror ("SDATA: %d ", serial_data ? 1 : 0);

	// bit 3 - not used

	// bit 2 - SRQ
	cbm_serial_request_write (space->machine, !(data & 0x04));

	// bit 1 - ATN
	cbm_serial_atn_write (space->machine, serial_atn = !(data & 0x02));
	logerror ("ATN: %d\n", serial_atn ? 1 : 0);

	// bit 0 - not used

	if (data & 0x04)
		logerror ("SRQ WRITE\n");
//	logerror ("IOW KI-2 data:%02x\n", data);
}

WRITE8_HANDLER( primo_FD_w )
{
	if (!input_port_read(space->machine, "MEMORY_EXPANSION"))
	{
		primo_port_FD = data;
		primo_update_memory(space->machine);
	}
}

/*******************************************************************************

	Driver initialization

*******************************************************************************/

static void primo_common_driver_init (void)
{
	primo_port_FD = 0x00;
}

DRIVER_INIT( primo32 )
{
	primo_common_driver_init();
	primo_video_memory_base = 0x6800;
}

DRIVER_INIT( primo48 )
{
	primo_common_driver_init();
	primo_video_memory_base = 0xa800;
}

DRIVER_INIT( primo64 )
{
	primo_common_driver_init();
	primo_video_memory_base = 0xe800;
}

/*******************************************************************************

	Machine initialization

*******************************************************************************/

static void primo_common_machine_init (running_machine *machine)
{
	if (input_port_read(machine, "MEMORY_EXPANSION"))
		primo_port_FD = 0x00;
	primo_update_memory(machine);
	cpu_set_clockscale(machine->cpu[0], input_port_read(machine, "CPU_CLOCK") ? 1.5 : 1.0);
}

MACHINE_RESET( primoa )
{
	primo_common_machine_init(machine);
}

MACHINE_RESET( primob )
{
	primo_common_machine_init(machine);

	cbm_serial_config(machine, &cbm_sim_drive_interface);	
	cbm_serial_reset_write (machine, 0);
	cbm_drive_0_config (SERIAL, 8);
	cbm_drive_1_config (SERIAL, 9);
}

/*******************************************************************************

	Snapshot files (.pss)

*******************************************************************************/

static void primo_setup_pss (running_machine *machine, UINT8* snapshot_data, UINT32 snapshot_size)
{
	int i;
	const device_config *speaker = devtag_get_device(machine, "speaker");

	/* Z80 registers */

	cpu_set_reg(machine->cpu[0], Z80_BC, snapshot_data[4] + snapshot_data[5]*256);
	cpu_set_reg(machine->cpu[0], Z80_DE, snapshot_data[6] + snapshot_data[7]*256);
	cpu_set_reg(machine->cpu[0], Z80_HL, snapshot_data[8] + snapshot_data[9]*256);
	cpu_set_reg(machine->cpu[0], Z80_AF, snapshot_data[10] + snapshot_data[11]*256);
	cpu_set_reg(machine->cpu[0], Z80_BC2, snapshot_data[12] + snapshot_data[13]*256);
	cpu_set_reg(machine->cpu[0], Z80_DE2, snapshot_data[14] + snapshot_data[15]*256);
	cpu_set_reg(machine->cpu[0], Z80_HL2, snapshot_data[16] + snapshot_data[17]*256);
	cpu_set_reg(machine->cpu[0], Z80_AF2, snapshot_data[18] + snapshot_data[19]*256);
	cpu_set_reg(machine->cpu[0], Z80_PC, snapshot_data[20] + snapshot_data[21]*256);
	cpu_set_reg(machine->cpu[0], Z80_SP, snapshot_data[22] + snapshot_data[23]*256);
	cpu_set_reg(machine->cpu[0], Z80_I, snapshot_data[24]);
	cpu_set_reg(machine->cpu[0], Z80_R, snapshot_data[25]);
	cpu_set_reg(machine->cpu[0], Z80_IX, snapshot_data[26] + snapshot_data[27]*256);
	cpu_set_reg(machine->cpu[0], Z80_IY, snapshot_data[28] + snapshot_data[29]*256);


	/* IO ports */

	// KI-1 bit 7 - NMI generator enable/disable
	primo_nmi = (snapshot_data[30] & 0x80) ? 1 : 0;

	// KI-1 bit 4 - speaker
	speaker_level_w(speaker, (snapshot_data[30]&0x10)>>4);


	/* memory */

	for (i=0; i<0xc000; i++)
		memory_write_byte(cpu_get_address_space(machine->cpu[0],ADDRESS_SPACE_PROGRAM), i+0x4000, snapshot_data[i+38]);
}

SNAPSHOT_LOAD( primo )
{
	UINT8 *snapshot_data;

	if (!(snapshot_data = (UINT8*) malloc(snapshot_size)))
		return INIT_FAIL;

	if (image_fread(image, snapshot_data, snapshot_size) != snapshot_size)
	{
		free(snapshot_data);
		return INIT_FAIL;
	}

	if (strncmp((char *)snapshot_data, "PS01", 4))
	{
		free(snapshot_data);
		return INIT_FAIL;
	}

	primo_setup_pss(image->machine,snapshot_data, snapshot_size);

	free(snapshot_data);
	return INIT_PASS;
}

/*******************************************************************************

	Quicload files (.pp)

*******************************************************************************/


static void primo_setup_pp (running_machine *machine,UINT8* quickload_data, UINT32 quickload_size)
{
	int i;

	UINT16 load_addr;
	UINT16 start_addr;

	load_addr = quickload_data[0] + quickload_data[1]*256;
	start_addr = quickload_data[2] + quickload_data[3]*256;

	for (i=4; i<quickload_size; i++)
		memory_write_byte(cpu_get_address_space(machine->cpu[0],ADDRESS_SPACE_PROGRAM), start_addr+i-4, quickload_data[i]);

	cpu_set_reg(machine->cpu[0], Z80_PC, start_addr);

	logerror ("Quickload .pp l: %04x r: %04x s: %04x\n", load_addr, start_addr, quickload_size-4);
}

QUICKLOAD_LOAD( primo )
{
	UINT8 *quickload_data;

	if (!(quickload_data = (UINT8*) malloc(quickload_size)))
		return INIT_FAIL;

	if (image_fread(image, quickload_data, quickload_size) != quickload_size)
	{
		free(quickload_data);
		return INIT_FAIL;
	}

	primo_setup_pp(image->machine, quickload_data, quickload_size);

	free(quickload_data);
	return INIT_PASS;
}