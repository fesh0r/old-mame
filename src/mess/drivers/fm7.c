/***********************************************************************************************

    Fujitsu Micro 7 (FM-7)

    12/05/2009 Skeleton driver.

    Computers in this series:

                 | Release |    Main CPU    |  Sub CPU  |              RAM              |
    =====================================================================================
    FM-8         | 1981-05 | M68A09 @ 1MHz  |  M6809    |    64K (main) + 48K (VRAM)    |
    FM-7         | 1982-11 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-NEW7      | 1984-05 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-77        | 1984-05 | M68B09 @ 2MHz  |  M68B09E  |  64/256K (main) + 48K (VRAM)  |
    FM-77AV      | 1985-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV20    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV20EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV40SX  | 1988-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |

    Note: FM-77AV dumps probably come from a FM-77AV40SX. Shall we confirm that both computers
    used the same BIOS components?

    memory map info from http://www.nausicaa.net/~lgreenf/fm7page.htm
    see also http://retropc.net/ryu/xm7/xm7.shtml


    Known issues:
     - Beeper is not implemented
     - Keyboard repeat is not implemented
     - Optional Kanji ROM use is not implemented
     - Other optional hardware is not implemented (RS232, Z80 card...)
     - FM-77AV and later aren't working (extra features not yet implemented)

************************************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/2203intf.h"
#include "sound/wave.h"
#include "sound/beep.h"

#include "devices/cassette.h"
#include "formats/fm7_cas.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "machine/ctronics.h"

#include "includes/fm7.h"

UINT8* fm7_video_ram;
UINT8* shared_ram;
static UINT8* fm7_boot_ram;  // Boot RAM (AV only)
UINT8 fm7_type;
static UINT8 irq_flags;  // active IRQ flags
static UINT8 irq_mask;  // IRQ mask
static emu_timer* fm7_timer;  // main timer, triggered every 2.0345ms
static emu_timer* fm7_subtimer;  // sub-CPU timer, triggered every 20ms
static emu_timer* fm7_keyboard_timer;
emu_timer* fm77av_vsync_timer;
static UINT8 basic_rom_en;
static UINT8 init_rom_en;  // AV only
static unsigned int key_delay;
static unsigned int key_repeat;
static UINT16 current_scancode;
static UINT32 key_data[4];
static UINT32 mod_data;
static UINT8 key_scan_mode;
static UINT8 break_flag;
static UINT8 fm7_psg_regsel;
static UINT8 psg_data;
static UINT8 fdc_side;
static UINT8 fdc_drive;
static UINT8 fdc_irq_flag;
static UINT8 fdc_drq_flag;
static UINT8 fm77av_ym_irq;
static UINT8 speaker_active;
static UINT16 fm7_kanji_address;


static struct key_encoder
{
	UINT8 buffer[12];
	UINT8 tx_count;
	UINT8 rx_count;
	UINT8 command_length;
	UINT8 answer_length;
	UINT8 latch;  // 0=ready to receive
	UINT8 ack;
	UINT8 position;
} fm7_encoder;

static struct mmr
{
	UINT8 bank_addr[8][16];
	UINT8 segment;
	UINT8 window_offset;
	UINT8 enabled;
	UINT8 mode;
} fm7_mmr;

extern struct fm7_video_flags fm7_video;

/* key scancode conversion table
 * The FM-7 expects different scancodes when shift,ctrl or graph is held, or
 * when kana is active.
 */
 // TODO: fill in shift,ctrl,graph and kana code
static const UINT16 fm7_key_list[0x60][7] =
{ // norm  shift ctrl  graph kana  sh.kana scan
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x01},  // ESC
	{0x31, 0x21, 0xf9, 0xf9, 0xc7, 0x00, 0x02},  // 1
	{0x32, 0x22, 0xfa, 0xfa, 0xcc, 0x00, 0x03},  // 2
	{0x33, 0x23, 0xfb, 0xfb, 0xb1, 0xa7, 0x04},
	{0x34, 0x24, 0xfc, 0xfc, 0xb3, 0xa9, 0x05},
	{0x35, 0x25, 0xf2, 0xf2, 0xb4, 0xaa, 0x06},
	{0x36, 0x26, 0xf3, 0xf3, 0xb5, 0xab, 0x07},
	{0x37, 0x27, 0xf4, 0xf4, 0xd4, 0xac, 0x08},
	{0x38, 0x28, 0xf5, 0xf5, 0xd5, 0xad, 0x09},
	{0x39, 0x29, 0xf6, 0xf6, 0xd6, 0xae, 0x0a},  // 9
	{0x30, 0x00, 0xf7, 0xf7, 0xdc, 0xa6, 0x0b},  // 0
	{0x2d, 0x3d, 0x1e, 0x8c, 0xce, 0x00, 0x0c},  // -
	{0x5e, 0x7e, 0x1c, 0x8b, 0xcd, 0x00, 0x0d},  // ^
	{0x5c, 0x7c, 0xf1, 0xf1, 0xb0, 0x00, 0x0e},  // Yen
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0f},  // Backspace
	{0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x10},  // Tab
	{0x71, 0x51, 0x11, 0xfd, 0xc0, 0x00, 0x11},  // Q
	{0x77, 0x57, 0x17, 0xf8, 0xc3, 0x00, 0x12},  // W
	{0x65, 0x45, 0x05, 0xe4, 0xb2, 0xa8, 0x13},  // E
	{0x72, 0x52, 0x12, 0xe5, 0xbd, 0x00, 0x14},
	{0x74, 0x54, 0x14, 0x9c, 0xb6, 0x00, 0x15},
	{0x79, 0x59, 0x19, 0x9d, 0xdd, 0x00, 0x16},
	{0x75, 0x55, 0x15, 0xf0, 0xc5, 0x00, 0x17},
	{0x69, 0x49, 0x09, 0xe8, 0xc6, 0x00, 0x18},
	{0x6f, 0x4f, 0x0f, 0xe9, 0xd7, 0x00, 0x19},
	{0x70, 0x50, 0x10, 0x8d, 0xbe, 0x00, 0x1a},  // P
	{0x40, 0x60, 0x00, 0x8a, 0xde, 0x00, 0x1b},  // @
	{0x5b, 0x7b, 0x1b, 0xed, 0xdf, 0xa2, 0x1c},  // [
	{0x0d, 0x0d, 0x00, 0x0d, 0x0d, 0x0d, 0x1d},  // Return
	{0x61, 0x41, 0x01, 0x95, 0xc1, 0x00, 0x1e},  // A
	{0x73, 0x53, 0x13, 0x96, 0xc4, 0x00, 0x1f},  // S

	{0x64, 0x44, 0x04, 0xe6, 0xbc, 0x00, 0x20},  // D
	{0x66, 0x46, 0x06, 0xe7, 0xca, 0x00, 0x21},
	{0x67, 0x47, 0x07, 0x9e, 0xb7, 0x00, 0x22},
	{0x68, 0x48, 0x08, 0x9f, 0xb8, 0x00, 0x23},
	{0x6a, 0x4a, 0x0a, 0xea, 0xcf, 0x00, 0x24},
	{0x6b, 0x4b, 0x0b, 0xeb, 0xc9, 0x00, 0x25},
	{0x6c, 0x4c, 0x0c, 0x8e, 0xd8, 0x00, 0x26},  // L
	{0x3b, 0x2b, 0x00, 0x99, 0xda, 0x00, 0x27},  // ;
	{0x3a, 0x2a, 0x00, 0x94, 0xb9, 0x00, 0x28},  // :
	{0x5d, 0x7d, 0x1d, 0xec, 0xd1, 0xa3, 0x29},  // ]
	{0x7a, 0x5a, 0x1a, 0x80, 0xc2, 0xaf, 0x2a},  // Z
	{0x78, 0x58, 0x18, 0x81, 0xbb, 0x00, 0x2b},  // X
	{0x63, 0x43, 0x03, 0x82, 0xbf, 0x00, 0x2c},  // C
	{0x76, 0x56, 0x16, 0x83, 0xcb, 0x00, 0x2d},
	{0x62, 0x42, 0x02, 0x84, 0xba, 0x00, 0x2e},
	{0x6e, 0x4e, 0x0e, 0x85, 0xd0, 0x00, 0x2f},
	{0x6d, 0x4d, 0x0d, 0x86, 0xd3, 0x00, 0x30},  // M
	{0x2c, 0x3c, 0x00, 0x87, 0xc8, 0xa4, 0x31},  // <
	{0x2e, 0x3e, 0x00, 0x88, 0xd9, 0xa1, 0x32},  // >
	{0x2f, 0x3f, 0x00, 0x97, 0xd2, 0xa5, 0x33},  // /
	{0x22, 0x5f, 0x1f, 0xe0, 0xdb, 0x00, 0x34},  // "
	{0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x35},  // Space
	{0x2a, 0x2a, 0x00, 0x98, 0x2a, 0x2a, 0x36},  // Tenkey
	{0x2f, 0x2f, 0x00, 0x91, 0x2f, 0x2f, 0x37},
	{0x2b, 0x2b, 0x00, 0x99, 0x2b, 0x2b, 0x38},
	{0x2d, 0x2d, 0x00, 0xee, 0x2d, 0x2d, 0x39},
	{0x37, 0x37, 0x00, 0xe1, 0x37, 0x37, 0x3a},
	{0x38, 0x38, 0x00, 0xe2, 0x38, 0x38, 0x3b},
	{0x39, 0x39, 0x00, 0xe3, 0x39, 0x39, 0x3c},
	{0x3d, 0x3d, 0x00, 0xef, 0x3d, 0x3d, 0x3d},  // Tenkey =
	{0x34, 0x34, 0x00, 0x93, 0x34, 0x34, 0x3e},
	{0x35, 0x35, 0x00, 0x8f, 0x35, 0x35, 0x3f},

	{0x36, 0x36, 0x00, 0x92, 0x36, 0x36, 0x40},
	{0x2c, 0x2c, 0x00, 0x00, 0x2c, 0x2c, 0x41},
	{0x31, 0x31, 0x00, 0x9a, 0x31, 0x31, 0x42},
	{0x32, 0x32, 0x00, 0x90, 0x32, 0x32, 0x43},
	{0x33, 0x33, 0x00, 0x9b, 0x33, 0x33, 0x44},
	{0x0d, 0x0d, 0x00, 0x0d, 0x0d, 0x0d, 0x45},
	{0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x46},
	{0x2e, 0x2e, 0x00, 0x2e, 0x2e, 0x2e, 0x47},
	{0x12, 0x12, 0x00, 0x12, 0x12, 0x12, 0x48}, // INS
	{0x05, 0x05, 0x00, 0x05, 0x05, 0x05, 0x49},  // EL
	{0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x0c, 0x4a},  // CLS
	{0x7f, 0x7f, 0x00, 0x7f, 0x7f, 0x7f, 0x4b},  // DEL
	{0x11, 0x11, 0x00, 0x11, 0x11, 0x11, 0x4c},  // DUP
	{0x1e, 0x19, 0x00, 0x1e, 0x1e, 0x19, 0x4d},  // Cursor Up
	{0x0b, 0x0b, 0x00, 0x0b, 0x0b, 0x0b, 0x4e},  // HOME
	{0x1d, 0x02, 0x00, 0x1d, 0x1d, 0x02, 0x4f},  // Cursor Left
	{0x1f, 0x1a, 0x00, 0x1f, 0x1f, 0x1a, 0x50},  // Cursor Down
	{0x1c, 0x06, 0x00, 0x1c, 0x1c, 0x16, 0x51},  // Cursor Right
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c},  // BREAK
	{0x101, 0x00, 0x101, 0x101, 0x101, 0x00, 0x5d},  // PF1
	{0x102, 0x00, 0x102, 0x102, 0x102, 0x00, 0x5e},
	{0x103, 0x00, 0x103, 0x103, 0x103, 0x00, 0x5f},
	{0x104, 0x00, 0x104, 0x104, 0x104, 0x00, 0x60},
	{0x105, 0x00, 0x105, 0x105, 0x105, 0x00, 0x61},
	{0x106, 0x00, 0x106, 0x106, 0x106, 0x00, 0x62},
	{0x107, 0x00, 0x107, 0x107, 0x107, 0x00, 0x63},
	{0x108, 0x00, 0x108, 0x108, 0x108, 0x00, 0x64},
	{0x109, 0x00, 0x109, 0x109, 0x109, 0x00, 0x65},
	{0x10a, 0x00, 0x10a, 0x10a, 0x10a, 0x00, 0x66},  // PF10
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};


