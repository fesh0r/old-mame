/******************************************************************************

        avigo.c

        TI "Avigo" PDA


        system driver

        Documentation:
                Hans B Pufal
                Avigo simulator


        MEMORY MAP:
                0x0000-0x03fff: flash 0 block 0
                0x4000-0x07fff: flash x block y
				0x8000-0x0bfff: ram block x, screen buffer, or flash x block y
                0xc000-0x0ffff: ram block 0

		Hardware:
			- Z80 CPU
            - 16c500c UART
			-  amd29f080 flash-file memory x 3 (3mb)
			- 128k ram
			- stylus pen
			- touch-pad screen
        TODO:
                Dissassemble the rom a bit and find out exactly
                how memory paging works!

			I don't have any documentation on the hardware, so a lot of this
			driver has been written using educated guesswork and a lot of help
			from an existing emulation written by Hans Pufal. Hans's emulator
			is also written from educated guesswork.

        Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "includes/avigo.h"
#include "includes/am29f080.h"
#include "includes/tc8521.h"
#include "includes/uart8250.h"


/* dummy timer used to read keys and pen stylus and generate an interrupt */
static void *avigo_dummy_timer;

static UINT8 avigo_key_line;
/* 
	bit 7:						?? high priority. When it occurs, clear this bit.
	bit 6: pen int				
	 An interrupt when pen is pressed against screen.

	bit 5: real time clock
		

	bit 4: 
	
	  
	bit 3: uart int				
	
	  
	bit 2: synchronisation link interrupt???keyboard int			;; check bit 5 of port 1,
	
	bit 1: ???		(cleared in nmi, and then set again)

*/

static UINT8 avigo_irq;
/* 128k ram */
unsigned char *avigo_memory;

/* bit 3 = speaker state */
static UINT8 avigo_speaker_data;

/* bits 0-5 define bank index */
static unsigned long avigo_ram_bank_l;
static unsigned long avigo_ram_bank_h;
/* bits 0-5 define bank index */
static unsigned long avigo_rom_bank_l;
static unsigned long avigo_rom_bank_h;
static unsigned long avigo_ad_control_status;
static  int avigo_flash_at_0x4000;
static  int avigo_flash_at_0x8000;

/* memory 0x0000-0x03fff */
static READ_HANDLER(avigo_flash_0x0000_read_handler)
{

        int flash_offset = offset;

	return amd_flash_bank_handler_r(0, flash_offset);
}

/* memory 0x04000-0x07fff */
static READ_HANDLER(avigo_flash_0x4000_read_handler)
{

        int flash_offset = (avigo_rom_bank_l<<14) | offset;

	//	logerror("flash offset: %04x offset: %04x\n",flash_offset, offset);


        return amd_flash_bank_handler_r(avigo_flash_at_0x4000, flash_offset);
}

/* memory 0x0000-0x03fff */
static WRITE_HANDLER(avigo_flash_0x0000_write_handler)
{

	int flash_offset = offset;

	amd_flash_bank_handler_w(0, flash_offset, data);
}

/* memory 0x04000-0x07fff */
static WRITE_HANDLER(avigo_flash_0x4000_write_handler)
{

        int flash_offset = (avigo_rom_bank_l<<14) | offset;

	//	logerror("flash offset: %04x offset: %04x\n",flash_offset, offset);

        amd_flash_bank_handler_w(avigo_flash_at_0x4000, flash_offset, data);
}

/* memory 0x08000-0x0bfff */
static READ_HANDLER(avigo_flash_0x8000_read_handler)
{

        int flash_offset = (avigo_ram_bank_l<<14) | offset;

	//	logerror("flash offset: %04x offset: %04x\n",flash_offset, offset);

        return amd_flash_bank_handler_r(avigo_flash_at_0x8000, flash_offset);
}

/* memory 0x08000-0x0bfff */
static WRITE_HANDLER(avigo_flash_0x8000_write_handler)
{

        int flash_offset = (avigo_ram_bank_l<<14) | offset;

	//	logerror("flash offset: %04x offset: %04x\n",flash_offset, offset);

        amd_flash_bank_handler_w(avigo_flash_at_0x8000, flash_offset, data);
}


