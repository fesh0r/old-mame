/*

    Fujitsu FM-Towns

    Japanese computer system released in 1989.

    CPU:  AMD 80386SX(DX?) (80387 available as add-on?)
    Sound:  Yamaha YM3438
            Ricoh RF5c68
    Video:  VGA + some custom extra video hardware
            320x200 - 640x480
            16 - 32768 colours from a possible palette of between 4096 and
              16.7m colours (depending on video mode)
            1024 sprites (16x16)


    Fujitsu FM-Towns Marty

    Japanese console, based on the FM-Towns computer, using an AMD 80386SX CPU,
    released in 1993


    16/5/09:  Skeleton driver.

    Issues: BIOS requires 386 protected mode.

*/

/* I/O port map (incomplete, could well be incorrect too)
 *
 * 0x0000   : Master 8259 PIC
 * 0x0002   : Master 8259 PIC
 * 0x0010   : Slave 8259 PIC
 * 0x0012   : Slave 8259 PIC
 * 0x0020 RW: bit 0 = soft reset (read/write), bit 6 = power off (write), bit 7 = NMI vector protect
 * 0x0022  W: bit 7 = power off (write)
 * 0x0025 R : returns 0x00? (read)
 * 0x0026 R : timer?
 * 0x0028 RW: bit 0 = NMI mask (read/write)
 * 0x0030 R : Machine ID (low)
 * 0x0031 R : Machine ID (high)
 * 0x0032 RW: bit 7 = RESET, bit 6 = CLK, bit 0 = data (serial ROM)
 * 0x0040   : 8253 PIT counter 0
 * 0x0042   : 8253 PIT counter 1
 * 0x0044   : 8253 PIT counter 2
 * 0x0046   : 8253 PIT mode port
 * 0x0060   : 8253 PIT ???
 * 0x006c RW: returns 0x00? (read) timer? (write)
 * 0x00a0-af: DMA controller 1 (uPD71071)
 * 0x00b0-bf: DMA controller 2 (uPD71071)
 * 0x0200-0f: Floppy controller (MB8877A)
 * 0x0400   : Video / CRTC (unknown)
 * 0x0404   : Enable whole first MB of RAM (disable VRAM, CMOS, BIOS ROM)
 * 0x0440-5f: Video / CRTC (unknown)
 * 0x0480 RW: bit 1 = disable BIOS ROM
 * 0x04c0-cf: CD-ROM controller (unknown? SCSI?)
 * 0x04d5   : Sound mute
 * 0x04d8   : YM3438 control port A / status
 * 0x04da   : YM3438 data port A / status
 * 0x04dc   : YM3438 control port B / status
 * 0x04de   : YM3438 data port B / status
 * 0x04e0-e3: volume ports
 * 0x04e9-ec: IRQ masks
 * 0x04f0-f8: RF5c68 registers
 * 0x05e8 R : RAM size in MB
 * 0x05ec RW: bit 0 = compatibility mode?
 * 0x0600 RW: Keyboard data port (8042)
 * 0x0602   : Keyboard control port (8042)
 * 0x0604   : (8042)
 * 0x3000 - 0x3fff : CMOS RAM
 * 0xfd90-a0: CRTC / Video
 * 0xff81: CRTC / Video - returns value in RAM location 0xcff81?
 * 
 * IRQ list
 * 
 * 		IRQ0 - PIT Timer IRQ
 * 		IRQ1 - Keyboard
 * 		IRQ2 - Serial Port
 * 		IRQ6 - Floppy Disc Drive
 * 		IRQ7 - PIC Cascade IRQ
 * 		IRQ8 - SCSI controller
 * 		IRQ9 - Built-in CD-ROM controller
 * 		IRQ11 - VSync interrupt
 * 		IRQ12 - Printer port
 * 		IRQ13 - Sound (FM?), Mouse
 * 		IRQ15 - PCM 
 * 
 * Machine ID list (I/O port 0x31)
 * 
	1(01h)	FM-TOWNS 1/2
	2(02h)	FM-TOWNS 1F/2F/1H/2H
	3(03h)	FM-TOWNS 10F/20F/40H/80H
	4(04h)	FM-TOWNSII UX
	5(05h)	FM-TOWNSII CX
	6(06h)	FM-TOWNSII UG
	7(07h)	FM-TOWNSII HR
	8(08h)	FM-TOWNSII HG
	9(09h)	FM-TOWNSII UR
	11(0Bh)	FM-TOWNSII MA
	12(0Ch)	FM-TOWNSII MX
	13(0Dh)	FM-TOWNSII ME
	14(0Eh)	TOWNS Application Card (PS/V Vision)
	15(0Fh)	FM-TOWNSII MF/Fresh/Fresh・TV
	16(10h)	FM-TOWNSII SN
	17(11h)	FM-TOWNSII HA/HB/HC
	19(13h)	FM-TOWNSII EA/Fresh・T/Fresh・ET/Fresh・FT
	20(14h)	FM-TOWNSII Fresh・E/Fresh・ES/Fresh・FS
	22(16h)	FMV-TOWNS H/Fresh・GS/Fresh・GT/H2
	23(17h)	FMV-TOWNS H20
	74(4Ah)	FM-TOWNS MARTY
 */

#include "driver.h"
#include "cpu/i386/i386.h"
#include "sound/2612intf.h"
#include "sound/rf5c68.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "formats/basicdsk.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "machine/upd71071.h"
#include "devices/messram.h"

static UINT8 ftimer;
static UINT8 nmi_mask;
static UINT8 compat_mode;
static UINT8 towns_system_port;
static UINT32 towns_ankcg_enable;
static UINT32 towns_mainmem_enable;
static UINT32 towns_ram_enable;
static UINT32* towns_vram;
static UINT8* towns_cmos;
static UINT8* towns_gfxvram;
static UINT8* towns_txtvram;
static UINT8 towns_vram_wplane;
static UINT8 towns_vram_rplane;
static UINT8 towns_palette_select;
static UINT8 towns_palette_r[256];
static UINT8 towns_palette_g[256];
static UINT8 towns_palette_b[256];
static UINT8 towns_degipal[8];
static UINT16 towns_kanji_offset;
static UINT8 towns_kanji_code_h;
static UINT8 towns_kanji_code_l;
static int towns_selected_drive;
static UINT8* towns_serial_rom;
static int towns_srom_position;
static UINT8 towns_srom_clk;
static UINT8 towns_srom_reset;
static UINT8 towns_rtc_select;
static UINT8 towns_rtc_data;
static UINT8 towns_rtc_reg[16];
static emu_timer* towns_rtc_timer;
static UINT8 towns_timer_mask;
static UINT8 towns_crtc_mix;
static UINT16 towns_machine_id;  // default is 0x0101


