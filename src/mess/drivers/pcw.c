/******************************************************************************

    pcw.c
    system driver

    Kevin Thacker [MESS driver]

    Thankyou to Jacob Nevins, Richard Fairhurst and Cliff Lawson,
    for their documentation that I used for the development of this
    driver.

    PCW came in 4 forms.
    PCW8256, PCW8512, PCW9256, PCW9512

    These systems were nicknamed "Joyce", apparently after a secretary who worked at
    Amstrad plc.

    These machines were designed for wordprocessing and other business applications.

    The computer came with Locoscript (wordprocessor by Locomotive Software Ltd),
    and CP/M+ (3.1).

    The original PCW8256 system came with a keyboard, green-screen monitor
    (which had 2 3" 80 track, double sided disc drives mounted vertically in it),
    and a dedicated printer. The other systems had different design but shared the
    same hardware.

    Since it was primarily designed as a wordprocessor, there were not many games
    written.
    Some of the games available:
     - Head Over Heels
     - Infocom adventures (Hitchhikers Guide to the Galaxy etc)

    However, it can use the CP/M OS, there is a large variety of CPM software that will
    run on it!!!!!!!!!!!!!!

    Later systems had:
        - black/white monitor,
        - dedicated printer was removed, and support for any printer was added
        - 3" internal drive replaced by a 3.5" drive

    All the logic for the system, except the FDC was found in a Amstrad designed custom
    chip.

    In the original PCW8256, there was no boot-rom. A boot-program was stored in the printer
    chip, and this was activated when the PCW was first switched on. AFAIK there are no
    dumps of this program, so I have written my own replacement.

    The boot-program performs a simple task: Load sector 0, track 0 to &f000 in ram, and execute
    &f010.

    From here CP/M is booted, and the appropiate programs can be run.

    The hardware:
       - Z80 CPU running at 3.4Mhz
       - NEC765 FDC
       - mono display
       - beep (a fixed hz tone which can be turned on/off)
       - 720x256 (PAL) bitmapped display, 720x200 (NTSC) bitmapped display
       - Amstrad CPC6128 style keyboard

  If there are special roms for any of the PCW series I would be interested in them
  so I can implement the driver properly.

  From comp.sys.amstrad.8bit FAQ:

  "Amstrad made the following PCW systems :

  - 1) PCW8256
  - 2) PCW8512
  - 3) PCW9512
  - 4) PCW9512+
  - 5) PcW10
  - 6) PcW16

  1 had 180K drives, 2 had a 180K A drive and a 720K B drive, 3 had only
  720K drives. All subsequent models had 3.5" disks using CP/M format at
  720K until 6 when it switched to 1.44MB in MS-DOS format. The + of
  model 4 was that it had a "real" parallel interface so could be sold
  with an external printer such as the Canon BJ10. The PcW10 wasn't
  really anything more than 4 in a more modern looking case.

  The PcW16 is a radical digression who's sole "raison d'etre" was to
  make a true WYSIWYG product but this meant a change in the screen and
  processor (to 16MHz) etc. which meant that it could not be kept
  compatible with the previous models (though documents ARE compatible)"


  TODO:
  - Printer hardware emulation (8256 etc)
  - Parallel port emulation (9512, 9512+, 10)
  - emulation of serial hardware
  - emulation of other hardware...?
 ******************************************************************************/
#include "driver.h"
#include "deprecat.h"
// nec765 interface
#include "machine/nec765.h"
#include "devices/dsk.h"
// pcw video hardware
#include "includes/pcw.h"
// pcw/pcw16 beeper
#include "sound/beep.h"

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

static void pcw_fdc_interrupt(int);

// pointer to pcw ram
unsigned int roller_ram_addr;
// flag to indicate if boot-program is enabled/disabled
static int 	pcw_boot;
static int	pcw_system_status;
unsigned short roller_ram_offset;
// code for CPU int type generated when FDC int is triggered
static int fdc_interrupt_code;
unsigned char pcw_vdu_video_control_register;
static int pcw_interrupt_counter;

static unsigned long pcw_banks[4];
static unsigned char pcw_bank_force = 0;


static void pcw_update_interrupt_counter(void)
{
	/* never increments past 15! */
	if (pcw_interrupt_counter==0x0f)
		return;

	/* increment count */
	pcw_interrupt_counter++;
}