static READ_HANDLER(avigo_ram_0xc000_read_handler)
{
	return avigo_memory[offset];
}

static WRITE_HANDLER(avigo_ram_0xc000_write_handler)
{
	avigo_memory[offset] = data;
}


static void avigo_refresh_ints(void)
{
	if (avigo_irq!=0)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}
}


/* previous input port data */
static int previous_input_port_data[4];
static int stylus_marker_x;
static int stylus_marker_y;
static int stylus_press_x;
static int stylus_press_y;

/* this is not a real interrupt. This timer updates the stylus position from mouse
movements, and checks if the mouse button is pressed to emulate a press of the stylus to the screen.
*/
static void avigo_dummy_timer_callback(int dummy)
{
	int i;
	int current_input_port_data[4];
	int changed;
	static int ox = 0, oy = 0;
	int nx,ny;
    int dx, dy;

	for (i=0; i<4; i++)
	{
		current_input_port_data[i] = readinputport(i);
	}

	changed = current_input_port_data[3]^previous_input_port_data[3];

	if ((changed & 0x01)!=0)
	{
		if ((current_input_port_data[3] & 0x01)!=0)
		{
			/* pen pressed to screen */
			
			logerror("pen pressed interrupt\n");
			stylus_press_x = stylus_marker_x;
			stylus_press_y = stylus_marker_y;
			/* set pen interrupt */
			avigo_irq |= (1<<6);
		}
		else
		{
			stylus_press_x = 0;
			stylus_press_y = 0;
		}
	}

	if ((changed & 0x02)!=0)
	{
		if ((current_input_port_data[3] & 0x02)!=0)
		{
			/* ????? causes a NMI */
			cpu_set_nmi_line(0, PULSE_LINE);
		}
	}

	for (i=0; i<4; i++)
	{
		previous_input_port_data[i] = current_input_port_data[i];
	}

	nx = readinputport(4);
	if (nx>=0x800) nx-=0x1000;
	else if (nx<=-0x800) nx+=0x1000;

	dx = nx - ox;
	ox = nx;

	ny = readinputport(5);
	if (ny>=0x800) ny-=0x1000;
	else if (ny<=-0x800) ny+=0x1000;

	dy = ny - oy;
	oy = ny;

	stylus_marker_x +=dx;
	stylus_marker_y +=dy;

	avigo_vh_set_stylus_marker_position(stylus_marker_x, stylus_marker_y);
#if 0
	/* not sure if keyboard generates an interrupt, or if something
	is plugged in for synchronisation! */
	/* not sure if this is correct! */
	for (i=0; i<2; i++)
	{
		int changed;
		int current;
		current = ~current_input_port_data[i];

		changed = ((current^(~previous_input_port_data[i])) & 0x07);

		if (changed!=0)
		{
			/* if there are 1 bits remaining, it means there is a bit
			that has changed, the old state was off and new state is on */
			if (current & changed)
			{
				avigo_irq |= (1<<2);
				break;
			}
		}
	}
#endif
	/* copy current to previous */
	memcpy(previous_input_port_data, current_input_port_data, sizeof(int)*4);

	/* refresh status of interrupts */
	avigo_refresh_ints();
}

/* does not do anything yet */
static void avigo_tc8521_alarm_int(int state)
{
//#if 0
	avigo_irq &=~(1<<5);
	
	if (state)
	{
		avigo_irq |= (1<<5);
	}

	avigo_refresh_ints();
//#endif
}


static struct tc8521_interface avigo_tc8521_interface =
{
	avigo_tc8521_alarm_int
};