static void towns_update_video_banks(const address_space* space);
static PIC8259_SET_INT_LINE( towns_pic_irq );

static void towns_init_serial_rom(running_machine* machine)
{
	// TODO: init serial ROM contents
	int x;
	char code[8] = { 0x04,0x65,0x54,0xA4,0x95,0x45,0x35,0x5F };
	UINT8* srom = memory_region(machine,"serial");
	
	memset(towns_serial_rom,0,256/8);
	
	if(srom)
	{
		memcpy(towns_serial_rom,srom,32);
		towns_machine_id = (towns_serial_rom[0x18] << 8) | towns_serial_rom[0x17];
		popmessage("Machine ID in serial ROM: %04x",towns_machine_id);
		return;
	}

	for(x=8;x<=21;x++)
		towns_serial_rom[x] = 0xff;
	
	for(x=0;x<=7;x++)
	{
		towns_serial_rom[x] = code[x];
	}

	// add Machine ID
	towns_machine_id = 0x0101;
	towns_serial_rom[0x17] = 0x01;
	towns_serial_rom[0x18] = 0x01;

	// serial number?
	towns_serial_rom[29] = 0x10;
	towns_serial_rom[28] = 0x6e;
	towns_serial_rom[27] = 0x54;
	towns_serial_rom[26] = 0x32;
	towns_serial_rom[25] = 0x10;
}

static void towns_init_rtc(void)
{
	// for now, we'll just keep a static time stored here
	// seconds
	towns_rtc_reg[0] = 0;
	towns_rtc_reg[1] = 3;
	// minutes
	towns_rtc_reg[2] = 2;
	towns_rtc_reg[3] = 1;
	// hours
	towns_rtc_reg[4] = 8;
	towns_rtc_reg[5] = 0;
	// weekday
	towns_rtc_reg[6] = 2;
	// day
	towns_rtc_reg[7] = 1;
	towns_rtc_reg[8] = 1;
	// month
	towns_rtc_reg[9] = 6;
	towns_rtc_reg[10] = 0;
	// year
	towns_rtc_reg[11] = 9;
	towns_rtc_reg[12] = 0; 
}

static READ8_HANDLER(towns_system_r)
{
	UINT8 ret = 0;

	switch(offset)
	{
		case 0x00:
			logerror("SYS: port 0x20 read\n");
			return 0x00;
		case 0x05:
			logerror("SYS: port 0x25 read\n");
			return 0x00;
		case 0x06:
			//logerror("SYS: (0x26) timer read\n");
			ftimer -= 0x13;
			return ftimer;
		case 0x08:
			logerror("SYS: (0x28) NMI mask read\n");
			return nmi_mask & 0x01;
		case 0x10:
			logerror("SYS: (0x30) Machine ID read\n");
			return (towns_machine_id >> 8) & 0xff;
		case 0x11:
			logerror("SYS: (0x31) Machine ID read\n");
			return towns_machine_id & 0xff;
		case 0x12:
			/* Bit 0 = data, bit 6 = CLK, bit 7 = RESET, bit 5 is always 1? */
			ret = (towns_serial_rom[towns_srom_position/8] & (1 << (towns_srom_position%8))) ? 1 : 0;
			ret |= towns_srom_clk;
			ret |= towns_srom_reset;
			//logerror("SYS: (0x32) Serial ROM read [0x%02x, pos=%i]\n",ret,towns_srom_position);
			return ret;
		default:
			//logerror("SYS: Unknown system port read (0x%02x)\n",offset+0x20);
			return 0x00;
	}
}

static WRITE8_HANDLER(towns_system_w)
{
	switch(offset)
	{
		case 0x00:
			logerror("SYS: port 0x20 write %02x\n",data);
			break;
		case 0x02:
			logerror("SYS: (0x22) power port write %02x\n",data);
			break;
		case 0x08:
			logerror("SYS: (0x28) NMI mask write %02x\n",data);
			nmi_mask = data & 0x01;
			break;
		case 0x12:
			//logerror("SYS: (0x32) Serial ROM write %02x\n",data);
			// clocks on low-to-high transition
			if((data & 0x40) && towns_srom_clk == 0) // CLK
			{  // advance to next bit
				towns_srom_position++;
			}
			if((data & 0x80) && towns_srom_reset == 0) // reset
			{  // reset to beginning
				towns_srom_position = 0;
			}
			towns_srom_clk = data & 0x40;
			towns_srom_reset = data & 0x80;
			break;
		default:
			logerror("SYS: Unknown system port write 0x%02x (0x%02x)\n",data,offset);
	}
}

static READ8_HANDLER(towns_sys6c_r)
{
	logerror("SYS: (0x6c) Timer? read\n");
	return 0x00;
}

static WRITE8_HANDLER(towns_sys6c_w)
{
	logerror("SYS: (0x6c) write to timer (0x%02x)\n",data);
	ftimer -= 0x54;
}

static READ8_HANDLER(towns_dma1_r)
{
	const device_config* dev = devtag_get_device(space->machine,"dma_1");

	logerror("DMA#1: read register %i\n",offset);
	return upd71071_r(dev,offset);
}

static WRITE8_HANDLER(towns_dma1_w)
{
	const device_config* dev = devtag_get_device(space->machine,"dma_1");

	logerror("DMA#1: wrote 0x%02x to register %i\n",data,offset);
	upd71071_w(dev,offset,data);
}

static READ8_HANDLER(towns_dma2_r)
{
	const device_config* dev = devtag_get_device(space->machine,"dma_2");

	logerror("DMA#2: read register %i\n",offset);
	return upd71071_r(dev,offset);
}

static WRITE8_HANDLER(towns_dma2_w)
{
	const device_config* dev = devtag_get_device(space->machine,"dma_2");

	logerror("DMA#2: wrote 0x%02x to register %i\n",data,offset);
	upd71071_w(dev,offset,data);
}

/*
 *  Floppy Disc Controller (MB8877A)
 */

static WRITE_LINE_DEVICE_HANDLER( towns_mb8877a_drq_w )
{
	upd71071_dmarq(device, state, 0);
}