static void main_irq_set_flag(running_machine* machine, UINT8 flag)
{
	irq_flags |= flag;

	if(irq_flags != 0)
		cputag_set_input_line(machine,"maincpu",M6809_IRQ_LINE,ASSERT_LINE);
}

static void main_irq_clear_flag(running_machine* machine, UINT8 flag)
{
	irq_flags &= ~flag;

	if(irq_flags == 0)
		cputag_set_input_line(machine,"maincpu",M6809_IRQ_LINE,CLEAR_LINE);
}


/*
 * Main CPU: I/O port 0xfd02
 *
 * On read: returns cassette data (bit 7) and printer status (bits 0-5)
 * On write: sets IRQ masks
 *   bit 0 - keypress
 *   bit 1 - printer
 *   bit 2 - timer
 *   bit 3 - not used
 *   bit 4 - MFD
 *   bit 5 - TXRDY
 *   bit 6 - RXRDY
 *   bit 7 - SYNDET
 *
 */
static WRITE8_HANDLER( fm7_irq_mask_w )
{
	irq_mask = data;
	logerror("IRQ mask set: 0x%02x\n",irq_mask);
}

/*
 * Main CPU: I/O port 0xfd03
 *
 * On read: returns which IRQ is currently active (typically read by IRQ handler)
 *   bit 0 - keypress
 *   bit 1 - printer
 *   bit 2 - timer
 *   bit 3 - ???
 * On write: Buzzer/Speaker On/Off
 *   bit 0 - speaker on/off
 *   bit 6 - buzzer on for 205ms
 *   bit 7 - buzzer on/off
 */
static READ8_HANDLER( fm7_irq_cause_r )
{
	UINT8 ret = ~irq_flags;

	// Timer and Printer IRQ flags are cleared when this port is read
	// Keyboard IRQ flag is cleared when the scancode is read from
	// either keyboard data port (main CPU 0xfd01 or sub CPU 0xd401)
	if(irq_flags & 0x04)
		main_irq_clear_flag(space->machine,IRQ_FLAG_TIMER);
	if(irq_flags & 0x02)
		main_irq_clear_flag(space->machine,IRQ_FLAG_PRINTER);

	logerror("IRQ flags read: 0x%02x\n",ret);
	return ret;
}

static TIMER_CALLBACK( fm7_beeper_off )
{
	beep_set_state(devtag_get_device(machine,"beeper"),0);
	logerror("timed beeper off\n");
}

static WRITE8_HANDLER( fm7_beeper_w )
{
	speaker_active = data & 0x01;

	if(!speaker_active)  // speaker not active, disable all beeper sound
	{
		beep_set_state(devtag_get_device(space->machine,"beeper"),0);
		return;
	}

	if(data & 0x80)
	{
		if(speaker_active)
			beep_set_state(devtag_get_device(space->machine,"beeper"),1);
	}
	else
		beep_set_state(devtag_get_device(space->machine,"beeper"),0);

	if(data & 0x40)
	{
		if(speaker_active)
		{
			beep_set_state(devtag_get_device(space->machine,"beeper"),1);
			logerror("timed beeper on\n");
			timer_set(space->machine,ATTOTIME_IN_MSEC(205),NULL,0,fm7_beeper_off);
		}
	}
	logerror("beeper state: %02x\n",data);
}


/*
 *  Sub CPU: port 0xd403 (read-only)
 *  On read: timed buzzer sound
 */
READ8_HANDLER( fm7_sub_beeper_r )
{
	if(speaker_active)
	{
		beep_set_state(devtag_get_device(space->machine,"beeper"),1);
		logerror("timed beeper on\n");
		timer_set(space->machine,ATTOTIME_IN_MSEC(205),NULL,0,fm7_beeper_off);
	}
	return 0xff;
}

static READ8_HANDLER( vector_r )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	UINT8* ROM = memory_region(space->machine,"init");

	if(init_rom_en)
		return ROM[0x1ff0+offset];
	else
	{
		if(fm7_type == SYS_FM7)
			return RAM[0xfff0+offset];
		else
			return RAM[0x3fff0+offset];
	}
}

static WRITE8_HANDLER( vector_w )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");

	if(fm7_type == SYS_FM7)
		RAM[0xfff0+offset] = data;
	else
		RAM[0x3fff0+offset] = data;
}

/*
 * Main CPU: I/O port 0xfd04
 *
 *  bit 0 - attention IRQ active, clears flag when read.
 *  bit 1 - break key active
 */
static READ8_HANDLER( fm7_fd04_r )
{
	UINT8 ret = 0xff;

	if(fm7_video.attn_irq != 0)
	{
		ret &= ~0x01;
		fm7_video.attn_irq = 0;
	}
	if(break_flag != 0)
	{
		ret &= ~0x02;
	}
	return ret;
}

/*
 *  Main CPU: I/O port 0xfd0f
 *
 *  On read, enables BASIC ROM at 0x8000 (default)
 *  On write, disables BASIC ROM, enables RAM (if more than 32kB)
 */
static READ8_HANDLER( fm7_rom_en_r )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");

	basic_rom_en = 1;
	if(fm7_type == SYS_FM7)
	{
		memory_install_readwrite8_handler(space,0x8000,0xfbff,0,0,SMH_BANK(1),SMH_NOP);
		memory_set_bankptr(space->machine,1,RAM+0x38000);
	}
	else
		fm7_mmr_refresh(space);
	logerror("BASIC ROM enabled\n");
	return 0x00;
}

static WRITE8_HANDLER( fm7_rom_en_w )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");

	basic_rom_en = 0;
	if(fm7_type == SYS_FM7)
	{
		memory_install_readwrite8_handler(space,0x8000,0xfbff,0,0,SMH_BANK(1),SMH_BANK(1));
		memory_set_bankptr(space->machine,1,RAM+0x8000);
	}
	else
		fm7_mmr_refresh(space);
	logerror("BASIC ROM disabled\n");
}

/*
 *  Main CPU: port 0xfd10
 *  Initiate ROM enable. (FM-77AV and later only)
 *  Port is write-only.  Initiate ROM is on by default.
 *
 */
static WRITE8_HANDLER( fm7_init_en_w )
{
	if(data & 0x02)
	{
		init_rom_en = 0;
		fm7_mmr_refresh(space);
	}
	else
	{
		init_rom_en = 1;
		fm7_mmr_refresh(space);
	}
}

/*
 *  Main CPU: I/O ports 0xfd18 - 0xfd1f
 *  Floppy Disk Controller (MB8877A)
 */
static WRITE_LINE_DEVICE_HANDLER( fm7_fdc_intrq_w )
{
	fdc_irq_flag = state;
}

static WRITE_LINE_DEVICE_HANDLER( fm7_fdc_drq_w )
{
	fdc_drq_flag = state;
}