static void avigo_refresh_memory(void)
{
        unsigned char *addr;

        switch (avigo_rom_bank_h)
        {
			/* 011 */
		  case 0x03:
          {
              avigo_flash_at_0x4000 = 1;
          }
          break;

			/* 101 */
          case 0x05:
          {
              avigo_flash_at_0x4000 = 2;
          }
          break;

          default:
              avigo_flash_at_0x4000 = 0;
              break;
        }

        addr = (unsigned char *)amd_flash_get_base(avigo_flash_at_0x4000);
        addr = addr + (avigo_rom_bank_l<<14);
        cpu_setbank(2, addr);
        cpu_setbank(6, addr);

        memory_set_bankhandler_r(2, 0, avigo_flash_0x4000_read_handler);
        memory_set_bankhandler_w(6, 0, avigo_flash_0x4000_write_handler);

        switch (avigo_ram_bank_h)
        {
				/* %101 */
                /* screen */
               case 0x06:
                {
                        memory_set_bankhandler_w(7, 0, avigo_vid_memory_w);
                        memory_set_bankhandler_r(3, 0, avigo_vid_memory_r);
                }
                break;

				/* %001 */
                /* ram */
                case 0x01:
                {
                        addr = avigo_memory + ((avigo_ram_bank_l & 0x07)<<14);

                        cpu_setbank(3, addr);
                        cpu_setbank(7, addr);

                        memory_set_bankhandler_w(7, 0, MWA_BANK7);
                        memory_set_bankhandler_r(3, 0, MRA_BANK3);
                }
                break;

				/* %111 */
                case 0x03:
                {
                        avigo_flash_at_0x8000 = 1;


                        addr = (unsigned char *)amd_flash_get_base(avigo_flash_at_0x8000);
                        addr = addr + (avigo_ram_bank_l<<14);
                        cpu_setbank(3, addr);
                        cpu_setbank(7, addr);

                        memory_set_bankhandler_r(3, 0, avigo_flash_0x8000_read_handler);
						memory_set_bankhandler_w(7,0, MWA_NOP);
						//   memory_set_bankhandler_w(7, 0, avigo_flash_0x8000_write_handler);



                }
                break;

                case 0x07:
                {
                        avigo_flash_at_0x8000 = 0;


                        addr = (unsigned char *)amd_flash_get_base(avigo_flash_at_0x8000);
                        addr = addr + (avigo_ram_bank_l<<14);
                        cpu_setbank(3, addr);
                        cpu_setbank(7, addr);

                        memory_set_bankhandler_r(3, 0, avigo_flash_0x8000_read_handler);
//                        memory_set_bankhandler_w(7, 0, avigo_flash_0x8000_write_handler);
						memory_set_bankhandler_w(7, 0, MWA_NOP);

                }
                break;

                default:
                  break;
        }

}



static void avigo_com_interrupt(int irq_num, int state)
{
        logerror("com int\r\n");

	avigo_irq &= ~(1<<3);

	if (state)
	{
		avigo_irq |= (1<<3);
	}

	avigo_refresh_ints();
}



static uart8250_interface avigo_com_interface[1]=
{
	{
		TYPE16550,
		1843200,
		avigo_com_interrupt,
		NULL,
		NULL
	},
};