static READ8_HANDLER(towns_floppy_r)
{
	const device_config* fdc = devtag_get_device(space->machine,"fdc");

	switch(offset)
	{
		case 0x00:
			return wd17xx_status_r(fdc,offset/2);
		case 0x02:
			return wd17xx_track_r(fdc,offset/2);
		case 0x04:
			return wd17xx_sector_r(fdc,offset/2);
		case 0x06:
			return wd17xx_data_r(fdc,offset/2);
		case 0x08:  // selected drive status?
			//logerror("FDC: read from offset 0x08\n");
			if(towns_selected_drive < 1 || towns_selected_drive > 2)
				return 0x01;
			else
				return 0x07;
		case 0x0e: // DRVCHG
			logerror("FDC: read from offset 0x0e\n");
			return 0x00;
		default:
			logerror("FDC: read from invalid or unimplemented register %02x\n",offset);
	}
	return 0x00;
}

static WRITE8_HANDLER(towns_floppy_w)
{
	const device_config* fdc = devtag_get_device(space->machine,"fdc");

	switch(offset)
	{
		case 0x00:
			// Commands 0xd0 and 0xfe (Write Track) are apparently ignored?
			if(data == 0xd0)
				return;
			if(data == 0xfe)
				return;
			wd17xx_command_w(fdc,offset/2,data);
			break;
		case 0x02:
			wd17xx_track_w(fdc,offset/2,data);
			break;
		case 0x04:
			wd17xx_sector_w(fdc,offset/2,data);
			break;
		case 0x06:
			wd17xx_data_w(fdc,offset/2,data);
			break;
		case 0x08:
			// bit 5 - CLKSEL
			if(data & 0x10)
			{
				if(towns_selected_drive != 0 && towns_selected_drive < 2)
				{
					floppy_drive_set_motor_state(floppy_get_device(space->machine, towns_selected_drive-1), data & 0x10);
					floppy_drive_set_ready_state(floppy_get_device(space->machine, towns_selected_drive-1), data & 0x10,0);
				}
			}
			wd17xx_set_side(fdc,(data & 0x04)>>2);
			// bit 1 - DDEN
			// bit 0 - IRQMSK
			logerror("FDC: write %02x to offset 0x08\n",data);
			break;
		case 0x0c:  // drive select
			switch(data & 0x0f)
			{
				case 0x00:
					towns_selected_drive = 0;  // No drive selected
					break;
				case 0x01:
					towns_selected_drive = 1;
					wd17xx_set_drive(fdc,0);
					break;
				case 0x02:
					towns_selected_drive = 2;
					wd17xx_set_drive(fdc,1);
					break;
				case 0x04:
					towns_selected_drive = 3;
					wd17xx_set_drive(fdc,2);
					break;
				case 0x08:
					towns_selected_drive = 4;
					wd17xx_set_drive(fdc,3);
					break;
			}
			logerror("FDC: drive select %02x\n",data);
			break;
		default:
			logerror("FDC: write %02x to invalid or unimplemented register %02x\n",data,offset);
	}
}

static UINT16 towns_fdc_dma_r(running_machine* machine)
{
	const device_config* fdc = devtag_get_device(machine,"fdc");
	return wd17xx_data_r(fdc,0);
}

static void towns_fdc_dma_w(running_machine* machine, UINT16 data)
{
	const device_config* fdc = devtag_get_device(machine,"fdc");
	wd17xx_data_w(fdc,0,data);
}

static READ8_HANDLER(towns_video_440_r)
{
	logerror("VID: read port %04x\n",offset+0x440);
	return 0x00;
}

static WRITE8_HANDLER(towns_video_440_w)
{
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x440);
}

static READ8_HANDLER(towns_video_5c8_r)
{
	logerror("VID: read port %04x\n",offset+0x5c8);
	return 0x00;
}

static WRITE8_HANDLER(towns_video_5c8_w)
{
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x5c8);
}

/* Video/CRTC
 *
 * 0xfd90 - palette colour select
 * 0xfd92/4/6 - BRG value
 * 0xfd98-9f  - degipal(?)
 */
static READ8_HANDLER(towns_video_fd90_r)
{
	switch(offset)
	{
		case 0x00:
			return towns_palette_select;
		case 0x02:
			return towns_palette_b[towns_palette_select];
		case 0x04:
			return towns_palette_r[towns_palette_select];
		case 0x06:
			return towns_palette_g[towns_palette_select];
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			return towns_degipal[offset-0x08];
	}
//	logerror("VID: read port %04x\n",offset+0xfd90);
	return 0x00;
}

static WRITE8_HANDLER(towns_video_fd90_w)
{
	switch(offset)
	{
		case 0x00:
			towns_palette_select = data;
			break;
		case 0x02:
			towns_palette_b[towns_palette_select] = data;
			break;
		case 0x04:
			towns_palette_r[towns_palette_select] = data;
			break;
		case 0x06:
			towns_palette_g[towns_palette_select] = data;
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			towns_degipal[offset-0x08] = data;
			break;
	}
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0xfd90);
}

static READ8_HANDLER(towns_keyboard_r)
{
	logerror("KB: read offset %02x\n",offset);
	return 0x00;
}

static WRITE8_HANDLER(towns_keyboard_w)
{
	logerror("KB: wrote 0x%02x to offset %02x\n",data,offset);
}

/*
 *  Port 0x60 - PIT Timer control
 *  On read:	bit 0: Timer 0 output level
 * 				bit 1: Timer 1 output level
 * 				bits 4-2: Timer masks (timer 2 = sound)
 *  On write:	bits 2-0: Timer mask set
 * 				bit 7: Timer 0 output reset
 */
static READ8_HANDLER(towns_port60_r)
{
	UINT8 val = 0x00;
	
	if ( pit8253_get_output(devtag_get_device(space->machine, "pit"), 0 ) )
		val |= 0x01;
	if ( pit8253_get_output(devtag_get_device(space->machine, "pit"), 1 ) )
		val |= 0x02;

	val |= (towns_timer_mask & 0x07) << 2;
	
	logerror("PIT: port 0x60 read, returning 0x%02x\n",val);
	return val;
}

static WRITE8_HANDLER(towns_port60_w)
{
	const device_config* dev = devtag_get_device(space->machine,"pic8259_master");

	if(data & 0x80)
	{
		towns_pic_irq(dev,0);
	}
	towns_timer_mask = data & 0x07;
	// bit 2 = sound (beeper?)
	logerror("PIT: wrote 0x%02x to port 0x60\n",data);
}