static READ8_HANDLER( fm7_fdc_r )
{
	const device_config* dev = devtag_get_device(space->machine,"fdc");
	UINT8 ret = 0;

	switch(offset)
	{
		case 0:
			return wd17xx_status_r(dev,offset);
		case 1:
			return wd17xx_track_r(dev,offset);
		case 2:
			return wd17xx_sector_r(dev,offset);
		case 3:
			return wd17xx_data_r(dev,offset);
		case 4:
			return fdc_side | 0xfe;
		case 5:
			return fdc_drive;
		case 6:
			// FM-7 always returns 0xff for this register
			return 0xff;
		case 7:
			if(fdc_irq_flag != 0)
				ret |= 0x40;
			if(fdc_drq_flag != 0)
				ret |= 0x80;
			return ret;
	}
	logerror("FDC: read from 0x%04x\n",offset+0xfd18);

	return 0x00;
}

static WRITE8_HANDLER( fm7_fdc_w )
{
	const device_config* dev = devtag_get_device(space->machine,"fdc");
	switch(offset)
	{
		case 0:
			wd17xx_command_w(dev,offset,data);
			break;
		case 1:
			wd17xx_track_w(dev,offset,data);
			break;
		case 2:
			wd17xx_sector_w(dev,offset,data);
			break;
		case 3:
			wd17xx_data_w(dev,offset,data);
			break;
		case 4:
			fdc_side = data & 0x01;
			wd17xx_set_side(dev,data & 0x01);
			logerror("FDC: wrote %02x to 0x%04x (side)\n",data,offset+0xfd18);
			break;
		case 5:
			fdc_drive = data;
			if((data & 0x03) > 0x01)
			{
				fdc_drive = 0;
			}
			else
			{
				wd17xx_set_drive(dev,data & 0x03);
				floppy_drive_set_motor_state(floppy_get_device(space->machine, data & 0x03), data & 0x80);
				floppy_drive_set_ready_state(floppy_get_device(space->machine, data & 0x03), data & 0x80,0);
				logerror("FDC: wrote %02x to 0x%04x (drive)\n",data,offset+0xfd18);
			}
			break;
		case 6:
			// FM77AV and later only. FM-7 returns 0xff;
			// bit 6 = 320k(1)/640k(0) FDD
			// bits 2,3 = logical drive
			logerror("FDC: mode write - %02x\n",data);
			break;
		default:
			logerror("FDC: wrote %02x to 0x%04x\n",data,offset+0xfd18);
	}
}

/*
 *  Main CPU: I/O ports 0xfd00-0xfd01
 *  Sub CPU: I/O ports 0xd400-0xd401
 *
 *  The scancode of the last key pressed is stored in fd/d401, with the 9th
 *  bit (MSB) in bit 7 of fd/d400.  0xfd00 also holds a flag for the main
 *  CPU clock speed in bit 0 (0 = 1.2MHz, 1 = 2MHz)
 *  Clears keyboard IRQ flag
 */
static READ8_HANDLER( fm7_keyboard_r)
{
	UINT8 ret;
	switch(offset)
	{
		case 0:
			ret = (current_scancode >> 1) & 0x80;
			ret |= 0x01; // 1 = 2MHz, 0 = 1.2MHz
			return ret;
		case 1:
			main_irq_clear_flag(space->machine,IRQ_FLAG_KEY);
			return current_scancode & 0xff;
		default:
			return 0x00;
	}
}

READ8_HANDLER( fm7_sub_keyboard_r)
{
	UINT8 ret;
	switch(offset)
	{
		case 0:
			ret = (current_scancode >> 1) & 0x80;
			return ret;
		case 1:
			main_irq_clear_flag(space->machine,IRQ_FLAG_KEY);
			return current_scancode & 0xff;
		default:
			return 0x00;
	}
}

/*
 *  Sub CPU: port 0xd431, 0xd432
 *  Keyboard encoder
 *
 *  d431 (R/W): Data register (8 bit)
 *  d432 (R/O): Status register
 *              bit 0 - ACK
 *              bit 7 - LATCH (0 if ready to receive)
 *
 *  Encoder commands:
 *      00 xx    : Set scancode format (FM-7, FM16B(?), Scan(Make/Break))
 *      01       : Get scancode format
 *      02 xx    : Set LED status
 *      03       : Get LED status
 *      04 xx    : Enable/Disable key repeat
 *      05 xx xx : Set repeat rate and time
 *      80 00    : Get RTC
 *      80 01 xx xx xx xx xx xx xx : Set RTC
 *      81 xx    : Video digitise
 *      82 xx    : Set video mode(?)
 *      83       : Get video mode(?)
 *      84 xx    : Video brightness (monitor?)
 *
 *  ACK is received after 5us.
 */
READ8_HANDLER( fm77av_key_encoder_r )
{
	UINT8 ret = 0xff;
	switch(offset)
	{
		case 0x00:  // data register
			if(fm7_encoder.rx_count > 0)
			{
				ret = fm7_encoder.buffer[fm7_encoder.position];
				fm7_encoder.position++;
				fm7_encoder.rx_count--;
				fm7_encoder.latch = 0;
			}
			if(fm7_encoder.rx_count > 0)
				fm7_encoder.latch = 1;  // more data to receive
			break;
		case 0x01:  // status register
			if(fm7_encoder.latch != 0)
				ret &= ~0x80;
			if(fm7_encoder.ack == 0)
				ret &= ~0x01;
			break;
	}

	return ret;
}

static void fm77av_encoder_setup_command(void)
{
	switch(fm7_encoder.buffer[0])
	{
		case 0:  // set scancode format
			fm7_encoder.tx_count = 2;
			break;
		case 1:  // get scancode format
			fm7_encoder.tx_count = 1;
			break;
		case 2:  // set LED
			fm7_encoder.tx_count = 2;
			break;
		case 3:  // get LED
			fm7_encoder.tx_count = 1;
			break;
		case 4:  // enable repeat
			fm7_encoder.tx_count = 2;
			break;
		case 5:  // set repeat rate
			fm7_encoder.tx_count = 3;
			break;
		case 0x80:  // get/set RTC (at least two bytes, 9 if byte two = 0x01)
			fm7_encoder.tx_count = 2;
			break;
		case 0x81:  // digitise
			fm7_encoder.tx_count = 2;
			break;
		case 0x82:  // set screen mode
			fm7_encoder.tx_count = 2;
			break;
		case 0x83:  // get screen mode
			fm7_encoder.tx_count = 1;
			break;
		case 0x84:  // set monitor brightness
			fm7_encoder.tx_count = 2;
			break;
		default:
			fm7_encoder.tx_count = 0;
			fm7_encoder.rx_count = 0;
			fm7_encoder.position = 0;
			logerror("ENC: Unknown command 0x%02x sent, ignoring\n",fm7_encoder.buffer[0]);
	}
}

static TIMER_CALLBACK( fm77av_encoder_ack )
{
	fm7_encoder.ack = 1;
}

static void fm77av_encoder_handle_command(void)
{
	switch(fm7_encoder.buffer[0])
	{
		case 0:  // set keyboard scancode mode
			key_scan_mode = fm7_encoder.buffer[1];
			fm7_encoder.rx_count = 0;
			logerror("ENC: Keyboard set to mode %i\n",fm7_encoder.buffer[1]);
			break;
		case 1:  // get keyboard scancode mode
			fm7_encoder.buffer[0] = key_scan_mode;
			fm7_encoder.rx_count = 1;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
		case 2:  // set LEDs
			fm7_encoder.rx_count = 0;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
		case 3:  // get LEDs
			fm7_encoder.rx_count = 1;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
		case 4:  // enable key repeat
			fm7_encoder.rx_count = 0;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
		case 5:  // set key repeat rate
			key_repeat = fm7_encoder.buffer[2] * 10;
			key_delay = fm7_encoder.buffer[1] * 10;
			fm7_encoder.rx_count = 0;
			logerror("ENC: Keyboard repeat rate set to %i/%i\n",fm7_encoder.buffer[1],fm7_encoder.buffer[2]);
			break;
		case 0x80:  // get/set RTC
			if(fm7_encoder.buffer[1] == 0x01)
				fm7_encoder.rx_count = 0;
			else
				fm7_encoder.rx_count = 7;
			logerror("ENC: Command %02x %02x recieved\n",fm7_encoder.buffer[0],fm7_encoder.buffer[1]);
			break;
		case 0x81:  // digitise
			fm7_encoder.rx_count = 0;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
		case 0x82:  // set screen mode
			fm7_encoder.rx_count = 0;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
		case 0x83:  // get screen mode
			fm7_encoder.rx_count = 1;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
		case 0x84:  // set monitor brightness
			fm7_encoder.rx_count = 0;
			logerror("ENC: Command %02x recieved\n",fm7_encoder.buffer[0]);
			break;
	}
	fm7_encoder.position = 0;
}

WRITE8_HANDLER( fm77av_key_encoder_w )
{
	fm7_encoder.ack = 0;
	if(offset == 0) // data register
	{
		if(fm7_encoder.position == 0)  // first byte
		{
			fm77av_encoder_setup_command();
		}
		if(fm7_encoder.position == 1)  // second byte
		{
			if(fm7_encoder.buffer[0] == 0x80 || fm7_encoder.buffer[1] == 0x01)
			{
				fm7_encoder.tx_count = 8; // 80 01 command is 9 bytes
			}
		}
		fm7_encoder.buffer[fm7_encoder.position] = data;
		fm7_encoder.position++;
		fm7_encoder.tx_count--;
		if(fm7_encoder.tx_count == 0)  // last byte
			fm77av_encoder_handle_command();

		// wait 5us to set ACK flag
		timer_set(space->machine,ATTOTIME_IN_USEC(5),NULL,0,fm77av_encoder_ack);

		//logerror("ENC: write 0x%02x to data register, moved to pos %i\n",data,fm7_encoder.position);
	}
}