/* PCW uses NEC765 in NON-DMA mode. FDC Ints are connected to /INT or
/NMI depending on choice (see system control below) */
static const nec765_interface pcw_nec765_interface =
{
	pcw_fdc_interrupt,
	NULL
};

/* determines if int line is held or cleared */
static void pcw_interrupt_handle(running_machine *machine)
{
	if (
		(pcw_interrupt_counter!=0) ||
		((fdc_interrupt_code==1) && ((pcw_system_status & (1<<5))!=0))
		)
	{
		cpunum_set_input_line(machine, 0, 0,HOLD_LINE);
	}
	else
	{
		cpunum_set_input_line(machine, 0, 0,CLEAR_LINE);
	}
}



/* callback for 1/300ths of a second interrupt */
static TIMER_CALLBACK(pcw_timer_interrupt)
{
	pcw_update_interrupt_counter();

	pcw_interrupt_handle(machine);
}

static int previous_fdc_int_state;

/* set/clear fdc interrupt */
static void	pcw_trigger_fdc_int(running_machine *machine)
{
	int state;

	state = pcw_system_status & (1<<5);

	switch (fdc_interrupt_code)
	{
		/* attach fdc to nmi */
		case 0:
		{
			/* I'm assuming that the nmi is edge triggered */
			/* a interrupt from the fdc will cause a change in line state, and
            the nmi will be triggered, but when the state changes because the int
            is cleared this will not cause another nmi */
			/* I'll emulate it like this to be sure */

			if (state!=previous_fdc_int_state)
			{
				if (state)
				{
					/* I'll pulse it because if I used hold-line I'm not sure
                    it would clear - to be checked */
					cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, PULSE_LINE);
				}
			}
		}
		break;

		/* attach fdc to int */
		case 1:
		{
			pcw_interrupt_handle(machine);
		}
		break;

		/* do not interrupt */
		default:
			break;
	}

	previous_fdc_int_state = state;
}

/* fdc interrupt callback. set/clear fdc int */
static void pcw_fdc_interrupt(int state)
{
	pcw_system_status &= ~(1<<5);

	if (state)
	{
		/* fdc interrupt occured */
		pcw_system_status |= (1<<5);
	}

	pcw_trigger_fdc_int(Machine);
}


/* Memory is banked in 16k blocks.

    The upper 16 bytes of block 3, contains the keyboard
    state. This is updated by the hardware.

    block 3 could be paged into any bank, and this explains the
    setup of the memory below.
*/
static ADDRESS_MAP_START(pcw_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(SMH_BANK1, SMH_BANK5)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(SMH_BANK2, SMH_BANK6)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(SMH_BANK3, SMH_BANK7)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(SMH_BANK4, SMH_BANK8)
ADDRESS_MAP_END


/* PCW keyboard is mapped into memory */
static  READ8_HANDLER(pcw_keyboard_r)
{
	static const char *keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
										"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15" };

	return input_port_read(machine, keynames[offset]);
}


/* -----------------------------------------------------------------------
 * PCW Banking
 * ----------------------------------------------------------------------- */

static void pcw_update_read_memory_block(running_machine *machine, int block, int bank)
{
	memory_set_bankptr(block + 1, mess_ram + ((bank * 0x4000) % mess_ram_size));

	/* bank 3? */
	if (bank == 3)
	{
		/* when upper 16 bytes are accessed use keyboard read
           handler */
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,
			block * 0x04000 + 0x3ff0, block * 0x04000 + 0x3fff, 0, 0,
			pcw_keyboard_r);
	}
	else
	{
		/* restore bank handler across entire block */
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,
			block * 0x04000 + 0x0000, block * 0x04000 + 0x3fff, 0, 0,
			(read8_machine_func) (STATIC_BANK1 + (FPTR)block));
	}
}



static void pcw_update_write_memory_block(int block, int bank)
{
	memory_set_bankptr(block + 5, mess_ram + ((bank * 0x4000) % mess_ram_size));
}


/* ----------------------------------------------------------------------- */

/* &F4 O  b7-b4: when set, force memory reads to access the same bank as
writes for &C000, &0000, &8000, and &4000 respectively */