static READ32_HANDLER(towns_sys5e8_r)
{
	switch(offset)
	{
		case 0x00:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: read RAM size port\n");
				return 0x06;  // 6MB is standard for the Marty
			}
			break;
		case 0x01:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: read port 5ec\n");
				return compat_mode & 0x01;
			}
			break;
	}
	return 0x00;
}

static WRITE32_HANDLER(towns_sys5e8_w)
{
	switch(offset)
	{
		case 0x00:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: wrote 0x%02x to port 5e8\n",data);
			}
			break;
		case 0x01:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: wrote 0x%02x to port 5ec\n",data);
				compat_mode = data & 0x01;
			}
			break;
	}
}

static READ32_HANDLER(towns_padport_r)
{
	if(ACCESSING_BITS_0_7)
		return 0x7f;

	return 0x00;
}

static READ8_HANDLER( towns_cmos8_r )
{
	return towns_cmos[offset];
}

static WRITE8_HANDLER( towns_cmos8_w )
{
	towns_cmos[offset] = data;
}

static READ8_HANDLER( towns_cmos_low_r )
{
	if(towns_mainmem_enable != 0)
		return messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset + 0xd8000];

	return towns_cmos[offset];
}

static WRITE8_HANDLER( towns_cmos_low_w )
{
	if(towns_mainmem_enable != 0)
		messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xd8000] = data;
	else
		towns_cmos[offset] = data;
}

static READ8_HANDLER( towns_cmos_r )
{
	return towns_cmos[offset];
}

static WRITE8_HANDLER( towns_cmos_w )
{
	towns_cmos[offset] = data;
}

static WRITE8_HANDLER( towns_gfx_w )
{
	if(towns_mainmem_enable != 0)
	{
		messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xc0000] = data;
		return;
	}
	if(towns_vram_wplane & 0x01)
		towns_gfxvram[offset] = data;
	if(towns_vram_wplane & 0x02)
		towns_gfxvram[offset + 0x8000] = data;
	if(towns_vram_wplane & 0x04)
		towns_gfxvram[offset + 0x10000] = data;
	if(towns_vram_wplane & 0x08)
		towns_gfxvram[offset + 0x18000] = data;
}

static READ8_HANDLER( towns_video_cff80_r )
{
	UINT8* ROM = memory_region(space->machine,"user");
	if(towns_mainmem_enable != 0)
		return messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xcff80];

	switch(offset)
	{
		case 0x00:  // mix register
			return towns_crtc_mix;
		case 0x01:  // read/write plane select (bit 0-3 write, bit 6-7 read)
			return ((towns_vram_rplane << 6) & 0xc0) | towns_vram_wplane;
		case 0x16:  // Kanji character data
			return ROM[(towns_kanji_offset << 1) + 0x180000];
		case 0x17:  // Kanji character data
			return ROM[(towns_kanji_offset++ << 1) + 0x180001];
		case 0x19:  // ANK CG ROM
			if(towns_ankcg_enable != 0)
				return 0x01;
			else
				return 0x00;
		default:
			logerror("VGA: read from invalid or unimplemented memory-mapped port %05x\n",0xcff80+offset*4);
	}

	return 0;
}

static WRITE8_HANDLER( towns_video_cff80_w )
{
	if(towns_mainmem_enable != 0)
	{
		messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xcff80] = data;
		return;
	}

	switch(offset)
	{
		case 0x00:  // mix register
			towns_crtc_mix = data;
			break;
		case 0x01:  // read/write plane select (bit 0-3 write, bit 6-7 read)
			towns_vram_wplane = data & 0x0f;
			towns_vram_rplane = (data & 0xc0) >> 6;
			towns_update_video_banks(space);
			logerror("VGA: VRAM wplane select = 0x%02x\n",towns_vram_wplane);
			break;
		case 0x14:  // Kanji offset (high)
			towns_kanji_code_h = data & 0x7f;
			break;
		case 0x15:  // Kanji offset (low)
			towns_kanji_code_l = data & 0x7f;
			// this is a little over the top...
			if(towns_kanji_code_h < 0x30)
			{
				towns_kanji_offset = ((towns_kanji_code_l & 0x1f) << 4)
				                   | (((towns_kanji_code_l - 0x20) & 0x20) << 8)
				                   | (((towns_kanji_code_l - 0x20) & 0x40) << 6)
				                   | ((towns_kanji_code_h & 0x07) << 9);
			}
			else if(towns_kanji_code_h < 0x70)
			{
				towns_kanji_offset = ((towns_kanji_code_l & 0x1f) << 4)
				                   + (((towns_kanji_code_l - 0x20) & 0x60) << 8)
				                   + ((towns_kanji_code_h & 0x0f) << 9)
				                   + (((towns_kanji_code_h - 0x30) & 0x70) * 0xc00)
				                   + 0x8000;
			}
			else
			{
				towns_kanji_offset = ((towns_kanji_code_l & 0x1f) << 4)
				                   | (((towns_kanji_code_l - 0x20) & 0x20) << 8)
				                   | (((towns_kanji_code_l - 0x20) & 0x40) << 6)
				                   | ((towns_kanji_code_h & 0x07) << 9)
				                   | 0x38000;
			}
			break;
		case 0x19:  // ANK CG ROM
			towns_ankcg_enable = data & 0x00000100;
			towns_update_video_banks(space);
			break;
		default:
			logerror("VGA: write %08x to invalid or unimplemented memory-mapped port %05x\n",data,0xcff80+offset);
	}
}