static READ8_HANDLER( fm7_cassette_printer_r )
{
	// bit 7: cassette input
	// bit 5: printer DET2
	// bit 4: printer DTT1
	// bit 3: printer PE
	// bit 2: printer acknowledge
	// bit 1: printer error
	// bit 0: printer busy
	UINT8 ret = 0x00;
	double data = cassette_input(devtag_get_device(space->machine,"cass"));
	const device_config* printer_dev = devtag_get_device(space->machine,"lpt");
	UINT8 pdata;
	int x;

	if(data > 0.03)
		ret |= 0x80;

	if(cassette_get_state(devtag_get_device(space->machine,"cass")) & CASSETTE_MOTOR_DISABLED)
		ret |= 0x80;  // cassette input is high when not in use.

	ret |= 0x70;

	if(input_port_read(space->machine,"config") & 0x01)
	{
		ret |= 0x0f;
		pdata = centronics_data_r(printer_dev,0);
		for(x=0;x<6;x++)
		{
			if(~pdata & (1<<x))
				if(input_port_read(space->machine,"lptjoy") & (1 << x))
					ret &= ~0x08;
		}
	}
	else
	{
		if(image_exists(devtag_get_device(space->machine,"lpt:printer")))
		{
			if(centronics_pe_r(printer_dev))
				ret |= 0x08;
			if(centronics_ack_r(printer_dev))
				ret |= 0x04;
			if(centronics_fault_r(printer_dev))
				ret |= 0x02;
			if(centronics_busy_r(printer_dev))
				ret |= 0x01;
		}
		else
			ret |= 0x0f;
	}
	return ret;
}

static WRITE8_HANDLER( fm7_cassette_printer_w )
{
	static UINT8 prev;
	switch(offset)
	{
		case 0:
		// bit 7: SLCTIN (select?)
		// bit 6: printer strobe
		// bit 1: cassette motor
		// bit 0: cassette output
			if((data & 0x01) != (prev & 0x01))
				cassette_output(devtag_get_device(space->machine,"cass"),(data & 0x01) ? +1.0 : -1.0);
			if((data & 0x02) != (prev & 0x02))
				cassette_change_state(devtag_get_device(space->machine, "cass" ),(data & 0x02) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			centronics_strobe_w(devtag_get_device(space->machine,"lpt"),!(data & 0x40));
			prev = data;
			break;
		case 1:
		// Printer data
			centronics_data_w(devtag_get_device(space->machine,"lpt"),0,data);
			break;
	}
}

/*
 *  Main CPU: 0xfd0b
 *   - bit 0: Boot mode: 0=BASIC, 1=DOS
 */
static READ8_HANDLER( fm77av_boot_mode_r )
{
	UINT8 ret = 0xff;

	if(input_port_read(space->machine,"DSW") & 0x02)
		ret &= ~0x01;

	return 0xff;
}

/*
 *  Main CPU: I/O ports 0xfd0d-0xfd0e
 *  PSG AY-3-891x (FM-7), YM2203 - (FM-77AV and later)
 *  0xfd0d - function select (bit 1 = BDIR, bit 0 = BC1)
 *  0xfd0e - data register
 *  AY I/O ports are not connected to anything.
 */
static void fm7_update_psg(running_machine* machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if(fm7_type == SYS_FM7)
	{
		switch(fm7_psg_regsel)
		{
			case 0x00:
				// High impedance
				break;
			case 0x01:
				// Data read
				psg_data = ay8910_r(devtag_get_device(space->machine,"psg"),0);
				break;
			case 0x02:
				// Data write
				ay8910_data_w(devtag_get_device(space->machine,"psg"),0,psg_data);
				break;
			case 0x03:
				// Address latch
				ay8910_address_w(devtag_get_device(space->machine,"psg"),0,psg_data);
				break;
		}
	}
	else
	{	// FM-77AV and later use a YM2203
		switch(fm7_psg_regsel)
		{
			case 0x00:
				// High impedance
				break;
			case 0x01:
				// Data read
				psg_data = ym2203_r(devtag_get_device(space->machine,"ym"),1);
				break;
			case 0x02:
				// Data write
				ym2203_w(devtag_get_device(space->machine,"ym"),1,psg_data);
				logerror("YM: data write 0x%02x\n",psg_data);
				break;
			case 0x03:
				// Address latch
				ym2203_w(devtag_get_device(space->machine,"ym"),0,psg_data);
				logerror("YM: address latch 0x%02x\n",psg_data);
				break;
			case 0x04:
				// Status register
				psg_data = ym2203_r(devtag_get_device(space->machine,"ym"),0);
				break;
			case 0x09:
				// Joystick port read
				psg_data = input_port_read(space->machine,"joy1");
				break;
		}
	}
}

static READ8_HANDLER( fm7_psg_select_r )
{
	return 0xff;
}

static WRITE8_HANDLER( fm7_psg_select_w )
{
	fm7_psg_regsel = data & 0x03;
	fm7_update_psg(space->machine);
}

static WRITE8_HANDLER( fm77av_ym_select_w )
{
	fm7_psg_regsel = data & 0x0f;
	fm7_update_psg(space->machine);
}

static READ8_HANDLER( fm7_psg_data_r )
{
//  fm7_update_psg(space->machine);
	return psg_data;
}

static WRITE8_HANDLER( fm7_psg_data_w )
{
	psg_data = data;
//  fm7_update_psg(space->machine);
}

static WRITE8_HANDLER( fm77av_bootram_w )
{
	if(!(fm7_mmr.mode & 0x01))
		return;
	fm7_boot_ram[offset] = data;
}

// Shared RAM is only usable on the main CPU if the sub CPU is halted
static READ8_HANDLER( fm7_main_shared_r )
{
	if(fm7_video.sub_halt != 0)
		return shared_ram[offset];
	else
		return 0xff;
}

static WRITE8_HANDLER( fm7_main_shared_w )
{
	if(fm7_video.sub_halt != 0)
		shared_ram[offset] = data;
}

static READ8_HANDLER( fm7_fmirq_r )
{
	UINT8 ret = 0xff;

	if(fm77av_ym_irq != 0)
		ret &= ~0x08;

	return ret;
}

static READ8_DEVICE_HANDLER( fm77av_joy_1_r )
{
	return input_port_read(device->machine,"joy1");
}

static READ8_DEVICE_HANDLER( fm77av_joy_2_r )
{
	return input_port_read(device->machine,"joy2");
}

static READ8_HANDLER( fm7_unknown_r )
{
	// Port 0xFDFC is read by Dig Dug.  Controller port, perhaps?
	// Must return 0xff for it to read the keyboard.
	// Mappy uses ports FD15 and FD16.  On the FM77AV, this is the YM2203,
	// but on the FM-7, this is nothing, so we return 0xff for it to
	// read the keyboard correctly.
	return 0xff;
}

/*
 * Memory Management Register
 * Main CPU: 0xfd80 - 0xfd93  (FM-77L4, FM-77AV and later only)
 *
 * fd80-fd8f (R/W): RAM bank select for current segment (A19-A12)
 * fd90 (W/O): segment select register (3-bit, default = 0)
 * fd92 (W/O): window offset register (OA15-OA8)
 * fd93 (R/W): mode select register
 *              - bit 7: MMR enable/disable
 *              - bit 6: window enable/disable
 *              - bit 0: boot RAM read-write/read-only
 *
 */
static READ8_HANDLER( fm7_mmr_r )
{
	if(offset < 0x10)
	{
		return fm7_mmr.bank_addr[fm7_mmr.segment][offset];
	}
	if(offset == 0x13)
		return fm7_mmr.mode;
	return 0xff;
}

static void fm7_update_bank(const address_space* space, int bank, UINT8 physical)
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	UINT16 size = 0xfff;

	if(bank == 15)
		size = 0xbff;

	if(physical >= 0x10 && physical <= 0x1b)
	{
		switch(physical)
		{
			case 0x10:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram0_r,fm7_vram0_w);
				break;
			case 0x11:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram1_r,fm7_vram1_w);
				break;
			case 0x12:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram2_r,fm7_vram2_w);
				break;
			case 0x13:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram3_r,fm7_vram3_w);
				break;
			case 0x14:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram4_r,fm7_vram4_w);
				break;
			case 0x15:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram5_r,fm7_vram5_w);
				break;
			case 0x16:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram6_r,fm7_vram6_w);
				break;
			case 0x17:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram7_r,fm7_vram7_w);
				break;
			case 0x18:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram8_r,fm7_vram8_w);
				break;
			case 0x19:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vram9_r,fm7_vram9_w);
				break;
			case 0x1a:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vramA_r,fm7_vramA_w);
				break;
			case 0x1b:
				memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_vramB_r,fm7_vramB_w);
				break;
		}
//      memory_set_bankptr(space->machine,bank+1,RAM+(physical<<12)-0x10000);
		return;
	}
	if(physical == 0x1c)
	{
		memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_console_ram_banked_r,fm7_console_ram_banked_w);
		return;
	}
	if(physical == 0x1d)
	{
		memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,fm7_sub_ram_ports_banked_r,fm7_sub_ram_ports_banked_w);
		return;
	}
	if(physical == 0x36 || physical == 0x37)
	{
		if(init_rom_en)
		{
			RAM = memory_region(space->machine,"init");
			memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,SMH_BANK(bank+1),SMH_NOP);
			memory_set_bankptr(space->machine,bank+1,RAM+(physical<<12)-0x36000);
			return;
		}
	}
	if(physical > 0x37 && physical <= 0x3f)
	{
		if(basic_rom_en)
		{
			RAM = memory_region(space->machine,"fbasic");
			memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,SMH_BANK(bank+1),SMH_NOP);
			memory_set_bankptr(space->machine,bank+1,RAM+(physical<<12)-0x38000);
			return;
		}
	}
	memory_install_readwrite8_handler(space,bank*0x1000,(bank*0x1000)+size,0,0,SMH_BANK(bank+1),SMH_BANK(bank+1));
	memory_set_bankptr(space->machine,bank+1,RAM+(physical<<12));
}