void avigo_init_machine(void)
{
		int i;
	unsigned char *addr;

	/* initialise flash memory */
	amd_flash_init(0);

	amd_flash_init(1);
	
	amd_flash_init(2);

    /* install os into flash from rom image data */
	{
		/* get base of rom data */
		char *rom = (char *)memory_region(REGION_CPU1)+0x010000;
		char *flash;

		if (rom!=NULL)
		{
			/* copy first 1mb into first flash */
			flash = (char *)amd_flash_get_base(0);

			if (flash!=NULL)
			{
				memcpy(flash, rom, 0x0100000);
			}

			/* copy second 1mb into second flash */
			flash = (char *)amd_flash_get_base(1);

			if (flash!=NULL)
			{
				memcpy(flash, rom+0x0100000, 0x0150000-0x0100000);
			}
		}
	}


	stylus_marker_x = AVIGO_SCREEN_WIDTH>>1;
	stylus_marker_y = AVIGO_SCREEN_HEIGHT>>1;
	stylus_press_x = 0;
	stylus_press_y = 0;
	avigo_vh_set_stylus_marker_position(stylus_marker_x, stylus_marker_y);

    /* if these files exist, they will overwrite the os installed
	from the rom in the code above.
	
	This is ok. These files will be created by this driver and they will
    contain the OS data in them! Since these files are in the flash
	memory they can be written to. */

	
	amd_flash_restore(0, "avigof1.nv");
	amd_flash_reset(0);

    amd_flash_restore(1, "avigof2.nv");
    amd_flash_reset(1);

    amd_flash_restore(2, "avigof3.nv");
    amd_flash_reset(2);

	/* initialise settings for port data */
	for (i=0; i<4; i++)
	{
		previous_input_port_data[i] = readinputport(i);
	}

	/* a timer used to check status of pen */
	/* an interrupt is generated when the pen is pressed to the screen */
	avigo_dummy_timer = timer_pulse(TIME_IN_HZ(50), 0, avigo_dummy_timer_callback);

    memory_set_bankhandler_r(1, 0, MRA_BANK1);
    memory_set_bankhandler_r(2, 0, MRA_BANK2);
    memory_set_bankhandler_r(3, 0, MRA_BANK3);
    memory_set_bankhandler_r(4, 0, MRA_BANK4);

    memory_set_bankhandler_w(5, 0, MWA_BANK5);
    memory_set_bankhandler_w(6, 0, MWA_BANK6);
    memory_set_bankhandler_w(7, 0, MWA_BANK7);
    memory_set_bankhandler_w(8, 0, MWA_BANK8);

	avigo_irq = 0;
	avigo_rom_bank_l = 0;
	avigo_rom_bank_h = 0;
	avigo_ram_bank_l = 0;
	avigo_ram_bank_h = 0;
	avigo_flash_at_0x4000 = 0;
	avigo_flash_at_0x8000 = 0;

	/* real time clock */
	tc8521_init(&avigo_tc8521_interface);

	/* serial/uart */
	uart8250_init(0, avigo_com_interface);
	uart8250_reset(0);

	/* allocate memory */
	avigo_memory = (unsigned char *)malloc(128*1024);

	if (avigo_memory!=NULL)
	{
		memset(avigo_memory, 0, 128*1024);
	}

	addr = (unsigned char *)amd_flash_get_base(0);
	cpu_setbank(1, addr);
	cpu_setbank(5, addr);

        /* initialise fixed settings */
	memory_set_bankhandler_r(1, 0, avigo_flash_0x0000_read_handler);
	memory_set_bankhandler_w(5, 0, avigo_flash_0x0000_write_handler);

	addr = avigo_memory;
	cpu_setbank(4, addr);
	cpu_setbank(8, addr);

	memory_set_bankhandler_r(4, 0, avigo_ram_0xc000_read_handler);
	memory_set_bankhandler_w(8, 0, avigo_ram_0xc000_write_handler);


	/* 0x08000 is specially banked! */
	avigo_refresh_memory();
}

void avigo_shutdown_machine(void)
{
	/* store and free flash memory */
	amd_flash_store(0, "avigof1.nv");
	amd_flash_finish(0);

    amd_flash_store(1, "avigof2.nv");
    amd_flash_finish(1);

    amd_flash_store(2, "avigof3.nv");
    amd_flash_finish(2);

    tc8521_stop();

	/* free memory */
	if (avigo_memory!=NULL)
    {
		free(avigo_memory);
		avigo_memory = NULL;
    }

	/* free timer */
	if (avigo_dummy_timer!=NULL)
	{
		timer_remove(avigo_dummy_timer);
		avigo_dummy_timer = NULL;
	}


}


MEMORY_READ_START( readmem_avigo )
	{0x00000, 0x03fff, MRA_BANK1},
	{0x04000, 0x07fff, MRA_BANK2},
	{0x08000, 0x0bfff, MRA_BANK3},
	{0x0c000, 0x0ffff, MRA_BANK4},
MEMORY_END


MEMORY_WRITE_START( writemem_avigo )
	{0x00000, 0x03fff, MWA_BANK5},
	{0x04000, 0x07fff, MWA_BANK6},
	{0x08000, 0x0bfff, MWA_BANK7},
	{0x0c000, 0x0ffff, MWA_BANK8},
MEMORY_END

READ_HANDLER(avigo_key_data_read_r)
{
	UINT8 data;

	data = 0x007;

	if (avigo_key_line & 0x01)
	{
		data &= readinputport(0);
	}

	if (avigo_key_line & 0x02)
	{
		data &= readinputport(1);
	}

	if (avigo_key_line & 0x04)
	{
		data &= readinputport(2);

	}

	/* if bit 5 is clear shows synchronisation logo! */
	/* bit 3 must be set, otherwise there is an infinite loop in startup */
	data |= (1<<3) | (1<<5);

	return data;
}