static void towns_update_video_banks(const address_space* space)
{
	UINT8* ROM;

	if(towns_mainmem_enable != 0)  // first MB is RAM
	{
		ROM = memory_region(space->machine,"user");

		memory_set_bankptr(space->machine,1,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xc0000);
		memory_set_bankptr(space->machine,2,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xc8000);
		memory_set_bankptr(space->machine,3,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xc9000);
		memory_set_bankptr(space->machine,4,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xca000);
		memory_set_bankptr(space->machine,5,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xca000);
		memory_set_bankptr(space->machine,10,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xca800);
		memory_set_bankptr(space->machine,6,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xcb000);
		memory_set_bankptr(space->machine,7,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xcb000);
		memory_set_bankptr(space->machine,8,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xcc000);
		if(towns_system_port & 0x02)
			memory_set_bankptr(space->machine,11,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xf8000);
		else
			memory_set_bankptr(space->machine,11,ROM+0x238000);
		memory_set_bankptr(space->machine,12,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xf8000);
		return;
	}
	else  // enable I/O ports and VRAM
	{
		ROM = memory_region(space->machine,"user");

		memory_set_bankptr(space->machine,1,towns_gfxvram+(towns_vram_rplane*0x8000));
		memory_set_bankptr(space->machine,2,towns_txtvram);
		memory_set_bankptr(space->machine,3,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xc9000);
		if(towns_ankcg_enable == 0)
			memory_set_bankptr(space->machine,4,ROM+0x180000+0x3d000);  // ANK CG 8x8
		else
			memory_set_bankptr(space->machine,4,towns_txtvram+0x2000);
		memory_set_bankptr(space->machine,5,towns_txtvram+0x2000);
		memory_set_bankptr(space->machine,10,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xca800);
		if(towns_ankcg_enable == 0)
			memory_set_bankptr(space->machine,6,ROM+0x180000+0x3d800);  // ANK CG 8x16
		else
			memory_set_bankptr(space->machine,6,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xcb000);
		memory_set_bankptr(space->machine,7,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xcb000);
		memory_set_bankptr(space->machine,8,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xcc000);
		if(towns_system_port & 0x02)
			memory_set_bankptr(space->machine,11,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xf8000);
		else
			memory_set_bankptr(space->machine,11,ROM+0x238000);
		memory_set_bankptr(space->machine,12,messram_get_ptr(devtag_get_device(space->machine, "messram"))+0xf8000);
		return;
	}
}

static READ8_HANDLER( towns_gfx_high_r )
{
	return towns_gfxvram[offset];
}

static WRITE8_HANDLER( towns_gfx_high_w )
{
	towns_gfxvram[offset] = data;
}

static READ8_HANDLER( towns_sys480_r )
{
	if(towns_system_port & 0x02)
		return 0x02;
	else
		return 0x00;
}

static WRITE8_HANDLER( towns_sys480_w )
{
	towns_system_port = data;
	towns_ram_enable = data & 0x02;
	towns_update_video_banks(space);
}

static WRITE32_HANDLER( towns_video_404_w )
{
	if(ACCESSING_BITS_0_7)
	{
		towns_mainmem_enable = data & 0x80;
		towns_update_video_banks(space);
	}
}

static READ32_HANDLER( towns_video_404_r )
{
	if(towns_mainmem_enable != 0)
		return 0x00000080;

	return 0;
}

static READ8_HANDLER(towns_video_ff81_r)
{
	return ((towns_vram_rplane << 6) & 0xc0) | towns_vram_wplane;
}

static WRITE8_HANDLER(towns_video_ff81_w)
{
	towns_vram_wplane = data & 0x0f;
	towns_vram_rplane = (data & 0xc0) >> 6;
	towns_update_video_banks(space);
	logerror("VGA: VRAM wplane select (I/O) = 0x%02x\n",towns_vram_wplane);
}

/*
 *  I/O ports 0x4c0-0x4cf
 *  CD-ROM driver (SCSI?)
 */
static READ8_HANDLER(towns_cdrom_r)
{
	switch(offset)
	{
		case 0x00:
			return 0x01;
		default:
			logerror("CD: read port %02x\n",offset*2);
			return 0x00;
	}
}

static WRITE8_HANDLER(towns_cdrom_w)
{
	switch(offset)
	{
		default:
			logerror("CD: read port %02x\n",offset*2);
	}
}

/* CMOS RTC
 * 0x70: Data port
 * 0x80: Register select
 */
static READ32_HANDLER(towns_rtc_r)
{
	if(ACCESSING_BITS_0_7)
		return 0x80 | towns_rtc_reg[towns_rtc_select];
		
	return 0x00;
}

static WRITE32_HANDLER(towns_rtc_w)
{
	if(ACCESSING_BITS_0_7)
		towns_rtc_data = data;;
}

static WRITE32_HANDLER(towns_rtc_select_w)
{
	if(ACCESSING_BITS_0_7)
	{
		if(!(data & 0x80))
		{
			if(data & 0x01)
				towns_rtc_select = towns_rtc_data & 0x0f;
		}
	}
}

static void rtc_hour(void)
{
	towns_rtc_reg[4]++;
	if(towns_rtc_reg[4] > 4 && towns_rtc_reg[5] == 2)
	{
		towns_rtc_reg[4] = 0;
		towns_rtc_reg[5] = 0;
	}
	else if(towns_rtc_reg[4] > 9)
	{
		towns_rtc_reg[4] = 0;
		towns_rtc_reg[5]++;
	}
}

static void rtc_minute(void)
{
	towns_rtc_reg[2]++;
	if(towns_rtc_reg[2] > 9)
	{
		towns_rtc_reg[2] = 0;
		towns_rtc_reg[3]++;
		if(towns_rtc_reg[3] > 5)
		{
			towns_rtc_reg[3] = 0;
			rtc_hour();
		}
	}
}

static TIMER_CALLBACK(rtc_second)
{
	// increase RTC time by one second
	towns_rtc_reg[0]++;
	if(towns_rtc_reg[0] > 9)
	{
		towns_rtc_reg[0] = 0;
		towns_rtc_reg[1]++;
		if(towns_rtc_reg[1] > 5)
		{
			towns_rtc_reg[1] = 0;
			rtc_minute();
		}
	}
}

static READ8_HANDLER(towns_unknown_r)
{
	return 0x08;
}

static READ8_HANDLER(towns_41ff_r)
{
	logerror("I/O port 0x41ff read\n");
	return 0x01;
}

static IRQ_CALLBACK( towns_irq_callback )
{
	const device_config* pic1 = devtag_get_device(device->machine,"pic8259_master");
	const device_config* pic2 = devtag_get_device(device->machine,"pic8259_slave");
	int r;
	
	r = pic8259_acknowledge(pic2);
	if(r == 0)
	{
		r = pic8259_acknowledge(pic1);
	}
	
	return r; 
}

static PIC8259_SET_INT_LINE( towns_pic_irq )
{
	cputag_set_input_line(device->machine,"maincpu",0,interrupt ? HOLD_LINE : CLEAR_LINE);
	logerror("PIC#1: set IRQ line to %i\n",interrupt);
}

static PIC8259_SET_INT_LINE( towns_slave_pic_irq )
{
	const device_config* dev = devtag_get_device(device->machine,"pic8259_master");

	pic8259_set_irq_line(dev,7,interrupt);
	logerror("PIC#2: set IRQ line to %i\n",interrupt);
}

static PIT8253_OUTPUT_CHANGED( towns_pit_out0_changed )
{
	const device_config* dev = devtag_get_device(device->machine,"pic8259_master");
	
	if(towns_timer_mask & 0x01)
	{
		pic8259_set_irq_line(dev,0,state);
	}
}