void fm7_mmr_refresh(const address_space* space)
{
	int x;
	UINT16 window_addr;
	UINT8* RAM = memory_region(space->machine,"maincpu");

	if(fm7_mmr.enabled)
	{
		for(x=0;x<16;x++)
			fm7_update_bank(space,x,fm7_mmr.bank_addr[fm7_mmr.segment][x]);
	}
	else
	{
		// when MMR is disabled, 0x30000-0x3ffff is banked in
		for(x=0;x<16;x++)
			fm7_update_bank(space,x,0x30+x);
	}

	if(fm7_mmr.mode & 0x40)
	{
		// Handle window offset - 0x7c00-0x7fff will show the area of extended
		// memory (0x00000-0x0ffff) defined by the window address register
		// 0x00 = 0x07c00, 0x04 = 0x08000 ... 0xff = 0x07400.
		window_addr = ((fm7_mmr.window_offset << 8) + 0x7c00) & 0xffff;
//      if(window_addr < 0xfc00)
		{
			memory_install_readwrite8_handler(space,0x7c00,0x7fff,0,0,SMH_BANK(24),SMH_BANK(24));
			memory_set_bankptr(space->machine,24,RAM+window_addr);
		}
	}
}

static WRITE8_HANDLER( fm7_mmr_w )
{
	if(offset < 0x10)
	{
		fm7_mmr.bank_addr[fm7_mmr.segment][offset] = data;
		if(fm7_mmr.enabled)
			fm7_update_bank(space,offset,data);
		logerror("MMR: Segment %i, bank %i, set to  0x%02x\n",fm7_mmr.segment,offset,data);
		return;
	}
	switch(offset)
	{
		case 0x10:
			fm7_mmr.segment = data & 0x07;
			fm7_mmr_refresh(space);
			logerror("MMR: Active segment set to %i\n",fm7_mmr.segment);
			break;
		case 0x12:
			fm7_mmr.window_offset = data;
			fm7_mmr_refresh(space);
			logerror("MMR: Window offset set to %02x\n",data);
			break;
		case 0x13:
			fm7_mmr.mode = data;
			fm7_mmr.enabled = data & 0x80;
			fm7_mmr_refresh(space);
			logerror("MMR: Mode register set to %02x\n",data);
			break;
	}
}

/*
 *  Main CPU: ports 0xfd20-0xfd23
 *  Kanji ROM read ports
 *  FD20 (W/O): ROM address high
 *  FD21 (W/O): ROM address low
 *  FD22 (R/O): Kanji data high
 *  FD23 (R/O): Kanji data low
 *
 *  Kanji ROM is visible at 0x20000 (first half only?)
 */
static READ8_HANDLER( fm7_kanji_r )
{
	UINT8* KROM = memory_region(space->machine,"kanji1");
	UINT32 addr = fm7_kanji_address << 1;

	switch(offset)
	{
		case 0:
		case 1:
			logerror("KANJI: read from invalid register %i\n",offset);
			return 0xff;  // write-only
		case 2:
			return KROM[addr];
		case 3:
			return KROM[addr+1];
		default:
			logerror("KANJI: read from invalid register %i\n",offset);
			return 0xff;
	}
}

static WRITE8_HANDLER( fm7_kanji_w )
{
	UINT16 addr;

	switch(offset)
	{
		case 0:
			addr = ((data & 0xff) << 8) | (fm7_kanji_address & 0x00ff);
			fm7_kanji_address = addr;
			break;
		case 1:
			addr = (data & 0xff) | (fm7_kanji_address & 0xff00);
			fm7_kanji_address = addr;
			break;
		case 2:
		case 3:
		default:
			logerror("KANJI: write to invalid register %i\n",offset);
	}
}

static TIMER_CALLBACK( fm7_timer_irq )
{
	if(irq_mask & IRQ_FLAG_TIMER)
	{
		main_irq_set_flag(machine,IRQ_FLAG_TIMER);
	}
}

static TIMER_CALLBACK( fm7_subtimer_irq )
{
	if(fm7_video.nmi_mask == 0 && fm7_video.sub_halt == 0)
		cputag_set_input_line(machine,"sub",INPUT_LINE_NMI,PULSE_LINE);
}

// When a key is pressed or released (in scan mode only), an IRQ is generated on the main CPU,
// or an FIRQ on the sub CPU, if masked.  Both CPUs have ports to read keyboard data.
// Scancodes are 9 bits in FM-7 mode, 8 bits in scan mode.
static void key_press(running_machine* machine, UINT16 scancode)
{
	current_scancode = scancode;

	if(scancode == 0)
		return;

	if(irq_mask & IRQ_FLAG_KEY)
	{
		main_irq_set_flag(machine,IRQ_FLAG_KEY);
	}
	else
	{
		cputag_set_input_line(machine,"sub",M6809_FIRQ_LINE,ASSERT_LINE);
	}
	logerror("KEY: sent scancode 0x%03x\n",scancode);
}

static void fm7_keyboard_poll_scan(running_machine* machine)
{
	const char* portnames[3] = { "key1","key2","key3" };
	int bit = 0;
	int x,y;
	UINT32 keys;
	UINT32 modifiers = input_port_read(machine,"key_modifiers");
	UINT16 modscancodes[6] = { 0x52, 0x53, 0x54, 0x55, 0x56, 0x5a };

	for(x=0;x<3;x++)
	{
		keys = input_port_read(machine,portnames[x]);

		for(y=0;y<32;y++)  // loop through each bit in the port
		{
			if((keys & (1<<y)) != 0 && (key_data[x] & (1<<y)) == 0)
			{
				key_press(machine,fm7_key_list[bit][6]); // key press
			}
			if((keys & (1<<y)) == 0 && (key_data[x] & (1<<y)) != 0)
			{
				key_press(machine,fm7_key_list[bit][6] | 0x80); // key release
			}
			bit++;
		}

		key_data[x] = keys;
	}
	// check modifier keys
	bit = 0;
	for(y=0;x<7;x++)
	{
		if((modifiers & (1<<y)) != 0 && (mod_data & (1<<y)) == 0)
		{
			key_press(machine,modscancodes[bit]); // key press
		}
		if((modifiers & (1<<y)) == 0 && (mod_data & (1<<y)) != 0)
		{
			key_press(machine,modscancodes[bit] | 0x80); // key release
		}
		bit++;
	}
	mod_data = modifiers;
}

static TIMER_CALLBACK( fm7_keyboard_poll )
{
	const char* portnames[3] = { "key1","key2","key3" };
	int x,y;
	int bit = 0;
	int mod = 0;
	UINT32 keys;
	UINT32 modifiers = input_port_read(machine,"key_modifiers");

	if(input_port_read(machine,"key3") & 0x40000)
	{
		break_flag = 1;
		cputag_set_input_line(machine,"maincpu",M6809_FIRQ_LINE,ASSERT_LINE);
	}
	else
		break_flag = 0;

	if(key_scan_mode == KEY_MODE_SCAN)
	{
		// handle scancode mode
		fm7_keyboard_poll_scan(machine);
		return;
	}

	// check key modifiers (Shift, Ctrl, Kana, etc...)
	if(modifiers & 0x02 || modifiers & 0x04)
		mod = 1;  // shift
	if(modifiers & 0x10)
		mod = 3;  // Graph  (shift has no effect with graph)
	if(modifiers & 0x01)
		mod = 2;  // ctrl (overrides shift, if also pressed)
	if(modifiers & 0x20)
		mod = 4;  // kana (overrides all)
	if((modifiers & 0x22) == 0x22 || (modifiers & 0x24) == 0x24)
		mod = 5;  // shifted kana

	for(x=0;x<3;x++)
	{
		keys = input_port_read(machine,portnames[x]);

		for(y=0;y<32;y++)  // loop through each bit in the port
		{
			if((keys & (1<<y)) != 0 && (key_data[x] & (1<<y)) == 0)
			{
				key_press(machine,fm7_key_list[bit][mod]); // key press
			}
			bit++;
		}

		key_data[x] = keys;
	}
}

static IRQ_CALLBACK(fm7_irq_ack)
{
	if(irqline == M6809_FIRQ_LINE)
		cputag_set_input_line(device->machine,"maincpu",irqline,CLEAR_LINE);
	return -1;
}

static IRQ_CALLBACK(fm7_sub_irq_ack)
{
	cputag_set_input_line(device->machine,"sub",irqline,CLEAR_LINE);
	return -1;
}

static void fm77av_fmirq(const device_config* device,int irq)
{
	if(irq == 1)
	{
		// cannot be masked
		main_irq_set_flag(device->machine,IRQ_FLAG_OTHER);
		fm77av_ym_irq = 1;
		logerror("YM: IRQ on\n");
	}
	else
	{
		main_irq_clear_flag(device->machine,IRQ_FLAG_OTHER);
		fm77av_ym_irq = 0;
		logerror("YM: IRQ off\n");
	}
}