/* set key line(s) to read */
/* bit 0 set for line 0, bit 1 set for line 1, bit 2 set for line 2 */
WRITE_HANDLER(avigo_set_key_line_w)
{
	/* 5, 101, read back 3 */
	avigo_key_line = data;
}

READ_HANDLER(avigo_irq_r)
{
	return avigo_irq;
}

WRITE_HANDLER(avigo_irq_w)
{
        avigo_irq &= ~data;

	avigo_refresh_ints();
}

READ_HANDLER(avigo_rom_bank_l_r)
{
	return avigo_rom_bank_l;
}

READ_HANDLER(avigo_rom_bank_h_r)
{
	return avigo_rom_bank_h;
}

READ_HANDLER(avigo_ram_bank_l_r)
{
	return avigo_ram_bank_l;
}

READ_HANDLER(avigo_ram_bank_h_r)
{
	return avigo_ram_bank_h;
}



WRITE_HANDLER(avigo_rom_bank_l_w)
{
	logerror("rom bank l w: %04x\n", data);

        avigo_rom_bank_l = data & 0x03f;

        avigo_refresh_memory();
}

WRITE_HANDLER(avigo_rom_bank_h_w)
{
	logerror("rom bank h w: %04x\n", data);


        /* 000 = flash 0
           001 = ram select
           011 = flash 1 (rom at ram - block 1 select)
           101 = flash 2
           110 = screen select?
           111 = flash 0 (rom at ram?)


        */
	avigo_rom_bank_h = data;


        avigo_refresh_memory();
}

WRITE_HANDLER(avigo_ram_bank_l_w)
{
	logerror("ram bank l w: %04x\n", data);

        avigo_ram_bank_l = data & 0x03f;

        avigo_refresh_memory();
}

WRITE_HANDLER(avigo_ram_bank_h_w)
{
	logerror("ram bank h w: %04x\n", data);

	avigo_ram_bank_h = data;

        avigo_refresh_memory();
}

READ_HANDLER(avigo_ad_control_status_r)
{
	logerror("avigo ad control read %02x\n",avigo_ad_control_status);

        return avigo_ad_control_status;
}

static unsigned int avigo_ad_value;

WRITE_HANDLER(avigo_ad_control_status_w)
{
	logerror("avigo ad control w %02x\n",data);

	if ((data & 0x070)==0x070)
	{
		/* bit 3 appears to select between 1 = x coord, 0 = y coord */
		/* when 6,5,4 = 1 */
		if ((data & 0x08)!=0)
		{
			logerror("a/d select x coordinate\n");
			logerror("x coord: %d\n",stylus_press_x);

			/* on screen range 0x060->0x03a0 */
			/* 832 is on-screen range */
			/* 5.2 a/d units per pixel */

			if (stylus_press_x!=0)
			{
				/* this might not be totally accurate because hitable screen
				area may include the border around the screen! */
				avigo_ad_value = ((int)(stylus_press_x * 5.2f))+0x060;
				avigo_ad_value &= 0x03fc;
			}
			else
			{
				avigo_ad_value = 0;
			}

			logerror("ad value: %d\n",avigo_ad_value);
			stylus_press_x = 0;

		}
		else
		{
			/* in the avigo rom, the y coordinate is inverted! */
			/* therefore a low value would be near the bottom of the display,
			and a high value at the top */
			
			/* total valid range 0x044->0x036a */
			/* 0x0350 is also checked */
			
			/* assumption 0x044->0x0350 is screen area and
			0x0350->0x036a is panel at bottom */
			
			/* 780 is therefore on-screen range */
			/* 3.25 a/d units per pixel */
			/* a/d unit * a/d range = total height */
			/* 3.25 * 1024.00 = 315.07 */
			
			logerror("a/d select y coordinate\n");
			logerror("y coord: %d\n",stylus_press_y);

			if (stylus_press_y!=0)
			{
				avigo_ad_value = 1024 - (((stylus_press_y)*3.25f) + 0x040);
			}
			else
			{
				avigo_ad_value = 0;
			}

			logerror("ad value: %d\n",avigo_ad_value);
			stylus_press_y = 0;
		}
	}

	/* bit 0: 1 if a/d complete, 0 if a/d not complete */
	avigo_ad_control_status = data | 1;
}