static PIT8253_OUTPUT_CHANGED( towns_pit_out1_changed )
{
	const device_config* dev = devtag_get_device(device->machine,"pic8259_master");
	
	if(towns_timer_mask & 0x02)
	{
		pic8259_set_irq_line(dev,0,state);
	}
}

static ADDRESS_MAP_START(towns_mem, ADDRESS_SPACE_PROGRAM, 32)
  // memory map based on FM-Towns/Bochs (Bochs modified to emulate the FM-Towns)
  // may not be (and probably is not) correct
  AM_RANGE(0x00000000, 0x000bffff) AM_RAM
  AM_RANGE(0x000c0000, 0x000c7fff) AM_READ(SMH_BANK(1))
  AM_RANGE(0x000c0000, 0x000c7fff) AM_WRITE8(towns_gfx_w,0xffffffff)
  AM_RANGE(0x000c8000, 0x000c8fff) AM_READWRITE(SMH_BANK(2),SMH_BANK(2))
  AM_RANGE(0x000c9000, 0x000c9fff) AM_READWRITE(SMH_BANK(3),SMH_BANK(3))
  AM_RANGE(0x000ca000, 0x000ca7ff) AM_READWRITE(SMH_BANK(4),SMH_BANK(5))
  AM_RANGE(0x000ca800, 0x000cafff) AM_READWRITE(SMH_BANK(10),SMH_BANK(10))
  AM_RANGE(0x000cb000, 0x000cbfff) AM_READWRITE(SMH_BANK(6),SMH_BANK(7))
  AM_RANGE(0x000cc000, 0x000cff7f) AM_READWRITE(SMH_BANK(8),SMH_BANK(8))
  AM_RANGE(0x000cff80, 0x000cffff) AM_READWRITE8(towns_video_cff80_r,towns_video_cff80_w,0xffffffff)
  AM_RANGE(0x000d0000, 0x000d7fff) AM_RAM
  AM_RANGE(0x000d8000, 0x000d9fff) AM_READWRITE8(towns_cmos_low_r,towns_cmos_low_w,0xffffffff) // CMOS? RAM
  AM_RANGE(0x000da000, 0x000effff) AM_RAM //READWRITE(SMH_BANK(11),SMH_BANK(11))
  AM_RANGE(0x000f0000, 0x000f7fff) AM_RAM //READWRITE(SMH_BANK(12),SMH_BANK(12))
  AM_RANGE(0x000f8000, 0x000fffff) AM_READWRITE(SMH_BANK(11),SMH_BANK(12))
  AM_RANGE(0x00100000, 0x005fffff) AM_RAM  // some extra RAM - seems to be needed to boot
  AM_RANGE(0x80000000, 0x8003ffff) AM_READWRITE8(towns_gfx_high_r,towns_gfx_high_w,0xffffffff) AM_MIRROR(0x1c0000) // VRAM
  AM_RANGE(0x81000000, 0x8101ffff) AM_RAM  // Sprite RAM
  AM_RANGE(0xc2000000, 0xc207ffff) AM_ROM AM_REGION("user",0x000000)  // OS ROM
  AM_RANGE(0xc2080000, 0xc20fffff) AM_ROM AM_REGION("user",0x100000)  // DIC ROM
  AM_RANGE(0xc2100000, 0xc213ffff) AM_ROM AM_REGION("user",0x180000)  // FONT ROM
  AM_RANGE(0xc2140000, 0xc2141fff) AM_READWRITE8(towns_cmos_r,towns_cmos_w,0xffffffff) // CMOS (mirror?)
  AM_RANGE(0xc2200000, 0xc2200fff) AM_NOP  // WAVE RAM
  AM_RANGE(0xfffc0000, 0xffffffff) AM_ROM AM_REGION("user",0x200000)  // SYSTEM ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( towns_io , ADDRESS_SPACE_IO, 32)
  // I/O ports derived from FM Towns/Bochs, these are specific to the FM Towns
  // Some common PC ports are likely to also be used
  // System ports
  AM_RANGE(0x0000,0x0003) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0x00ff00ff)
  AM_RANGE(0x0010,0x0013) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0x00ff00ff)
  AM_RANGE(0x0020,0x0033) AM_READWRITE8(towns_system_r,towns_system_w, 0xffffffff)
  AM_RANGE(0x0040,0x0047) AM_DEVREADWRITE8("pit",pit8253_r, pit8253_w, 0x00ff00ff)
  AM_RANGE(0x0060,0x0063) AM_READWRITE8(towns_port60_r, towns_port60_w, 0x000000ff)
  AM_RANGE(0x006c,0x006f) AM_READWRITE8(towns_sys6c_r,towns_sys6c_w, 0x000000ff)
  // 0x0070/0x0080 - CMOS RTC
  AM_RANGE(0x0070,0x0073) AM_READWRITE(towns_rtc_r,towns_rtc_w)
  AM_RANGE(0x0080,0x0083) AM_WRITE(towns_rtc_select_w)
  // DMA controllers (uPD71071)
  AM_RANGE(0x00a0,0x00af) AM_READWRITE8(towns_dma1_r, towns_dma1_w, 0xffffffff)
  AM_RANGE(0x00b0,0x00bf) AM_READWRITE8(towns_dma2_r, towns_dma2_w, 0xffffffff)
  // Floppy controller
  AM_RANGE(0x0200,0x020f) AM_READWRITE8(towns_floppy_r, towns_floppy_w, 0xffffffff)
  // CRTC / Video
  AM_RANGE(0x0400,0x0403) AM_NOP  // R/O (0x400)
  AM_RANGE(0x0404,0x0407) AM_READWRITE(towns_video_404_r, towns_video_404_w)  // R/W (0x404)
  AM_RANGE(0x0440,0x045f) AM_READWRITE8(towns_video_440_r, towns_video_440_w, 0xffffffff)
  // System port
  AM_RANGE(0x0480,0x0483) AM_READWRITE8(towns_sys480_r,towns_sys480_w,0x000000ff)  // R/W (0x480)
  // CD-ROM
  AM_RANGE(0x04c0,0x04cf) AM_READWRITE8(towns_cdrom_r,towns_cdrom_w,0x00ff00ff)
  // Joystick / Mouse ports(?)
  AM_RANGE(0x04d0,0x04d3) AM_READ(towns_padport_r)
  // Sound (YM3438 [FM], RF5c68 [PCM])
  AM_RANGE(0x04d4,0x04d7) AM_NOP  // R/W  -- (0x4d5) mute?
  AM_RANGE(0x04d8,0x04df) AM_DEVREADWRITE8("fm",ym3438_r,ym3438_w,0x00ff00ff)
  AM_RANGE(0x04e0,0x04e3) AM_NOP  // R/W  -- volume ports
  AM_RANGE(0x04e8,0x04ef) AM_NOP  // R/O  -- (0x4e9) FM IRQ flag (bit 0), PCM IRQ flag (bit 3)
                                  // (0x4ea) PCM IRQ mask
                                  // R/W  -- (0x4eb) PCM IRQ flag
                                  // W/O  -- (0x4ec) LED control
  AM_RANGE(0x04f0,0x04fb) AM_DEVWRITE8("pcm",rf5c68_w,0xffffffff)
  // CRTC / Video
  AM_RANGE(0x05c8,0x05cb) AM_READWRITE8(towns_video_5c8_r, towns_video_5c8_w, 0xffffffff)
  // System ports
  AM_RANGE(0x05e8,0x05ef) AM_READWRITE(towns_sys5e8_r, towns_sys5e8_w)
  // Keyboard (8042 MCU)
  AM_RANGE(0x0600,0x0607) AM_READWRITE8(towns_keyboard_r, towns_keyboard_w,0x00ff00ff)
  // No idea what this is currently...
  AM_RANGE(0x0c30,0x0c33) AM_READ8(towns_unknown_r,0x00ff0000)
  // CMOS
  AM_RANGE(0x3000,0x3fff) AM_READWRITE8(towns_cmos8_r, towns_cmos8_w,0x00ff00ff)
  // Something (MS-DOS wants this 0x41ff be 1
  AM_RANGE(0x41fc,0x41ff) AM_READ8(towns_41ff_r,0xff000000)
  // CRTC / Video (again)
  AM_RANGE(0xfd90,0xfda3) AM_READWRITE8(towns_video_fd90_r, towns_video_fd90_w, 0xffffffff)
  AM_RANGE(0xff80,0xff83) AM_READWRITE8(towns_video_ff81_r, towns_video_ff81_w, 0x0000ff00)

ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( towns )
INPUT_PORTS_END