static void pcw_update_mem(running_machine *machine, int block, int data)
{
	/* expansion ram select.
        if block is 0-7, selects internal ram instead for read/write
        */
	if (data & 0x080)
	{
		int bank;

		/* same bank for reading and writing */
		bank = data & 0x7f;

		pcw_update_read_memory_block(machine, block, bank);
		pcw_update_write_memory_block(block, bank);
	}
	else
	{
		/* specify a different bank for reading and writing */
		int write_bank;
		int read_bank;
		int mask=0;

		switch (block)
		{
			case 0:
			{
				mask = (1<<6);
			}
			break;

			case 1:
			{
				mask = (1<<4);
			}
			break;

			case 2:
			{
				mask = (1<<5);
			}
			break;

			case 3:
			{
				mask = (1<<7);
			}
			break;
		}

		if (pcw_bank_force & mask)
		{
			read_bank = data & 0x07;
		}
		else
		{
			read_bank = (data>>4) & 0x07;
		}

		pcw_update_read_memory_block(machine, block, read_bank);

		write_bank = data & 0x07;
		pcw_update_write_memory_block(block, write_bank);
	}

	/* if boot is active, page in fake ROM */
	if ((pcw_boot) && (block==0))
	{
		unsigned char *FakeROM;

		FakeROM = &memory_region(machine, "main")[0x010000];

		memory_set_bankptr(1, FakeROM);
	}
}

/* from Jacob Nevins docs */
static int pcw_get_sys_status(running_machine *machine)
{
	return pcw_interrupt_counter | (input_port_read(machine, "EXTRA") & (0x040 | 0x010)) | pcw_system_status;
}

static READ8_HANDLER(pcw_interrupt_counter_r)
{
	int data;

	/* from Jacob Nevins docs */

	/* get data */
	data = pcw_get_sys_status(machine);
	/* clear int counter */
	pcw_interrupt_counter = 0;
	/* update interrupt */
	pcw_interrupt_handle(machine);
	/* return data */
	return data;
}


static WRITE8_HANDLER(pcw_bank_select_w)
{
	LOG(("BANK: %2x %x\n",offset, data));
	pcw_banks[offset] = data;

	pcw_update_mem(machine, offset, data);
}

static WRITE8_HANDLER(pcw_bank_force_selection_w)
{
	pcw_bank_force = data;

	pcw_update_mem(machine, 0, pcw_banks[0]);
	pcw_update_mem(machine, 1, pcw_banks[1]);
	pcw_update_mem(machine, 2, pcw_banks[2]);
	pcw_update_mem(machine, 3, pcw_banks[3]);
}


static WRITE8_HANDLER(pcw_roller_ram_addr_w)
{
	/*
    Address of roller RAM. b7-5: bank (0-7). b4-1: address / 512. */

	roller_ram_addr = (((data>>5) & 0x07)<<14) |
							((data & 0x01f)<<9);
}

static WRITE8_HANDLER(pcw_pointer_table_top_scan_w)
{
	roller_ram_offset = data;
}

static WRITE8_HANDLER(pcw_vdu_video_control_register_w)
{
	pcw_vdu_video_control_register = data;
}

static WRITE8_HANDLER(pcw_system_control_w)
{
	LOG(("SYSTEM CONTROL: %d\n",data));

	switch (data)
	{
		/* end bootstrap */
		case 0:
		{
			pcw_boot = 0;
			pcw_update_mem(machine, 0, pcw_banks[0]);
		}
		break;

		/* reboot */
		case 1:
		{
		}
		break;

		/* connect fdc interrupt to nmi */
		case 2:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			fdc_interrupt_code = 0;

			/* previously connected to INT? */
			if (fdc_previous_interrupt_code == 1)
			{
				/* yes */

				pcw_interrupt_handle(machine);
			}

			pcw_trigger_fdc_int(machine);
		}
		break;


		/* connect fdc interrupt to interrupt */
		case 3:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			/* connect to INT */
			fdc_interrupt_code = 1;

			/* previously connected to NMI? */
			if (fdc_previous_interrupt_code == 0)
			{
				/* yes */

				/* clear nmi interrupt */
				cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, CLEAR_LINE);
			}

			/* re-issue interrupt */
			pcw_interrupt_handle(machine);
		}
		break;


		/* connect fdc interrupt to neither */
		case 4:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			fdc_interrupt_code = 2;

			/* previously connected to NMI or INT? */
			if ((fdc_previous_interrupt_code == 0) || (fdc_previous_interrupt_code == 1))
			{
				/* yes */

				/* Clear NMI */
				cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, CLEAR_LINE);

			}

			pcw_interrupt_handle(machine);
		}
		break;

		/* set fdc terminal count */
		case 5:
		{
			nec765_set_tc_state(machine, 1);
		}
		break;

		/* clear fdc terminal count */
		case 6:
		{
			nec765_set_tc_state(machine, 0);
		}
		break;

		/* screen on */
		case 7:
		{


		}
		break;

		/* screen off */
		case 8:
		{

		}
		break;

		/* disc motor on */
		case 9:
		{
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1);
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1,1);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
		}
		break;

		/* disc motor off */
		case 10:
		{
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), 0);
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), 0);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1,1);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
		}
		break;

		/* beep on */
		case 11:
		{
			beep_set_state(0,1);
		}
		break;

		/* beep off */
		case 12:
		{
			beep_set_state(0,0);
		}
		break;

	}
}