READ_HANDLER(avigo_ad_data_r)
{
	unsigned char data;

	data = 0;

	/* original */
	
	/* status AND	11110111 */
	/* status OR	01110000 -> C20F */
	
	switch (avigo_ad_control_status & 0x078)
	{
		/* x1110xxx */
		/* read upper 4 bits of 10 bit A/D number */
		case 0x070:
		case 0x078:
		{		
			/* upper 4 bits of 10 bit A/D number in bits 7-4 of data */
			/* bit 0 must be 0, bit 1 must be 0 */
			/* bit 3 must be 1. bit 2 can have any value */
	
			logerror("a/d read upper 4 bits\n");
			data = ((avigo_ad_value>>6) & 0x0f)<<4;
			data |= 8;
		}
		break;

		/* x0111xxx */
		case 0x038:
		{
			/* lower 6 bits of 10-bit A/D number in bits 7-2 of data */
			/* bit 0 must be 1, bit 1 must be 0 */

			logerror("a/d lower 6-bits\n");
			data = ((avigo_ad_value & 0x03f)<<2);
			data |= 1;
		}
		break;

		default:
			break;
	}

	/* x coord? */
	/* wait for bit 0 of status to become 1 */
	/* read data -> d */

	
	/* C20f AND 10111111 */
	/* C20f OR  00001000 */
	/* x0111xxx */

	/* bit 1 must be 0, bit 0 must be 1 */
	/* read data -> e */

	/* upper 4 bits of d contain data */
	/* bits 0 and 1 do not contain data of e, but all other bits do */

	/* get bit 5 and 6 of d */
	/* and put into bit 0 and 1 of e */

	/* C20f OR  01000000 */
	/* x1111xxx */

	/* y coord? */
	/* bit 0 must be 0, bit 1 must be 0 */
	/* bit 3 must be 1. bit 2 can have any value */
	/* read data -> d */

	/* C20f AND  10111111 */
	/* x0111xxx */

	/* bit 1 must be 0, bit 0 must be 1 */
	/* read data -> e */


	/* original and 1111100 */
	/* original or  1111000 */
	/* 1111x00 */



	/* if fails! */
	/* original */
	/* AND 1001100 */
	/* OR  1001000 */
	/* 1001x00 */


	/* AND 1101100 */
	/* OR  1101000 */
	/* 1101x00 */

	/* 1111x00 */

	logerror("avigo ad read %02x\n",data);

	return data;	
}


WRITE_HANDLER(avigo_speaker_w)
{
	UINT8 previous_speaker;

	previous_speaker = avigo_speaker_data;
	avigo_speaker_data = data;

	/* changed state? */
	if (((data^avigo_speaker_data) & (1<<3))!=0)
	{
		/* DAC output state */
		speaker_level_w(0,(data>>3) & 0x01);
	}
}

READ_HANDLER(avigo_unmapped_r)
{
	logerror("read unmapped port\n",offset);

	return 0x0ff;
}

/* port 0x04:

  bit 7: ??? if set, does a write 0x00 to 0x02e */

  /* port 0x029:
	port 0x02e */
READ_HANDLER(avigo_04_r)
{
	/* must be both 0 for it to boot! */
	return 0x0ff^((1<<7) | (1<<5));
}