static DRIVER_INIT( towns )
{
	towns_vram = auto_alloc_array(machine,UINT32,0x20000);
	towns_cmos = auto_alloc_array(machine,UINT8,0x2000);
	towns_gfxvram = auto_alloc_array(machine,UINT8,0x40000);
	towns_txtvram = auto_alloc_array(machine,UINT8,0x8000);
	towns_serial_rom = auto_alloc_array(machine,UINT8,256/8);
	towns_init_serial_rom(machine);
	towns_init_rtc();
	towns_rtc_timer = timer_alloc(machine,rtc_second,NULL);
	
	cpu_set_irq_callback(cputag_get_cpu(machine,"maincpu"), towns_irq_callback);
}

static DRIVER_INIT( marty )
{
	DRIVER_INIT_CALL(towns);
	towns_machine_id = 0x034a;
}

static MACHINE_RESET( towns )
{
	ftimer = 0x00;
	nmi_mask = 0x00;
	compat_mode = 0x00;
	towns_ankcg_enable = 0x00;
	towns_mainmem_enable = 0x00;
	towns_ram_enable = 0x00;
	towns_update_video_banks(cpu_get_address_space(cputag_get_cpu(machine,"maincpu"),ADDRESS_SPACE_PROGRAM));
	towns_vram_wplane = 0x00;
	timer_adjust_periodic(towns_rtc_timer,attotime_zero,0,ATTOTIME_IN_HZ(1));
}

static VIDEO_START( towns )
{
}

static VIDEO_UPDATE( towns )
{
	int x,y;
	UINT8 dat1,dat2,dat3,dat4;
	int bit;
	UINT8 colour;

	for(y=0;y<480;y++)
	{
		for(x=0;x<640;x++)
		{
			bit = 7 - (x & 0x07);
			dat1 = towns_gfxvram[(x/8) + (y*0x50)];
			dat2 = towns_gfxvram[(x/8) + (y*0x50) + 0x8000];
			dat3 = towns_gfxvram[(x/8) + (y*0x50) + 0x10000];
			dat4 = towns_gfxvram[(x/8) + (y*0x50) + 0x18000];
			colour = (dat1 & (1 << bit) ? 1 : 0)
				| (dat2 & (1 << bit) ? 2 : 0)
				| (dat3 & (1 << bit) ? 4 : 0)
				| (dat4 & (1 << bit) ? 8 : 0);
			*BITMAP_ADDR32(bitmap,y,x) =
				(towns_palette_r[colour] << 16)
				| (towns_palette_g[colour] << 8)
				| (towns_palette_b[colour]);
		}
	}

    return 0;
}

static const struct pit8253_config towns_pit8253_config =
{
	{
		{
			307200,
			towns_pit_out0_changed
		},
		{
			307200,
			towns_pit_out1_changed
		},
		{
			307200,
			NULL
		}
	}
};

static const struct pic8259_interface towns_pic8259_master_config =
{
	towns_pic_irq
};


static const struct pic8259_interface towns_pic8259_slave_config =
{
	towns_slave_pic_irq
};

static const wd17xx_interface towns_mb8877a_interface =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE("dma_1", towns_mb8877a_drq_w),
	{FLOPPY_0,FLOPPY_1,FLOPPY_2,FLOPPY_3}
};

static FLOPPY_OPTIONS_START( towns )
	FLOPPY_OPTION( fmt_bin, "bin", "BIN disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([77])
		SECTORS([8])
		SECTOR_LENGTH([1024])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config towns_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(towns),
	DO_NOT_KEEP_GEOMETRY
};

static const upd71071_intf towns_dma_config =
{
	"maincpu",
	1000000,
	{ towns_fdc_dma_r, 0, 0, 0 },
	{ towns_fdc_dma_w, 0, 0, 0 }
};

// for debugging
static const gfx_layout x1_chars_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,8,9,10,11,12,13,14,15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

/* decoded for debugging purpose, this will be nuked in the end... */
static GFXDECODE_START( towns )
	GFXDECODE_ENTRY( "user",   0x180000, x1_chars_16x16,  0, 0x100 ) //needs to be checked when the ROM will be redumped
GFXDECODE_END