static READ8_HANDLER(pcw_system_status_r)
{
	/* from Jacob Nevins docs */
	return pcw_get_sys_status(machine);
}

/* read from expansion hardware - additional hardware not part of
the PCW custom ASIC */
static  READ8_HANDLER(pcw_expansion_r)
{
	logerror("pcw expansion r: %04x\n",offset+0x080);

	switch (offset-0x080)
	{
		case 0x0e0:
		{
			/* spectravideo joystick */
			if (input_port_read(machine, "EXTRA") & 0x020)
			{
				return input_port_read(machine, "SPECTRAVIDEO");
			}
			else
			{
				return 0x0ff;
			}
		}

		case 0x09f:
		{

			/* kempston joystick */
			return input_port_read(machine, "KEMPSTON");
		}

		case 0x0e1:
		{
			return 0x07f;
		}

		case 0x085:
		{
			return 0x0fe;
		}

		case 0x087:
		{

			return 0x0ff;
		}

	}

	/* result from floating bus/no peripherial at this port */
	return 0x0ff;
}

/* write to expansion hardware - additional hardware not part of
the PCW custom ASIC */
static WRITE8_HANDLER(pcw_expansion_w)
{
	logerror("pcw expansion w: %04x %02x\n",offset+0x080, data);






}

static READ8_HANDLER(pcw_fdc_r)
{
	/* from Jacob Nevins docs. FDC I/O is not fully decoded */
	if (offset & 1)
	{
		return nec765_data_r(machine, 0);
	}

	return nec765_status_r(machine, 0);
}

static WRITE8_HANDLER(pcw_fdc_w)
{
	/* from Jacob Nevins docs. FDC I/O is not fully decoded */
	if (offset & 1)
	{
		nec765_data_w(machine, 0,data);
	}
}

/* TODO: Implement the printer for PCW8256, PCW8512,PCW9256*/
static WRITE8_HANDLER(pcw_printer_data_w)
{
}

static WRITE8_HANDLER(pcw_printer_command_w)
{
}

static  READ8_HANDLER(pcw_printer_data_r)
{
	return 0x0ff;
}

static  READ8_HANDLER(pcw_printer_status_r)
{
	return 0x0ff;
}

/* TODO: Implement parallel port! */
static  READ8_HANDLER(pcw9512_parallel_r)
{
	if (offset==1)
	{
		return 0xff^0x020;
	}

	logerror("pcw9512 parallel r: offs: %04x\n", (int) offset);
	return 0x00;
}

/* TODO: Implement parallel port! */
static WRITE8_HANDLER(pcw9512_parallel_w)
{
	logerror("pcw9512 parallel w: offs: %04x data: %02x\n",offset,data);
}