/*
   0000 - 7FFF: (RAM) BASIC working area, user's area
   8000 - FBFF: (ROM) F-BASIC ROM, extra user RAM
   FC00 - FC7F: more RAM, if 64kB is installed
   FC80 - FCFF: Shared RAM between main and sub CPU, available only when sub CPU is halted
   FD00 - FDFF: I/O space (6809 uses memory-mapped I/O)
   FE00 - FFEF: Boot rom
   FFF0 - FFFF: Interrupt vector table
*/
// The FM-7 has only 64kB RAM, so we'll worry about banking when we do the later models
static ADDRESS_MAP_START( fm7_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0x7fff) AM_RAM
	AM_RANGE(0x8000,0xfbff) AM_ROMBANK(1) // also F-BASIC ROM, when enabled
	AM_RANGE(0xfc00,0xfc7f) AM_RAM
	AM_RANGE(0xfc80,0xfcff) AM_RAM AM_READWRITE(fm7_main_shared_r,fm7_main_shared_w)
	// I/O space (FD00-FDFF)
	AM_RANGE(0xfd00,0xfd01) AM_READWRITE(fm7_keyboard_r,fm7_cassette_printer_w)
	AM_RANGE(0xfd02,0xfd02) AM_READWRITE(fm7_cassette_printer_r,fm7_irq_mask_w)  // IRQ mask
	AM_RANGE(0xfd03,0xfd03) AM_READWRITE(fm7_irq_cause_r,fm7_beeper_w)  // IRQ flags
	AM_RANGE(0xfd04,0xfd04) AM_READ(fm7_fd04_r)
	AM_RANGE(0xfd05,0xfd05) AM_READWRITE(fm7_subintf_r,fm7_subintf_w)
	AM_RANGE(0xfd06,0xfd0c) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd0d,0xfd0d) AM_READWRITE(fm7_psg_select_r,fm7_psg_select_w)
	AM_RANGE(0xfd0e,0xfd0e) AM_READWRITE(fm7_psg_data_r, fm7_psg_data_w)
	AM_RANGE(0xfd0f,0xfd0f) AM_READWRITE(fm7_rom_en_r,fm7_rom_en_w)
	AM_RANGE(0xfd10,0xfd17) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd18,0xfd1f) AM_READWRITE(fm7_fdc_r,fm7_fdc_w)
	AM_RANGE(0xfd20,0xfd23) AM_READWRITE(fm7_kanji_r,fm7_kanji_w)
	AM_RANGE(0xfd24,0xfd36) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd37,0xfd37) AM_WRITE(fm7_multipage_w)
	AM_RANGE(0xfd38,0xfd3f) AM_READWRITE(fm7_palette_r,fm7_palette_w)
	AM_RANGE(0xfd40,0xfdff) AM_READ(fm7_unknown_r)
	// Boot ROM
	AM_RANGE(0xfe00,0xffdf) AM_ROMBANK(17)
	AM_RANGE(0xffe0,0xffef) AM_RAM
	AM_RANGE(0xfff0,0xffff) AM_READWRITE(vector_r,vector_w)
ADDRESS_MAP_END

/*
   0000 - 3FFF: Video RAM bank 0 (Blue plane)
   4000 - 7FFF: Video RAM bank 1 (Red plane)
   8000 - BFFF: Video RAM bank 2 (Green plane)
   D000 - D37F: (RAM) working area
   D380 - D3FF: Shared RAM between main and sub CPU
   D400 - D4FF: I/O ports
   D800 - FFDF: (ROM) Graphics command code
   FFF0 - FFFF: Interrupt vector table
*/