static MACHINE_DRIVER_START( towns )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I386, 16000000)
    MDRV_CPU_PROGRAM_MAP(towns_mem)
    MDRV_CPU_IO_MAP(towns_io)

    MDRV_MACHINE_RESET(towns)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_GFXDECODE(towns)

    /* sound hardware */
    MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("fm", YM3438, 53693100 / 7) // actual clock speed unknown
//  MDRV_SOUND_CONFIG(ym3438_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("pcm", RF5C68, 2150000)  // actual clock speed unknown
//  MDRV_SOUND_CONFIG(rf5c68_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

    MDRV_PIT8253_ADD("pit",towns_pit8253_config)

	MDRV_PIC8259_ADD( "pic8259_master", towns_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", towns_pic8259_slave_config )

	MDRV_MB8877_ADD("fdc",towns_mb8877a_interface)
	MDRV_FLOPPY_4_DRIVES_ADD(towns_floppy_config)

	MDRV_UPD71071_ADD("dma_1",towns_dma_config)
	MDRV_UPD71071_ADD("dma_2",towns_dma_config)

    MDRV_VIDEO_START(towns)
    MDRV_VIDEO_UPDATE(towns)
		
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("6M")
MACHINE_DRIVER_END

/* ROM definitions */
ROM_START( fmtowns )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos.rom",  0x000000, 0x080000, CRC(112872ee) SHA1(57fd146478226f7f215caf63154c763a6d52165e) )
	ROM_LOAD("fmt_f20.rom",  0x080000, 0x080000, CRC(9f55a20c) SHA1(1920711cb66340bb741a760de187de2f76040b8c) )
	ROM_LOAD("fmt_dic.rom",  0x100000, 0x080000, CRC(82d1daa2) SHA1(7564020dba71deee27184824b84dbbbb7c72aa4e) )
	ROM_LOAD("fmt_fnt.rom",  0x180000, 0x040000, CRC(dd6fd544) SHA1(a216482ea3162f348fcf77fea78e0b2e4288091a) )
	ROM_LOAD("fmt_sys.rom",  0x200000, 0x040000, CRC(afe4ebcf) SHA1(4cd51de4fca9bd7a3d91d09ad636fa6b47a41df5) )
ROM_END

ROM_START( fmtownsa )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos_a.rom",  0x000000, 0x080000, CRC(22270e9f) SHA1(a7e97b25ff72b14121146137db8b45d6c66af2ae) )
	ROM_LOAD("fmt_f20_a.rom",  0x080000, 0x080000, CRC(75660aac) SHA1(6a521e1d2a632c26e53b83d2cc4b0edecfc1e68c) )
	ROM_LOAD("fmt_dic_a.rom",  0x100000, 0x080000, CRC(74b1d152) SHA1(f63602a1bd67c2ad63122bfb4ffdaf483510f6a8) )
	ROM_LOAD("fmt_fnt_a.rom",  0x180000, 0x040000, CRC(0108a090) SHA1(1b5dd9d342a96b8e64070a22c3a158ca419894e1) )
	ROM_LOAD("fmt_sys_a.rom",  0x200000, 0x040000, CRC(92f3fa67) SHA1(be21404098b23465d24c4201a81c96ac01aff7ab) )
ROM_END

ROM_START( fmtmarty )
  ROM_REGION32_LE( 0x400000, "user", 0)
	ROM_LOAD("mrom.m36",  0x000000, 0x200000, CRC(9c0c060c) SHA1(5721c5f9657c570638352fa9acac57fa8d0b94bd) )
	ROM_LOAD("mrom.m37",  0x200000, 0x180000, CRC(fb66bb56) SHA1(e273b5fa618373bdf7536495cd53c8aac1cce9a5) )
	ROM_CONTINUE(0x180000,0x40000)
	ROM_CONTINUE(0x200000,0x40000)
ROM_END

ROM_START( carmarty )
  ROM_REGION32_LE( 0x480000, "user", 0)
	ROM_LOAD("fmt_dos.rom",  0x000000, 0x080000, CRC(2bc2af96) SHA1(99cd51c5677288ad8ef711b4ac25d981fd586884) )
	ROM_LOAD("fmt_dic.rom",  0x100000, 0x080000, CRC(82d1daa2) SHA1(7564020dba71deee27184824b84dbbbb7c72aa4e) )
	ROM_LOAD("fmt_fnt.rom",  0x180000, 0x040000, CRC(dd6fd544) SHA1(a216482ea3162f348fcf77fea78e0b2e4288091a) )
	ROM_LOAD("fmt_sys.rom",  0x200000, 0x040000, CRC(e1ff7ce1) SHA1(e6c359177e4e9fb5bbb7989c6bbf6e95c091fd88) )
	ROM_LOAD("mar_ex0.rom",  0x280000, 0x080000, CRC(e248bfbd) SHA1(0ce89952a7901dd4d256939a6bc8597f87e51ae7) )
	ROM_LOAD("mar_ex1.rom",  0x300000, 0x080000, CRC(ab2e94f0) SHA1(4b3378c772302622f8e1139ed0caa7da1ab3c780) )
	ROM_LOAD("mar_ex2.rom",  0x380000, 0x080000, CRC(ce150ec7) SHA1(1cd8c39f3b940e03f9fe999ebcf7fd693f843d04) )
	ROM_LOAD("mar_ex3.rom",  0x400000, 0x080000, CRC(582fc7fc) SHA1(a77d8014e41e9ff0f321e156c0fe1a45a0c5e58e) )
  ROM_REGION( 0x20, "serial", 0)
    ROM_LOAD("mytowns.rom",  0x00, 0x20, CRC(bc58eba6) SHA1(483087d823c3952cc29bd827e5ef36d12c57ad49) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  	MACHINE     INPUT    INIT    CONFIG COMPANY      FULLNAME            FLAGS */
COMP( 1989, fmtowns,  0,    	0, 		towns, 		towns, 	 towns,  	 0,  	"Fujitsu",   "FM-Towns",		 GAME_NOT_WORKING)
COMP( 1989, fmtownsa, fmtowns,	0, 		towns, 		towns, 	 towns,  	 0,  	"Fujitsu",   "FM-Towns (alternate)", GAME_NOT_WORKING)
CONS( 1993, fmtmarty, 0,    	0, 		towns, 		towns, 	 marty,  	 0,  	"Fujitsu",   "FM-Towns Marty",	 GAME_NOT_WORKING)
CONS( 1994, carmarty, fmtmarty,	0, 		towns, 		towns, 	 towns,  	 0,  	"Fujitsu",   "FM-Towns Car Marty",	 GAME_NOT_WORKING)