static ADDRESS_MAP_START(pcw_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x07f) AM_READWRITE(pcw_fdc_r,					pcw_fdc_w)
	AM_RANGE(0x080, 0x0ef) AM_READWRITE(pcw_expansion_r,			pcw_expansion_w)
	AM_RANGE(0x0f0, 0x0f3) AM_WRITE(								pcw_bank_select_w)
	AM_RANGE(0x0f4, 0x0f4) AM_READWRITE(pcw_interrupt_counter_r,	pcw_bank_force_selection_w)
	AM_RANGE(0x0f5, 0x0f5) AM_WRITE(								pcw_roller_ram_addr_w)
	AM_RANGE(0x0f6, 0x0f6) AM_WRITE(								pcw_pointer_table_top_scan_w)
	AM_RANGE(0x0f7, 0x0f7) AM_WRITE(								pcw_vdu_video_control_register_w)
	AM_RANGE(0x0f8, 0x0f8) AM_READWRITE(pcw_system_status_r,		pcw_system_control_w)
	AM_RANGE(0x0fc, 0x0fc) AM_READWRITE(pcw_printer_data_r,			pcw_printer_data_w)
	AM_RANGE(0x0fd, 0x0fd) AM_READWRITE(pcw_printer_status_r,		pcw_printer_command_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(pcw9512_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x07f) AM_READWRITE(pcw_fdc_r,					pcw_fdc_w)
	AM_RANGE(0x080, 0x0ef) AM_READWRITE(pcw_expansion_r,			pcw_expansion_w)
	AM_RANGE(0x0f0, 0x0f3) AM_WRITE(								pcw_bank_select_w)
	AM_RANGE(0x0f4, 0x0f4) AM_READWRITE(pcw_interrupt_counter_r,	pcw_bank_force_selection_w)
	AM_RANGE(0x0f5, 0x0f5) AM_WRITE(								pcw_roller_ram_addr_w)
	AM_RANGE(0x0f6, 0x0f6) AM_WRITE(								pcw_pointer_table_top_scan_w)
	AM_RANGE(0x0f7, 0x0f7) AM_WRITE(								pcw_vdu_video_control_register_w)
	AM_RANGE(0x0f8, 0x0f8) AM_READWRITE(pcw_system_status_r,		pcw_system_control_w)
	AM_RANGE(0x0fc, 0x0fd) AM_READWRITE(pcw9512_parallel_r,			pcw9512_parallel_w)
ADDRESS_MAP_END



static TIMER_CALLBACK(setup_beep)
{
	beep_set_state(0, 0);
	beep_set_frequency(0, 3750);
}


static MACHINE_START( pcw )
{
	nec765_init(machine, &pcw_nec765_interface,NEC765A,NEC765_RDY_PIN_CONNECTED);
	fdc_interrupt_code = 0;
	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_DS_80);
}


static DRIVER_INIT(pcw)
{
	pcw_boot = 1;

	cpunum_set_input_line_vector(0, 0,0x0ff);


	/* ram paging is actually undefined at power-on */
	pcw_banks[0] = 0;
	pcw_banks[1] = 1;
	pcw_banks[2] = 2;
	pcw_banks[3] = 3;

	pcw_update_mem(machine, 0, pcw_banks[0]);
	pcw_update_mem(machine, 1, pcw_banks[1]);
	pcw_update_mem(machine, 2, pcw_banks[2]);
	pcw_update_mem(machine, 3, pcw_banks[3]);

	/* lower 4 bits are interrupt counter */
	pcw_system_status = 0x000;
	pcw_system_status &= ~((1<<6) | (1<<5) | (1<<4));

	pcw_interrupt_counter = 0;

	roller_ram_offset = 0;

	/* timer interrupt */
	timer_pulse(ATTOTIME_IN_HZ(300), NULL, 0, pcw_timer_interrupt);

	timer_set(attotime_zero, NULL, 0, setup_beep);
}


/*
b7:   k2     k1     [+]    .      ,      space  V      X      Z      del<   alt
b6:   k3     k5     1/2    /      M      N      B      C      lock          k.
b5:   k6     k4     shift  ;      K      J      F      D      A             enter
b4:   k9     k8     k7     ??      L      H      G      S      tab           f8
b3:   paste  copy   #      P      I      Y      T      W      Q             [-]
b2:   f2     cut    return [      O      U      R      E      stop          can
b1:   k0     ptr    ]      -      9      7      5      3      2             extra
b0:   f4     exit   del>   =      0      8      6      4      1             f6
      &3FF0  &3FF1  &3FF2  &3FF3  &3FF4  &3FF5  &3FF6  &3FF7  &3FF8  &3FF9  &3FFA

2008-05 FP: 
Small note about atural keyboard: currently,
- "Paste" is mapped to 'F9'
- "Exit" is mapped to 'F10'
- "Ptr" is mapped to 'Print Screen'
- "Cut" is mapped to 'F11'
- "Copy" is mapped to 'F12'
- "Stop" is mapped to 'Esc'
- [+] is mapped to 'Page Up'
- [-] is mapped to 'Page Down'
- "Extra" is mapped to 'Home'
- "Can" is mapped to 'Insert'
*/