static ADDRESS_MAP_START( fm7_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0xbfff) AM_READWRITE(fm7_vram_r,fm7_vram_w) // VRAM
	AM_RANGE(0xc000,0xcfff) AM_RAM // Console RAM
	AM_RANGE(0xd000,0xd37f) AM_RAM // Work RAM
	AM_RANGE(0xd380,0xd3ff) AM_RAM AM_BASE(&shared_ram)
	// I/O space (D400-D4FF)
	AM_RANGE(0xd400,0xd401) AM_READ(fm7_sub_keyboard_r)
	AM_RANGE(0xd402,0xd402) AM_READ(fm7_cancel_ack)
	AM_RANGE(0xd403,0xd403) AM_READ(fm7_sub_beeper_r)
	AM_RANGE(0xd404,0xd404) AM_READ(fm7_attn_irq_r)
	AM_RANGE(0xd408,0xd408) AM_READWRITE(fm7_crt_r,fm7_crt_w)
	AM_RANGE(0xd409,0xd409) AM_READWRITE(fm7_vram_access_r,fm7_vram_access_w)
	AM_RANGE(0xd40a,0xd40a) AM_READWRITE(fm7_sub_busyflag_r,fm7_sub_busyflag_w)
	AM_RANGE(0xd40e,0xd40f) AM_WRITE(fm7_vram_offset_w)
	AM_RANGE(0xd800,0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x1000,0x1fff) AM_RAMBANK(2)
	AM_RANGE(0x2000,0x2fff) AM_RAMBANK(3)
	AM_RANGE(0x3000,0x3fff) AM_RAMBANK(4)
	AM_RANGE(0x4000,0x4fff) AM_RAMBANK(5)
	AM_RANGE(0x5000,0x5fff) AM_RAMBANK(6)
	AM_RANGE(0x6000,0x6fff) AM_RAMBANK(7)
	AM_RANGE(0x7000,0x7fff) AM_RAMBANK(8)
	AM_RANGE(0x8000,0x8fff) AM_RAMBANK(9)
	AM_RANGE(0x9000,0x9fff) AM_RAMBANK(10)
	AM_RANGE(0xa000,0xafff) AM_RAMBANK(11)
	AM_RANGE(0xb000,0xbfff) AM_RAMBANK(12)
	AM_RANGE(0xc000,0xcfff) AM_RAMBANK(13)
	AM_RANGE(0xd000,0xdfff) AM_RAMBANK(14)
	AM_RANGE(0xe000,0xefff) AM_RAMBANK(15)
	AM_RANGE(0xf000,0xfbff) AM_RAMBANK(16)
	AM_RANGE(0xfc00,0xfc7f) AM_RAM
	AM_RANGE(0xfc80,0xfcff) AM_RAM AM_READWRITE(fm7_main_shared_r,fm7_main_shared_w)
	// I/O space (FD00-FDFF)
	AM_RANGE(0xfd00,0xfd01) AM_READWRITE(fm7_keyboard_r,fm7_cassette_printer_w)
	AM_RANGE(0xfd02,0xfd02) AM_READWRITE(fm7_cassette_printer_r,fm7_irq_mask_w)  // IRQ mask
	AM_RANGE(0xfd03,0xfd03) AM_READWRITE(fm7_irq_cause_r,fm7_beeper_w)  // IRQ flags
	AM_RANGE(0xfd04,0xfd04) AM_READ(fm7_fd04_r)
	AM_RANGE(0xfd05,0xfd05) AM_READWRITE(fm7_subintf_r,fm7_subintf_w)
	AM_RANGE(0xfd06,0xfd0a) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd0b,0xfd0b) AM_READ(fm77av_boot_mode_r)
	AM_RANGE(0xfd0c,0xfd0c) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd0d,0xfd0d) AM_READWRITE(fm7_psg_select_r,fm7_psg_select_w)
	AM_RANGE(0xfd0e,0xfd0e) AM_READWRITE(fm7_psg_data_r, fm7_psg_data_w)
	AM_RANGE(0xfd0f,0xfd0f) AM_READWRITE(fm7_rom_en_r,fm7_rom_en_w)
	AM_RANGE(0xfd10,0xfd10) AM_WRITE(fm7_init_en_w)
	AM_RANGE(0xfd11,0xfd11) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd12,0xfd12) AM_READWRITE(fm77av_sub_modestatus_r,fm77av_sub_modestatus_w)
	AM_RANGE(0xfd13,0xfd13) AM_WRITE(fm77av_sub_bank_w)
	AM_RANGE(0xfd14,0xfd14) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd15,0xfd15) AM_READWRITE(fm7_psg_select_r,fm77av_ym_select_w)
	AM_RANGE(0xfd16,0xfd16) AM_READWRITE(fm7_psg_data_r,fm7_psg_data_w)
	AM_RANGE(0xfd17,0xfd17) AM_READ(fm7_fmirq_r)
	AM_RANGE(0xfd18,0xfd1f) AM_READWRITE(fm7_fdc_r,fm7_fdc_w)
	AM_RANGE(0xfd20,0xfd23) AM_READWRITE(fm7_kanji_r,fm7_kanji_w)
	AM_RANGE(0xfd24,0xfd2b) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd30,0xfd34) AM_WRITE(fm77av_analog_palette_w)
	AM_RANGE(0xfd35,0xfd36) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd37,0xfd37) AM_WRITE(fm7_multipage_w)
	AM_RANGE(0xfd38,0xfd3f) AM_READWRITE(fm7_palette_r,fm7_palette_w)
	AM_RANGE(0xfd40,0xfd7f) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd80,0xfd93) AM_READWRITE(fm7_mmr_r,fm7_mmr_w)
	AM_RANGE(0xfd94,0xfdff) AM_READ(fm7_unknown_r)
	// Boot ROM (RAM on FM77AV and later)
	AM_RANGE(0xfe00,0xffdf) AM_RAM_WRITE(fm77av_bootram_w) AM_BASE(&fm7_boot_ram)
	AM_RANGE(0xffe0,0xffef) AM_RAM
	AM_RANGE(0xfff0,0xffff) AM_READWRITE(vector_r,vector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0xbfff) AM_READWRITE(fm7_vram_r,fm7_vram_w) // VRAM
	AM_RANGE(0xc000,0xcfff) AM_RAM AM_REGION("maincpu",0x1c000) // Console RAM
	AM_RANGE(0xd000,0xd37f) AM_RAM AM_REGION("maincpu",0x1d000) // Work RAM
	AM_RANGE(0xd380,0xd3ff) AM_RAM AM_BASE(&shared_ram)
	// I/O space (D400-D4FF)
	AM_RANGE(0xd400,0xd401) AM_READ(fm7_sub_keyboard_r)
	AM_RANGE(0xd402,0xd402) AM_READ(fm7_cancel_ack)
	AM_RANGE(0xd403,0xd403) AM_READ(fm7_sub_beeper_r)
	AM_RANGE(0xd404,0xd404) AM_READ(fm7_attn_irq_r)
	AM_RANGE(0xd408,0xd408) AM_READWRITE(fm7_crt_r,fm7_crt_w)
	AM_RANGE(0xd409,0xd409) AM_READWRITE(fm7_vram_access_r,fm7_vram_access_w)
	AM_RANGE(0xd40a,0xd40a) AM_READWRITE(fm7_sub_busyflag_r,fm7_sub_busyflag_w)
	AM_RANGE(0xd40e,0xd40f) AM_WRITE(fm7_vram_offset_w)
	AM_RANGE(0xd410,0xd42b) AM_READWRITE(fm77av_alu_r, fm77av_alu_w)
	AM_RANGE(0xd430,0xd430) AM_READWRITE(fm77av_video_flags_r,fm77av_video_flags_w)
	AM_RANGE(0xd431,0xd432) AM_READWRITE(fm77av_key_encoder_r,fm77av_key_encoder_w)
	AM_RANGE(0xd500,0xd7ff) AM_RAM AM_REGION("maincpu",0x1d500) // Work RAM
	AM_RANGE(0xd800,0xdfff) AM_ROMBANK(20)
	AM_RANGE(0xe000,0xffff) AM_ROMBANK(21)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( fm7 )

  PORT_START("key1")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_UNUSED)
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(27)
  PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
  PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
  PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
  PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
  PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
  PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
  PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
  PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
  PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^')
  PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xEF\xBF\xA5")
  PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
  PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
  PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
  PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
  PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
  PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
  PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
  PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
  PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
  PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
  PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
  PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
  PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@')
  PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[')
  PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
  PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
  PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')

  PORT_START("key2")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
  PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
  PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
  PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
  PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
  PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']')
  PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
  PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
  PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
  PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
  PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
  PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
  PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
  PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
  PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.')
  PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
  PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("_")
  PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
  PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey *") PORT_CODE(KEYCODE_ASTERISK)
  PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey /") PORT_CODE(KEYCODE_SLASH_PAD)
  PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey +") PORT_CODE(KEYCODE_PLUS_PAD)
  PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey -") PORT_CODE(KEYCODE_MINUS_PAD)
  PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 7") PORT_CODE(KEYCODE_7_PAD)
  PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 8") PORT_CODE(KEYCODE_8_PAD)
  PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 9") PORT_CODE(KEYCODE_9_PAD)
  PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey =")
  PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 4") PORT_CODE(KEYCODE_4_PAD)
  PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 5") PORT_CODE(KEYCODE_5_PAD)

  PORT_START("key3")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 6") PORT_CODE(KEYCODE_6_PAD)
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey ,")
  PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 1") PORT_CODE(KEYCODE_1_PAD)
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 2") PORT_CODE(KEYCODE_2_PAD)
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 3") PORT_CODE(KEYCODE_3_PAD)
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey Enter") PORT_CODE(KEYCODE_ENTER_PAD)
  PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 0") PORT_CODE(KEYCODE_0_PAD)
  PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey .") PORT_CODE(KEYCODE_DEL_PAD)
  PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_INSERT)
  PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("EL") PORT_CODE(KEYCODE_PGUP)
  PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CLS") PORT_CODE(KEYCODE_PGDN)
  PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
  PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DUP") PORT_CODE(KEYCODE_END)
  PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
  PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
  PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
  PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
  PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
  PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_ESC)
  PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
  PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2)
  PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)
  PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4)
  PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5)
  PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF6") PORT_CODE(KEYCODE_F6)
  PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF7") PORT_CODE(KEYCODE_F7)
  PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF8") PORT_CODE(KEYCODE_F8)
  PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF9") PORT_CODE(KEYCODE_F9)
  PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF10") PORT_CODE(KEYCODE_F10)

  PORT_START("key_modifiers")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT)
  PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CAP") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_RALT)
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Kana") PORT_CODE(KEYCODE_RCONTROL) PORT_TOGGLE

  PORT_START("lptjoy")
  PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_JOYSTICK_RIGHT) PORT_NAME("LPT Joystick Right") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_JOYSTICK_LEFT) PORT_NAME("LPT Joystick Left") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_JOYSTICK_UP) PORT_NAME("LPT Joystick Up") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_JOYSTICK_DOWN) PORT_NAME("LPT Joystick Down") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_BUTTON2) PORT_NAME("LPT Joystick Button 2") PORT_PLAYER(1)
  PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_BUTTON1) PORT_NAME("LPT Joystick Button 1") PORT_PLAYER(1)

  PORT_START("joy1")
  PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_JOYSTICK_UP) PORT_NAME("1P Joystick Up") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_JOYSTICK_DOWN) PORT_NAME("1P Joystick Down") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_JOYSTICK_LEFT) PORT_NAME("1P Joystick Left") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_JOYSTICK_RIGHT) PORT_NAME("1P Joystick Right") PORT_8WAY PORT_PLAYER(1)
  PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_BUTTON2) PORT_NAME("1P Joystick Button 2") PORT_PLAYER(1)
  PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_BUTTON1) PORT_NAME("1P Joystick Button 1") PORT_PLAYER(1)
  PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_UNUSED)
  PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_UNUSED)

  PORT_START("joy2")
  PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_JOYSTICK_UP) PORT_NAME("2P Joystick Up") PORT_8WAY PORT_PLAYER(2)
  PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_JOYSTICK_DOWN) PORT_NAME("2P Joystick Down") PORT_8WAY PORT_PLAYER(2)
  PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_JOYSTICK_LEFT) PORT_NAME("2P Joystick Left") PORT_8WAY PORT_PLAYER(2)
  PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_JOYSTICK_RIGHT) PORT_NAME("2P Joystick Right") PORT_8WAY PORT_PLAYER(2)
  PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_BUTTON2) PORT_NAME("2P Joystick Button 2") PORT_PLAYER(2)
  PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_BUTTON1) PORT_NAME("2P Joystick Button 1") PORT_PLAYER(2)
  PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_UNUSED)
  PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_UNUSED)


  PORT_START("DSW")
  PORT_DIPNAME(0x01,0x01,"Switch A") PORT_DIPLOCATION("SWA:1")
  PORT_DIPSETTING(0x00,DEF_STR( Off ))
  PORT_DIPSETTING(0x01,DEF_STR( On ))
  PORT_DIPNAME(0x02,0x02,"Boot mode") PORT_DIPLOCATION("SWA:2")
  PORT_DIPSETTING(0x00,"DOS")
  PORT_DIPSETTING(0x02,"BASIC")
  PORT_DIPNAME(0x04,0x00,"Switch C") PORT_DIPLOCATION("SWA:3")
  PORT_DIPSETTING(0x00,DEF_STR( Off ))
  PORT_DIPSETTING(0x04,DEF_STR( On ))
  PORT_DIPNAME(0x08,0x00,"FM-8 Compatibility mode") PORT_DIPLOCATION("SWA:4")
  PORT_DIPSETTING(0x00,DEF_STR( Off ))
  PORT_DIPSETTING(0x08,DEF_STR( On ))

  PORT_START("config")
  PORT_CONFNAME(0x01,0x00,"Printer port device")
  PORT_CONFSETTING(0x00,"Printer")
  PORT_CONFSETTING(0x01,"Dempa Shinbunsha Joystick")
INPUT_PORTS_END

static DRIVER_INIT(fm7)
{
//  shared_ram = auto_alloc_array(machine,UINT8,0x80);
	fm7_video_ram = auto_alloc_array(machine,UINT8,0x18000);  // 2 pages on some systems
	fm7_timer = timer_alloc(machine,fm7_timer_irq,NULL);
	fm7_subtimer = timer_alloc(machine,fm7_subtimer_irq,NULL);
	fm7_keyboard_timer = timer_alloc(machine,fm7_keyboard_poll,NULL);
	if(fm7_type != SYS_FM7)
		fm77av_vsync_timer = timer_alloc(machine,fm77av_vsync,NULL);
	cpu_set_irq_callback(cputag_get_cpu(machine,"maincpu"),fm7_irq_ack);
	cpu_set_irq_callback(cputag_get_cpu(machine,"sub"),fm7_sub_irq_ack);
}

static MACHINE_START(fm7)
{
	// The FM-7 has no initialisation ROM, and no other obvious
	// way to set the reset vector, so for now this will have to do.
	UINT8* RAM = memory_region(machine,"maincpu");

	RAM[0xfffe] = 0xfe;
	RAM[0xffff] = 0x00;

	memset(shared_ram,0xff,0x80);
	fm7_type = SYS_FM7;

	beep_set_frequency(devtag_get_device(machine,"beeper"),1200);
	beep_set_state(devtag_get_device(machine,"beeper"),0);
}

static MACHINE_START(fm77av)
{
	UINT8* RAM = memory_region(machine,"maincpu");
	UINT8* ROM = memory_region(machine,"init");

	memset(shared_ram,0xff,0x80);

	// last part of Initiate ROM is visible at the end of RAM too (interrupt vectors)
	memcpy(RAM+0x3fff0,ROM+0x1ff0,16);

	fm7_video.subrom = 0;  // default sub CPU ROM is type C.
	RAM = memory_region(machine,"subsyscg");
	memory_set_bankptr(machine,20,RAM);
	RAM = memory_region(machine,"subsys_c");
	memory_set_bankptr(machine,21,RAM+0x800);

	fm7_type = SYS_FM77AV;
	beep_set_frequency(devtag_get_device(machine,"beeper"),1200);
	beep_set_state(devtag_get_device(machine,"beeper"),0);
}