PORT_READ_START( readport_avigo )
	{0x000, 0x000, avigo_unmapped_r},
    {0x001, 0x001, avigo_key_data_read_r},
	{0x002, 0x002, avigo_unmapped_r},
	{0x003, 0x003, avigo_irq_r},
	{0x004, 0x004, avigo_04_r},
	{0x005, 0x005, avigo_rom_bank_l_r},
	{0x006, 0x006, avigo_rom_bank_h_r},
	{0x007, 0x007, avigo_ram_bank_l_r},
	{0x008, 0x008, avigo_ram_bank_h_r},
    {0x009, 0x009, avigo_ad_control_status_r},
	{0x00a, 0x00f, avigo_unmapped_r},
	{0x010, 0x01f, tc8521_r},
	{0x020, 0x02c, avigo_unmapped_r},
    {0x02d, 0x02d, avigo_ad_data_r},
	{0x02e, 0x02f, avigo_unmapped_r},
	{0x030, 0x037, uart8250_0_r},
	{0x038, 0x0ff, avigo_unmapped_r},
PORT_END

PORT_WRITE_START( writeport_avigo )
	{0x001, 0x001, avigo_set_key_line_w},
	{0x003, 0x003, avigo_irq_w},
	{0x005, 0x005, avigo_rom_bank_l_w},
	{0x006, 0x006, avigo_rom_bank_h_w},
	{0x007, 0x007, avigo_ram_bank_l_w},
	{0x008, 0x008, avigo_ram_bank_h_w},
    {0x009, 0x009, avigo_ad_control_status_w},
   	{0x010, 0x01f, tc8521_w},
	{0x028, 0x028, avigo_speaker_w},
	{0x030, 0x037, uart8250_0_w},
PORT_END


INPUT_PORTS_START(avigo)
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "PAGE UP", KEYCODE_PGUP, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "PAGE DOWN", KEYCODE_PGDN, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "LIGHT", KEYCODE_L, IP_JOY_NONE)
	PORT_BIT (0x0f7, 0xf7, IPT_UNUSED)

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "TO DO", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "ADDRESS", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SCHEDULE", KEYCODE_S, IP_JOY_NONE)
	PORT_BIT (0x0f7, 0xf7, IPT_UNUSED)

	PORT_START
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "MEMO", KEYCODE_M, IP_JOY_NONE)
    PORT_BIT (0x0fe, 0xfe, IPT_UNUSED)

	PORT_START	
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Pen/Stylus pressed", KEYCODE_Q, JOYCODE_1_BUTTON1) 
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "?? Causes a NMI", KEYCODE_W, JOYCODE_1_BUTTON2) 		

	/* these two ports are used to emulate the position of the pen/stylus on the screen */
	/* a cursor is drawn to indicate the position, so when a click is done, it will occur in the correct place */
	PORT_START /* Mouse - X AXIS */ 
	PORT_ANALOGX( 0xfff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE ) 

	PORT_START /* Mouse - Y AXIS */
	PORT_ANALOGX( 0xfff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE ) 	
	
INPUT_PORTS_END

static struct Speaker_interface avigo_speaker_interface=
{
 1,
 {50},
};

static struct MachineDriver machine_driver_avigo =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
            CPU_Z80 ,  /* type */
            4000000, 
            readmem_avigo,                   /* MemoryReadAddress */
            writemem_avigo,                  /* MemoryWriteAddress */
            readport_avigo,                  /* IOReadPort */
            writeport_avigo,                 /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
                        0, 0,   /* every scanline */
		},
	},
        50,                                                     /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
        avigo_init_machine,                      /* init machine */
        avigo_shutdown_machine,
	/* video hardware */
        640, /* screen width */
        480,  /* screen height */
        {0, (640 - 1), 0, (480 - 1)},        /* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
        256,                                                        /* total colours */
        256,                                                        /* color table len */
        avigo_init_palette,                      /* init palette */

        VIDEO_TYPE_RASTER,                                  /* video attributes */
        0,                                                                 /* MachineLayer */
        avigo_vh_start,
        avigo_vh_stop,
        avigo_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
	{
		{
                SOUND_SPEAKER,
                &avigo_speaker_interface
        },
	}
};




/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START(avigo)
        ROM_REGION((64*1024)+0x0150000, REGION_CPU1,0)
        ROM_LOAD("avigo.rom", 0x010000, 0x0150000, 0x160ee4a6)
ROM_END


static const struct IODevice io_avigo[] =
{

	{IO_END}
};


/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMPX( 1997, avigo,   0,                avigo,  avigo,      0,       "Texas Instruments", "TI Avigo 100 PDA",GAME_NOT_WORKING)