static INPUT_PORTS_START(pcw)
	/* keyboard "ports". These are poked automatically into the PCW address space */

	PORT_START("LINE0")		/* 0x03ff0 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3  F4") PORT_CODE(KEYCODE_F3)		PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)						PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1  F2") PORT_CODE(KEYCODE_F1)		PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Paste") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)						PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)						PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)						PORT_CHAR(UCHAR_MAMEKEY(3_PAD)) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)						PORT_CHAR(UCHAR_MAMEKEY(2_PAD))

	PORT_START("LINE1")		/* 0x03ff1 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Exit") PORT_CODE(KEYCODE_PGDN)		PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ptr") PORT_CODE(KEYCODE_END)		PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Cut") PORT_CODE(KEYCODE_SLASH_PAD)	PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Copy") PORT_CODE(KEYCODE_ASTERISK) 	PORT_CHAR(UCHAR_MAMEKEY(F12))
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)						PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) 						PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) 						PORT_CHAR(UCHAR_MAMEKEY(5_PAD)) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)						PORT_CHAR(UCHAR_MAMEKEY(1_PAD)) PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START("LINE2")		/* 0x03ff2 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL>") PORT_CODE(KEYCODE_DEL)		PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)					PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)						PORT_CHAR('#') PORT_CHAR('>')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)						PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x40, 0x000, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[+]") PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(PGUP))	// 1st key on the left from 'Spacebar'

	PORT_START("LINE3")		/* 0x03ff3 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) 						PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) 						PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) 					PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)							PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)  						PORT_CHAR('\xA7') PORT_CHAR('<')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) 						PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) 						PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) 						PORT_CHAR('.')

	PORT_START("LINE4")		/* 0x03ff4 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)							PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)							PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)							PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)							PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)							PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)							PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)							PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 						PORT_CHAR(',')

	PORT_START("LINE5")		/* 0x03ff5 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)							PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)							PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)							PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)							PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)							PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)							PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)							PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)						PORT_CHAR(' ')

	PORT_START("LINE6")		/* 0x03ff6 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)							PORT_CHAR('6') PORT_CHAR('\'')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)							PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)							PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)							PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)							PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)							PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)							PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)							PORT_CHAR('v') PORT_CHAR('V')

	PORT_START("LINE7")		/* 0x03ff7 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)							PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)							PORT_CHAR('3') PORT_CHAR('\xA3')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)							PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)							PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)							PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)							PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)							PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)							PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("LINE8")		/* 0x03ff8 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)							PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)							PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_ESC)		PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)							PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) 						PORT_CHAR('\t')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)							PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)							PORT_CHAR('z') PORT_CHAR('Z')

	PORT_START("LINE9")		/* 0x03ff9 */
	PORT_BIT(0x07f,0x00, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL<") PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)

	PORT_START("LINE10")	/* 0x03ffa */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5  F6") PORT_CODE(KEYCODE_F5)		PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Extra") PORT_CODE(KEYCODE_LCONTROL)	PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Can") PORT_CODE(KEYCODE_PGUP)		PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[-]") PORT_CODE(KEYCODE_F4)			PORT_CHAR(UCHAR_MAMEKEY(PGDN))	// 1st key on the right from 'Spacebar'
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7  F8") PORT_CODE(KEYCODE_F7) 		PORT_CHAR(UCHAR_MAMEKEY(F7)) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD) 					PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD) 					PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD)) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Alt") PORT_CODE(KEYCODE_LALT)		PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) PORT_CHAR(UCHAR_MAMEKEY(RALT))

	/* at this point the following reflect the above key combinations but in a incomplete
    way. No details available at this time */
	PORT_START("LINE11")	/* 0x03ffb */
	PORT_BIT(0xff, 0x00,	 IPT_UNUSED)

	PORT_START("LINE12")	/* 0x03ffc */
	PORT_BIT(0xff, 0x00,	 IPT_UNUSED)

	/* 2008-05  FP: not sure if this key is correct, "Caps Lock" is already mapped above. 
	For now, I let it with no default mapping. */
	PORT_START("LINE13")	/* 0x03ffd */
	PORT_BIT(0x3f, 0x000, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK") //PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x80, 0x000, IPT_UNUSED)

	PORT_START("LINE14")	/* 0x03ffe */
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	PORT_START("LINE15")	/* 0x03fff */
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* from here on are the pretend dipswitches for machine config etc */
	PORT_START("EXTRA")
	/* vblank */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* frame rate option */
	PORT_DIPNAME( 0x10, 0x010, "50/60Hz Frame Rate Option")
	PORT_DIPSETTING(	0x00, "60hz")
	PORT_DIPSETTING(	0x10, "50Hz" )
	/* spectravideo joystick enabled */
	PORT_DIPNAME( 0x20, 0x020, "Spectravideo Joystick Enabled")
	PORT_DIPSETTING(	0x00, DEF_STR(No))
	PORT_DIPSETTING(	0x20, DEF_STR(Yes) )

	/* Spectravideo joystick */
	/* bit 7: 0
    6: 1
    5: 1
    4: 1 if in E position
    3: 1 if N
    2: 1 if W
    1: 1 if fire pressed
    0: 1 if S
    */

	PORT_START("SPECTRAVIDEO")
	PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)
	PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)
	PORT_BIT(0x008, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)
	PORT_BIT(0x020, 0x020, IPT_UNUSED)
	PORT_BIT(0x040, 0x040, IPT_UNUSED)
	PORT_BIT(0x080, 0x00, IPT_UNUSED)

	/* Kempston joystick */
	PORT_START("KEMPSTON")
	PORT_BIT( 0xff, 0x00,	 IPT_UNUSED)