static MACHINE_RESET(fm7)
{
	UINT8* RAM = memory_region(machine,"maincpu");
	UINT8* ROM = memory_region(machine,"init");

	timer_adjust_periodic(fm7_timer,ATTOTIME_IN_NSEC(2034500),0,ATTOTIME_IN_NSEC(2034500));
	timer_adjust_periodic(fm7_subtimer,ATTOTIME_IN_MSEC(20),0,ATTOTIME_IN_MSEC(20));
	timer_adjust_periodic(fm7_keyboard_timer,attotime_zero,0,ATTOTIME_IN_MSEC(10));
	if(fm7_type != SYS_FM7)
		timer_adjust_oneshot(fm77av_vsync_timer,video_screen_get_time_until_vblank_end(machine->primary_screen),0);

	irq_mask = 0x00;
	irq_flags = 0x00;
	fm7_video.attn_irq = 0;
	fm7_video.sub_busy = 0x80;  // busy at reset
	basic_rom_en = 1;  // enabled at reset
	if(fm7_type == SYS_FM7)
		init_rom_en = 0;
	else
	{
		init_rom_en = 1;
		// last part of Initiate ROM is visible at the end of RAM too (interrupt vectors)
		memcpy(RAM+0x3fff0,ROM+0x1ff0,16);
	}
	if(fm7_type == SYS_FM7)
		memory_set_bankptr(machine,1,RAM+0x38000);
	key_delay = 700;  // 700ms on FM-7
	key_repeat = 70;  // 70ms on FM-7
	break_flag = 0;
	key_scan_mode = KEY_MODE_FM7;
	fm7_psg_regsel = 0;
	psg_data = 0;
	fdc_side = 0;
	fdc_drive = 0;
	fm7_mmr.mode = 0;
	fm7_mmr.segment = 0;
	fm7_mmr.enabled = 0;
	fm77av_ym_irq = 0;
	fm7_encoder.latch = 1;
	fm7_encoder.ack = 1;
	// set boot mode (FM-7 only, AV and later has boot RAM instead)
	if(fm7_type == SYS_FM7)
	{
		if(!(input_port_read(machine,"DSW") & 0x02))
		{  // DOS mode
			memory_set_bankptr(machine,17,memory_region(machine,"dos"));
		}
		else
		{  // BASIC mode
			memory_set_bankptr(machine,17,memory_region(machine,"basic"));
		}
	}
	if(fm7_type != SYS_FM7)  // set default RAM banks
	{
		fm7_mmr_refresh(cpu_get_address_space(cputag_get_cpu(machine,"maincpu"),ADDRESS_SPACE_PROGRAM));
	}
}

static const wd17xx_interface fm7_mb8877a_interface =
{
	DEVCB_LINE(fm7_fdc_intrq_w),
	DEVCB_LINE(fm7_fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static const ay8910_interface fm7_psg_intf =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,	/* portA read */
	DEVCB_NULL,	/* portB read */
	DEVCB_NULL,					/* portA write */
	DEVCB_NULL					/* portB write */
};

static const ym2203_interface fm7_ym_intf =
{
	{
		AY8910_LEGACY_OUTPUT,
		AY8910_DEFAULT_LOADS,
		DEVCB_HANDLER(fm77av_joy_1_r),
		DEVCB_HANDLER(fm77av_joy_2_r),
		DEVCB_NULL,					/* portA write */
		DEVCB_NULL					/* portB write */
	},
	fm77av_fmirq
};

static const cassette_config fm7_cassette_config =
{
	fm7_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};

static const floppy_config fm7_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( fm7 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_mem)
	MDRV_QUANTUM_PERFECT_CPU("maincpu")

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_sub_mem)
	MDRV_QUANTUM_PERFECT_CPU("sub")

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("psg", AY8910, XTAL_4_9152MHz / 4)
	MDRV_SOUND_CONFIG(fm7_psg_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",1.0)
	MDRV_SOUND_ADD("beeper", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",1.0)
	MDRV_SOUND_WAVE_ADD("wave","cass")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.20)

	MDRV_MACHINE_START(fm7)
	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(fm7)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)

	MDRV_CASSETTE_ADD("cass",fm7_cassette_config)

	MDRV_MB8877_ADD("fdc",fm7_mb8877a_interface)

	MDRV_CENTRONICS_ADD("lpt",standard_centronics)

	MDRV_FLOPPY_2_DRIVES_ADD(fm7_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fm77av )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809, XTAL_2MHz)  // actually MB68B09E, but the 6809E core runs too slowly
	MDRV_CPU_PROGRAM_MAP(fm77av_mem)
	MDRV_QUANTUM_PERFECT_CPU("maincpu")

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm77av_sub_mem)
	MDRV_QUANTUM_PERFECT_CPU("sub")

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ym", YM2203, XTAL_4_9152MHz / 4)
	MDRV_SOUND_CONFIG(fm7_ym_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",1.0)
	MDRV_SOUND_ADD("beeper", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",1.0)
	MDRV_SOUND_WAVE_ADD("wave","cass")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.20)

	MDRV_MACHINE_START(fm77av)
	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8 + 4096)
	MDRV_PALETTE_INIT(fm7)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)

	MDRV_CASSETTE_ADD("cass",fm7_cassette_config)

	MDRV_MB8877_ADD("fdc",fm7_mb8877a_interface)

	MDRV_CENTRONICS_ADD("lpt",standard_centronics)

	MDRV_FLOPPY_2_DRIVES_ADD(fm7_floppy_config)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( fm7 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x20000, "sub", 0 )
	ROM_LOAD( "subsys_c.rom", 0xd800,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c) )

	// either one of these boot ROMs are selectable via DIP switch
	ROM_REGION( 0x200, "basic", 0 )
	ROM_LOAD( "boot_bas.rom", 0x0000,  0x0200, CRC(c70f0c74) SHA1(53b63a301cba7e3030e79c59a4d4291eab6e64b0) )

	ROM_REGION( 0x200, "dos", 0 )
	ROM_LOAD( "boot_dos.rom", 0x0000,  0x0200, CRC(198614ff) SHA1(037e5881bd3fed472a210ee894a6446965a8d2ef) )

	// optional Kanji ROM
	ROM_REGION( 0x20000, "kanji1", 0 )
	ROM_LOAD_OPTIONAL( "kanji.rom", 0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )

ROM_END

ROM_START( fm77av )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_FILL(0x0000,0x40000,0xff)

	ROM_REGION( 0x2000, "init", 0 )
	ROM_LOAD( "initiate.rom", 0x0000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )

	ROM_REGION( 0x7c00, "fbasic", 0 )
	ROM_LOAD( "fbasic30.rom", 0x0000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_FILL(0x0000,0x10000,0xff)

	// sub CPU ROMs
	ROM_REGION( 0x2800, "subsys_c", 0 )
	ROM_LOAD( "subsys_c.rom", 0x0000,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c) )
	ROM_REGION( 0x2000, "subsys_a", 0 )
	ROM_LOAD( "subsys_a.rom", 0x0000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83) )
	ROM_REGION( 0x2000, "subsys_b", 0 )
	ROM_LOAD( "subsys_b.rom", 0x0000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8) )
	ROM_REGION( 0x2000, "subsyscg", 0 )
	ROM_LOAD( "subsyscg.rom", 0x0000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "kanji1", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )

	// optional dict rom?
ROM_END

ROM_START( fm7740sx )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_FILL(0x0000,0x40000,0xff)

	ROM_REGION( 0x2000, "init", 0 )
	ROM_LOAD( "initiate.rom", 0x0000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )

	ROM_REGION( 0x7c00, "fbasic", 0 )
	ROM_LOAD( "fbasic30.rom", 0x0000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_FILL(0x0000,0x10000,0xff)

	// sub CPU ROMs
	ROM_REGION( 0x2800, "subsys_c", 0 )
	ROM_LOAD( "subsys_c.rom", 0x0000,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c) )
	ROM_REGION( 0x2000, "subsys_a", 0 )
	ROM_LOAD( "subsys_a.rom", 0x0000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83) )
	ROM_REGION( 0x2000, "subsys_b", 0 )
	ROM_LOAD( "subsys_b.rom", 0x0000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8) )
	ROM_REGION( 0x2000, "subsyscg", 0 )
	ROM_LOAD( "subsyscg.rom", 0x0000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "kanji1", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )
	ROM_LOAD( "kanji2.rom", 0x0000, 0x20000, CRC(38644251) SHA1(ebfdc43c38e1380709ed08575c346b2467ad1592) )

	/* These should be loaded at 2e000-2ffff of maincpu, but I'm not sure if it is correct */
	ROM_REGION( 0x4c000, "additional", 0 )
	ROM_LOAD( "dicrom.rom", 0x00000, 0x40000, CRC(b142acbc) SHA1(fe9f92a8a2750bcba0a1d2895e75e83858e4f97f) )
	ROM_LOAD( "extsub.rom", 0x40000, 0x0c000, CRC(0f7fcce3) SHA1(a1304457eeb400b4edd3c20af948d66a04df255e) )

ROM_END

/* Driver */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE  INPUT   INIT  CONFIG  COMPANY      FULLNAME        FLAGS */
COMP( 1982, fm7,      0,      0,      fm7,     fm7,    fm7,  0,    "Fujitsu",   "FM-7",         0)
COMP( 1985, fm77av,   fm7,    0,      fm77av,  fm7,    fm7,  0,    "Fujitsu",   "FM-77AV",      GAME_IMPERFECT_GRAPHICS)
COMP( 1985, fm7740sx, fm7,    0,      fm77av,  fm7,    fm7,  0,    "Fujitsu",   "FM-77AV40SX",  GAME_NOT_WORKING)