INPUT_PORTS_END


/* PCW8256, PCW8512, PCW9256 */
static MACHINE_DRIVER_START( pcw )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, 4000000)       /* clock supplied to chip, but in reality it is 3.4Mhz */
	MDRV_CPU_PROGRAM_MAP(pcw_map, 0)
	MDRV_CPU_IO_MAP(pcw_io, 0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(pcw)

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(PCW_SCREEN_WIDTH, PCW_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, PCW_SCREEN_WIDTH-1, 0, PCW_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(PCW_NUM_COLOURS)
	MDRV_PALETTE_INIT( pcw )

	MDRV_VIDEO_START( pcw )
	MDRV_VIDEO_UPDATE( pcw )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


/* PCW9512, PCW9512+, PCW10 */
static MACHINE_DRIVER_START( pcw9512 )
	MDRV_IMPORT_FROM( pcw )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_IO_MAP(pcw9512_io, 0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/* I am loading the boot-program outside of the Z80 memory area, because it
is banked. */

// for now all models use the same rom
#define ROM_PCW(model)												\
	ROM_START(model)												\
		ROM_REGION(0x014000, "main",0)							\
		ROM_LOAD("pcwboot.bin", 0x010000, 608, BAD_DUMP CRC(679b0287) SHA1(5dde974304e3376ace00850d6b4c8ec3b674199e))	\
	ROM_END															\

ROM_PCW(pcw8256)
ROM_PCW(pcw8512)
ROM_PCW(pcw9256)
ROM_PCW(pcw9512)
ROM_PCW(pcw10)

static void generic_pcw_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(generic_pcw)
	CONFIG_DEVICE(generic_pcw_floppy_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(pcw_256k)
	CONFIG_IMPORT_FROM(generic_pcw)
	CONFIG_RAM_DEFAULT(256 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(pcw_512k)
	CONFIG_IMPORT_FROM(generic_pcw)
	CONFIG_RAM_DEFAULT(512 * 1024)
SYSTEM_CONFIG_END

/* these are all variants on the pcw design */
/* major difference is memory configuration and drive type */
/*     YEAR NAME        PARENT      COMPAT  MACHINE   INPUT INIT    CONFIG      COMPANY        FULLNAME */
COMP( 1985, pcw8256,   0,			0,		pcw,	  pcw,	pcw,	pcw_256k,	"Amstrad plc", "PCW8256",		GAME_NOT_WORKING)
COMP( 1985, pcw8512,   pcw8256,	0,		pcw,	  pcw,	pcw,	pcw_512k,	"Amstrad plc", "PCW8512",		GAME_NOT_WORKING)
COMP( 1987, pcw9256,   pcw8256,	0,		pcw,	  pcw,	pcw,	pcw_256k,	"Amstrad plc", "PCW9256",		GAME_NOT_WORKING)
COMP( 1987, pcw9512,   pcw8256,	0,		pcw9512,  pcw,	pcw,	pcw_512k,	"Amstrad plc", "PCW9512 (+)",	GAME_NOT_WORKING)
COMP( 1993, pcw10,	    pcw8256,	0,		pcw9512,  pcw,	pcw,	pcw_512k,	"Amstrad plc", "PCW10",			GAME_NOT_WORKING)

