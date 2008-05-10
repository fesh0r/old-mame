#include "driver.h"
#include "deprecat.h"
#include "video/atarist.h"
#include "cpu/m68000/m68k.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6800/m6800.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/printer.h"
#include "machine/centroni.h"
#include "includes/serial.h"
#include "machine/6850acia.h"
#include "machine/68901mfp.h"
#include "machine/8530scc.h"
#include "machine/rp5c15.h"
#include "machine/wd17xx.h"
#include "sound/ay8910.h"
#include "audio/lmc1992.h"
#include "includes/atarist.h"

/*

    TODO:

    - fix floppy interface
    - fix mouse
    - MSA disk image support
    - UK keyboard layout for the special keys
    - accurate screen timing
    - floppy DMA transfer timer
    - memory shadow for boot memory check
    - STe DMA sound and LMC1992 Microwire mixer
    - Mega ST/STe MC68881 FPU
    - MIDI interface
    - Mega STe 16KB cache
    - Mega STe LAN

*/

/* Floppy Disk Controller */

#define ATARIST_FLOPPY_STATUS_FDC_DATA_REQUEST	0x04
#define ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO	0x02
#define ATARIST_FLOPPY_STATUS_DMA_ERROR			0x01

#define ATARIST_FLOPPY_MODE_WRITE				0x0100
#define ATARIST_FLOPPY_MODE_FDC_ACCESS			0x0080
#define ATARIST_FLOPPY_MODE_DMA_DISABLE			0x0040
#define ATARIST_FLOPPY_MODE_SECTOR_COUNT		0x0010
#define ATARIST_FLOPPY_MODE_HDC					0x0008
#define ATARIST_FLOPPY_MODE_ADDRESS_MASK		0x0006

#define ATARIST_FLOPPY_BYTES_PER_SECTOR			512

static struct FDC
{
	UINT32 dmabase;
	UINT16 status, mode;
	UINT8 sectors;
	int dmabytes;
	int irq;
} fdc;

static void atarist_fdc_dma_transfer(running_machine *machine)
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	if ((fdc.mode & ATARIST_FLOPPY_MODE_DMA_DISABLE) == 0)
	{
		while (fdc.sectors > 0)
		{
			if (fdc.mode & ATARIST_FLOPPY_MODE_WRITE)
			{
				wd17xx_data_w(machine, 0, RAM[fdc.dmabase]);
			}
			else
			{
				RAM[fdc.dmabase] = wd17xx_data_r(machine, 0);
			}

			fdc.dmabase++;
			fdc.dmabytes--;

			if (fdc.dmabytes == 0)
			{
				fdc.sectors--;

				if (fdc.sectors == 0)
				{
					fdc.status &= ~ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO;
				}
				else
				{
					fdc.dmabytes = ATARIST_FLOPPY_BYTES_PER_SECTOR;
				}
			}
		}
	}
}

static void atarist_fdc_callback(running_machine *machine, wd17xx_state_t event, void *param)
{
	switch (event)
	{
	case WD17XX_IRQ_SET:
		fdc.irq = 1;
		break;

	case WD17XX_IRQ_CLR:
		fdc.irq = 0;
		break;

	case WD17XX_DRQ_SET:
		fdc.status |= ATARIST_FLOPPY_STATUS_FDC_DATA_REQUEST;
		atarist_fdc_dma_transfer(machine);
		break;

	case WD17XX_DRQ_CLR:
		fdc.status &= ~ATARIST_FLOPPY_STATUS_FDC_DATA_REQUEST;
		break;
	}
}

static READ16_HANDLER( atarist_fdc_data_r )
{
	if (fdc.mode & ATARIST_FLOPPY_MODE_SECTOR_COUNT)
	{
		return fdc.sectors;
	}
	else
	{
		if (fdc.mode & ATARIST_FLOPPY_MODE_HDC)
		{
			// HDC not implemented
			fdc.status &= ~ATARIST_FLOPPY_STATUS_DMA_ERROR;

			return 0;
		}
		else
		{
			return wd17xx_r(machine, (fdc.mode & ATARIST_FLOPPY_MODE_ADDRESS_MASK) >> 1);
		}
	}
}

static WRITE16_HANDLER( atarist_fdc_data_w )
{
	if (fdc.mode & ATARIST_FLOPPY_MODE_SECTOR_COUNT)
	{
		if (data == 0)
		{
			fdc.status &= ~ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO;
		}
		else
		{
			fdc.status |= ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO;
		}

		fdc.sectors = data;
	}
	else
	{
		if (fdc.mode & ATARIST_FLOPPY_MODE_HDC)
		{
			// HDC not implemented
			fdc.status &= ~ATARIST_FLOPPY_STATUS_DMA_ERROR;
		}
		else
		{
			wd17xx_w(machine, (fdc.mode & ATARIST_FLOPPY_MODE_ADDRESS_MASK) >> 1, data);
		}
	}
}

static READ16_HANDLER( atarist_fdc_dma_status_r )
{
	return fdc.status;
}

static WRITE16_HANDLER( atarist_fdc_dma_mode_w )
{
	if ((data & ATARIST_FLOPPY_MODE_WRITE) != (fdc.mode & ATARIST_FLOPPY_MODE_WRITE))
	{
		fdc.status = 0;
		fdc.sectors = 0;
	}

	fdc.mode = data;
}

static READ16_HANDLER( atarist_fdc_dma_base_r )
{
	switch (offset)
	{
	case 0:
		return (fdc.dmabase >> 16) & 0xff;
	case 1:
		return (fdc.dmabase >> 8) & 0xff;
	case 2:
		return fdc.dmabase & 0xff;
	}

	return 0;
}

static WRITE16_HANDLER( atarist_fdc_dma_base_w )
{
	switch (offset)
	{
	case 0:
		fdc.dmabase = (fdc.dmabase & 0x00ffff) | ((data & 0xff) << 16);
		break;
	case 1:
		fdc.dmabase = (fdc.dmabase & 0x0000ff) | ((data & 0xff) << 8);
		break;
	case 2:
		fdc.dmabase = data & 0xff;
		break;
	}

	fdc.dmabytes = ATARIST_FLOPPY_BYTES_PER_SECTOR;
}

/* MMU */

static int mmu;

static READ16_HANDLER( atarist_mmu_r )
{
	return mmu;
}

static WRITE16_HANDLER( atarist_mmu_w )
{
	mmu = data & 0xff;
}

/* IKBD */

static struct IKBD
{
	UINT8 keylatch;
	UINT8 mouse_x, mouse_y;
	UINT8 mouse_px, mouse_py, mouse_pc;
	UINT8 rx, tx;
} ikbd;

static const int IKBD_MOUSE_XYA[3][4] = { { 0, 0, 0, 0 }, { 1, 1, 0, 0 }, { 0, 1, 1, 0 } };
static const int IKBD_MOUSE_XYB[3][4] = { { 0, 0, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 0, 0 } };

enum
{
	IKBD_MOUSE_PHASE_STATIC = 0,
	IKBD_MOUSE_PHASE_POSITIVE,
	IKBD_MOUSE_PHASE_NEGATIVE
};

static READ8_HANDLER( ikbd_port1_r )
{
	/*

        bit     description

        0       Keyboard column input
        1       Keyboard column input
        2       Keyboard column input
        3       Keyboard column input
        4       Keyboard column input
        5       Keyboard column input
        6       Keyboard column input
        7       Keyboard column input

    */

	return ikbd.keylatch;
}

static READ8_HANDLER( ikbd_port2_r )
{
	/*

        bit     description

        0       JOY 1-5
        1       JOY 0-6
        2       JOY 1-6
        3       SD FROM CPU
        4       SD TO CPU

    */

	return (ikbd.tx << 3) | (input_port_read_safe(machine, "IKBD_JOY1", 0xff) & 0x06);
}

static WRITE8_HANDLER( ikbd_port2_w )
{
	/*

        bit     description

        0       JOY 1-5
        1       JOY 0-6
        2       JOY 1-6
        3       SD FROM CPU
        4       SD TO CPU

    */

	ikbd.rx = (data & 0x10) >> 4;
}

static WRITE8_HANDLER( ikbd_port3_w )
{
	/*

        bit     description

        0       CAPS LOCK LED
        1       Keyboard row select
        2       Keyboard row select
        3       Keyboard row select
        4       Keyboard row select
        5       Keyboard row select
        6       Keyboard row select
        7       Keyboard row select

    */

	set_led_status(1, data & 0x01);

	if (~data & 0x02) ikbd.keylatch = input_port_read(machine, "P31");
	if (~data & 0x04) ikbd.keylatch = input_port_read(machine, "P32");
	if (~data & 0x08) ikbd.keylatch = input_port_read(machine, "P33");
	if (~data & 0x10) ikbd.keylatch = input_port_read(machine, "P34");
	if (~data & 0x20) ikbd.keylatch = input_port_read(machine, "P35");
	if (~data & 0x40) ikbd.keylatch = input_port_read(machine, "P36");
	if (~data & 0x80) ikbd.keylatch = input_port_read(machine, "P37");
}

static READ8_HANDLER( ikbd_port4_r )
{
	/*

        bit     description

        0       JOY 0-1 or mouse XB
        1       JOY 0-2 or mouse XA
        2       JOY 0-3 or mouse YA
        3       JOY 0-4 or mouse YB
        4       JOY 1-1
        5       JOY 1-2
        6       JOY 1-3
        7       JOY 1-4

    */

	if (input_port_read(machine, "config") & 0x01)
	{
		/*

                Right   Left        Up      Down

            XA  1100    0110    YA  1100    0110
            XB  0110    1100    YB  0110    1100

        */

		UINT8 data = input_port_read_safe(machine, "IKBD_JOY0", 0xff) & 0xf0;
		UINT8 x = input_port_read_safe(machine, "IKBD_MOUSEX", 0x00);
		UINT8 y = input_port_read_safe(machine, "IKBD_MOUSEY", 0x00);

		if (x == ikbd.mouse_x)
		{
			ikbd.mouse_px = IKBD_MOUSE_PHASE_STATIC;
		}
		else if (x > ikbd.mouse_x)
		{
			ikbd.mouse_px = IKBD_MOUSE_PHASE_POSITIVE;
		}
		else if (x < ikbd.mouse_x)
		{
			ikbd.mouse_px = IKBD_MOUSE_PHASE_NEGATIVE;
		}

		if (y == ikbd.mouse_y)
		{
			ikbd.mouse_py = IKBD_MOUSE_PHASE_STATIC;
		}
		else if (y > ikbd.mouse_y)
		{
			ikbd.mouse_py = IKBD_MOUSE_PHASE_POSITIVE;
		}
		else if (y < ikbd.mouse_y)
		{
			ikbd.mouse_py = IKBD_MOUSE_PHASE_NEGATIVE;
		}

		data |= IKBD_MOUSE_XYB[ikbd.mouse_px][ikbd.mouse_pc];	   // XB
		data |= IKBD_MOUSE_XYA[ikbd.mouse_px][ikbd.mouse_pc] << 1; // XA
		data |= IKBD_MOUSE_XYA[ikbd.mouse_py][ikbd.mouse_pc] << 2; // YA
		data |= IKBD_MOUSE_XYB[ikbd.mouse_py][ikbd.mouse_pc] << 3; // YB

		ikbd.mouse_pc++;

		if (ikbd.mouse_pc == 4)
		{
			ikbd.mouse_pc = 0;
		}

		ikbd.mouse_x = x;
		ikbd.mouse_y = y;

		return data;
	}
	else
	{
		return input_port_read_safe(machine, "IKBD_JOY0", 0xff);
	}
}

static WRITE8_HANDLER( ikbd_port4_w )
{
	/*

        bit     description

        0       Keyboard row select
        1       Keyboard row select
        2       Keyboard row select
        3       Keyboard row select
        4       Keyboard row select
        5       Keyboard row select
        6       Keyboard row select
        7       Keyboard row select

    */

	if (~data & 0x01) ikbd.keylatch = input_port_read(machine, "P40");
	if (~data & 0x02) ikbd.keylatch = input_port_read(machine, "P41");
	if (~data & 0x04) ikbd.keylatch = input_port_read(machine, "P42");
	if (~data & 0x08) ikbd.keylatch = input_port_read(machine, "P43");
	if (~data & 0x10) ikbd.keylatch = input_port_read(machine, "P44");
	if (~data & 0x20) ikbd.keylatch = input_port_read(machine, "P45");
	if (~data & 0x40) ikbd.keylatch = input_port_read(machine, "P46");
	if (~data & 0x80) ikbd.keylatch = input_port_read(machine, "P47");
}

/* DMA Sound */

static struct DMASOUND
{
	UINT32 base, end, cntr;
	UINT32 baselatch, endlatch;
	UINT16 ctrl, mode;
	UINT8 fifo[8];
	UINT8 samples;
	int active;
} dmasound;

static const int DMASOUND_RATE[] = { Y2/640/8, Y2/640/4, Y2/640/2, Y2/640 };

static emu_timer *dmasound_timer;

static void atariste_dmasound_set_state(running_machine *machine, int level)
{
	const device_config *mc68901 = device_list_find_by_tag(machine->config->devicelist, MC68901, MC68901_TAG);

	dmasound.active = level;
	mc68901_tai_w(mc68901, level);

	if (level == 0)
	{
		dmasound.baselatch = dmasound.base;
		dmasound.endlatch = dmasound.end;
	}
	else
	{
		dmasound.cntr = dmasound.baselatch;
	}
}

static TIMER_CALLBACK( atariste_dmasound_tick )
{
	if (dmasound.samples == 0)
	{
		int i;

		for (i = 0; i < 8; i++)
		{
			dmasound.fifo[i] = memory_region(REGION_CPU1)[dmasound.cntr];
			dmasound.cntr++;
			dmasound.samples++;

			if (dmasound.cntr == dmasound.endlatch)
			{
				atariste_dmasound_set_state(machine, 0);
				break;
			}
		}
	}

	if (dmasound.ctrl & 0x80)
	{
//      logerror("DMA sound left  %i\n", dmasound.fifo[7 - dmasound.samples]);
		dmasound.samples--;

//      logerror("DMA sound right %i\n", dmasound.fifo[7 - dmasound.samples]);
		dmasound.samples--;
	}
	else
	{
//      logerror("DMA sound mono %i\n", dmasound.fifo[7 - dmasound.samples]);
		dmasound.samples--;
	}

	if ((dmasound.samples == 0) && (dmasound.active == 0))
	{
		if ((dmasound.ctrl & 0x03) == 0x03)
		{
			atariste_dmasound_set_state(machine, 1);
		}
		else
		{
			timer_enable(dmasound_timer, 0);
		}
	}
}

static READ16_HANDLER( atariste_sound_dma_control_r )
{
	return dmasound.ctrl;
}

static READ16_HANDLER( atariste_sound_dma_base_r )
{
	switch (offset)
	{
	case 0x00:
		return (dmasound.base >> 16) & 0x3f;
	case 0x01:
		return (dmasound.base >> 8) & 0xff;
	case 0x02:
		return dmasound.base & 0xff;
	}

	return 0;
}

static READ16_HANDLER( atariste_sound_dma_counter_r )
{
	switch (offset)
	{
	case 0x00:
		return (dmasound.cntr >> 16) & 0x3f;
	case 0x01:
		return (dmasound.cntr >> 8) & 0xff;
	case 0x02:
		return dmasound.cntr & 0xff;
	}

	return 0;
}

static READ16_HANDLER( atariste_sound_dma_end_r )
{
	switch (offset)
	{
	case 0x00:
		return (dmasound.end >> 16) & 0x3f;
	case 0x01:
		return (dmasound.end >> 8) & 0xff;
	case 0x02:
		return dmasound.end & 0xff;
	}

	return 0;
}

static READ16_HANDLER( atariste_sound_mode_r )
{
	return dmasound.mode;
}

static WRITE16_HANDLER( atariste_sound_dma_control_w )
{
	dmasound.ctrl = data & 0x03;

	if (dmasound.ctrl & 0x01)
	{
		if (!dmasound.active)
		{
			atariste_dmasound_set_state(machine, 1);
			timer_adjust_periodic(dmasound_timer, attotime_zero, 0, ATTOTIME_IN_HZ(DMASOUND_RATE[dmasound.mode & 0x03]));
		}
	}
	else
	{
		atariste_dmasound_set_state(machine, 0);
		timer_enable(dmasound_timer, 0);
	}
}

static WRITE16_HANDLER( atariste_sound_dma_base_w )
{
	switch (offset)
	{
	case 0x00:
		dmasound.base = (data << 16) & 0x3f0000;
		break;
	case 0x01:
		dmasound.base = (dmasound.base & 0x3f00fe) | (data & 0xff) << 8;
		break;
	case 0x02:
		dmasound.base = (dmasound.base & 0x3fff00) | (data & 0xfe);
		break;
	}

	if (!dmasound.active)
	{
		dmasound.baselatch = dmasound.base;
	}
}

static WRITE16_HANDLER( atariste_sound_dma_end_w )
{
	switch (offset)
	{
	case 0x00:
		dmasound.end = (data << 16) & 0x3f0000;
		break;
	case 0x01:
		dmasound.end = (dmasound.end & 0x3f00fe) | (data & 0xff) << 8;
		break;
	case 0x02:
		dmasound.end = (dmasound.end & 0x3fff00) | (data & 0xfe);
		break;
	}

	if (!dmasound.active)
	{
		dmasound.endlatch = dmasound.end;
	}
}

static WRITE16_HANDLER( atariste_sound_mode_w )
{
	dmasound.mode = data & 0x8f;
}

/* Microwire */

static struct MICROWIRE
{
	UINT16 data, mask;
	int shift;
} mwire;

static emu_timer *microwire_timer;

static void atariste_microwire_shift(running_machine *machine)
{
	const device_config *lmc1992 = device_list_find_by_tag(machine->config->devicelist, LMC1992, LMC1992_TAG);

	if (BIT(mwire.mask, 15))
	{
		lmc1992_data_w(lmc1992, BIT(mwire.data, 15));
		lmc1992_clock_w(lmc1992, 1);
		lmc1992_clock_w(lmc1992, 0);
	}

	// rotate mask and data left

	mwire.mask = (mwire.mask << 1) | BIT(mwire.mask, 15);
	mwire.data = (mwire.data << 1) | BIT(mwire.data, 15);
	mwire.shift++;
}

static TIMER_CALLBACK( atariste_microwire_tick )
{
	const device_config *lmc1992 = device_list_find_by_tag(machine->config->devicelist, LMC1992, LMC1992_TAG);

	switch (mwire.shift)
	{
	case 0:
		lmc1992_enable_w(lmc1992, 0);
		atariste_microwire_shift(machine);
		break;

	default:
		atariste_microwire_shift(machine);
		break;

	case 15:
		atariste_microwire_shift(machine);
		lmc1992_enable_w(lmc1992, 1);
		mwire.shift = 0;
		timer_enable(microwire_timer, 0);
		break;
	}
}

static READ16_HANDLER( atariste_microwire_data_r )
{
	return mwire.data;
}

static WRITE16_HANDLER( atariste_microwire_data_w )
{
	if (!timer_enabled(microwire_timer))
	{
		mwire.data = data;
		timer_adjust_periodic(microwire_timer, attotime_zero, 0, ATTOTIME_IN_USEC(2));
	}
}

static READ16_HANDLER( atariste_microwire_mask_r )
{
	return mwire.mask;
}

static WRITE16_HANDLER( atariste_microwire_mask_w )
{
	if (!timer_enabled(microwire_timer))
	{
		mwire.mask = data;
	}
}

/* Mega STe Cache */

static UINT16 megaste_cache;

static READ16_HANDLER( megaste_cache_r )
{
	return megaste_cache;
}

static WRITE16_HANDLER( megaste_cache_w )
{
	megaste_cache = data;
	cpunum_set_clock(machine, 0, (data & 0x01) ? Y2/2 : Y2/4);
}

/* ST Book */

static READ16_HANDLER( stbook_config_r )
{
	/*

        bit     description

        0       _POWER_SWITCH
        1       _TOP_CLOSED
        2       _RTC_ALARM
        3       _SOURCE_DEAD
        4       _SOURCE_LOW
        5       _MODEM_WAKE
        6       (reserved)
        7       _EXPANSION_WAKE
        8       (reserved)
        9       (reserved)
        10      (reserved)
        11      (reserved)
        12      (reserved)
        13      SELF TEST
        14      LOW SPEED FLOPPY
        15      DMA AVAILABLE

    */

	return (input_port_read(machine, "SW400") << 8) | 0xff;
}

static WRITE16_HANDLER( stbook_lcd_control_w )
{
	/*

        bit     description

        0       Shadow Chip OFF
        1       _SHIFTER OFF
        2       POWEROFF
        3       _22ON
        4       RS-232_OFF
        5       (reserved)
        6       (reserved)
        7       MTR_PWR_ON

    */
}

/* Memory Maps */

static ADDRESS_MAP_START( ikbd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_internal_registers_r, hd63701_internal_registers_w)
	AM_RANGE(0x0080, 0x00ff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ikbd_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(HD63701_PORT1, HD63701_PORT1) AM_READ(ikbd_port1_r)
	AM_RANGE(HD63701_PORT2, HD63701_PORT2) AM_READWRITE(ikbd_port2_r, ikbd_port2_w)
	AM_RANGE(HD63701_PORT3, HD63701_PORT3) AM_WRITE(ikbd_port3_w)
	AM_RANGE(HD63701_PORT4, HD63701_PORT4) AM_READWRITE(ikbd_port4_r, ikbd_port4_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( st_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM
	AM_RANGE(0x000008, 0x1fffff) AM_RAMBANK(1)
	AM_RANGE(0x200000, 0x3fffff) AM_RAMBANK(2)
	AM_RANGE(0xfa0000, 0xfbffff) AM_ROMBANK(3)
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READ(atarist_shifter_counter_r)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atarist_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_READWRITE8(AY8910_read_port_0_r, AY8910_control_port_0_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_WRITE8(AY8910_write_port_0_w, 0xff00)
	AM_RANGE(0xfffa00, 0xfffa2f) AM_DEVREADWRITE8(MC68901, MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
	AM_RANGE(0xfffc00, 0xfffc01) AM_READWRITE8(acia6850_0_stat_r, acia6850_0_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_READWRITE8(acia6850_0_data_r, acia6850_0_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_READWRITE8(acia6850_1_stat_r, acia6850_1_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_READWRITE8(acia6850_1_data_r, acia6850_1_data_w, 0xff00)
ADDRESS_MAP_END

static ADDRESS_MAP_START( megast_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM
	AM_RANGE(0x000008, 0x1fffff) AM_RAMBANK(1)
	AM_RANGE(0x200000, 0x3fffff) AM_RAMBANK(2)
	AM_RANGE(0xfa0000, 0xfbffff) AM_ROMBANK(3)
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff7f30, 0xff7f31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w) // for TOS 1.02
	AM_RANGE(0xff8000, 0xff8007) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READ(atarist_shifter_counter_r)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atarist_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_READWRITE8(AY8910_read_port_0_r, AY8910_control_port_0_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_WRITE8(AY8910_write_port_0_w, 0xff00)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xfffa00, 0xfffa3f) AM_DEVREADWRITE8(MC68901, MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
//  AM_RANGE(0xfffa40, 0xfffa57) AM_READWRITE(megast_fpu_r, megast_fpu_w)
	AM_RANGE(0xfffc00, 0xfffc01) AM_READWRITE8(acia6850_0_stat_r, acia6850_0_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_READWRITE8(acia6850_0_data_r, acia6850_0_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_READWRITE8(acia6850_1_stat_r, acia6850_1_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_READWRITE8(acia6850_1_data_r, acia6850_1_data_w, 0xff00)
	AM_RANGE(0xfffc20, 0xfffc3f) AM_READWRITE(rp5c15_r, rp5c15_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ste_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM
	AM_RANGE(0x000008, 0x1fffff) AM_RAMBANK(1)
	AM_RANGE(0x200000, 0x3fffff) AM_RAMBANK(2)
	AM_RANGE(0xe00000, 0xefffff) AM_ROM
	AM_RANGE(0xfa0000, 0xfbffff) AM_ROMBANK(3)
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READWRITE(atariste_shifter_counter_r, atariste_shifter_counter_w)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff820c, 0xff820d) AM_READWRITE(atariste_shifter_base_low_r, atariste_shifter_base_low_w)
	AM_RANGE(0xff820e, 0xff820f) AM_READWRITE(atariste_shifter_lineofs_r, atariste_shifter_lineofs_w)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atariste_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8264, 0xff8265) AM_READWRITE(atariste_shifter_pixelofs_r, atariste_shifter_pixelofs_w)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_READWRITE8(AY8910_read_port_0_r, AY8910_control_port_0_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_WRITE8(AY8910_write_port_0_w, 0xff00)
	AM_RANGE(0xff8900, 0xff8901) AM_READWRITE(atariste_sound_dma_control_r, atariste_sound_dma_control_w)
	AM_RANGE(0xff8902, 0xff8907) AM_READWRITE(atariste_sound_dma_base_r, atariste_sound_dma_base_w)
	AM_RANGE(0xff8908, 0xff890d) AM_READ(atariste_sound_dma_counter_r)
	AM_RANGE(0xff890e, 0xff8913) AM_READWRITE(atariste_sound_dma_end_r, atariste_sound_dma_end_w)
	AM_RANGE(0xff8920, 0xff8921) AM_READWRITE(atariste_sound_mode_r, atariste_sound_mode_w)
	AM_RANGE(0xff8922, 0xff8923) AM_READWRITE(atariste_microwire_data_r, atariste_microwire_data_w)
	AM_RANGE(0xff8924, 0xff8925) AM_READWRITE(atariste_microwire_mask_r, atariste_microwire_mask_w)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xff9200, 0xff9201) AM_READ_PORT("JOY0")
	AM_RANGE(0xff9202, 0xff9203) AM_READ_PORT("JOY1")
	AM_RANGE(0xff9210, 0xff9211) AM_READ_PORT("PADDLE0X")
	AM_RANGE(0xff9212, 0xff9213) AM_READ_PORT("PADDLE0Y")
	AM_RANGE(0xff9214, 0xff9215) AM_READ_PORT("PADDLE1X")
	AM_RANGE(0xff9216, 0xff9217) AM_READ_PORT("PADDLE1Y")
	AM_RANGE(0xff9220, 0xff9221) AM_READ_PORT("GUNX")
	AM_RANGE(0xff9222, 0xff9223) AM_READ_PORT("GUNY")
	AM_RANGE(0xfffa00, 0xfffa2f) AM_DEVREADWRITE8(MC68901, MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
	AM_RANGE(0xfffc00, 0xfffc01) AM_READWRITE8(acia6850_0_stat_r, acia6850_0_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_READWRITE8(acia6850_0_data_r, acia6850_0_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_READWRITE8(acia6850_1_stat_r, acia6850_1_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_READWRITE8(acia6850_1_data_r, acia6850_1_data_w, 0xff00)
ADDRESS_MAP_END

static ADDRESS_MAP_START( megaste_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM
	AM_RANGE(0x000008, 0x1fffff) AM_RAMBANK(1)
	AM_RANGE(0x200000, 0x3fffff) AM_RAMBANK(2)
	AM_RANGE(0xe00000, 0xefffff) AM_ROM
	AM_RANGE(0xfa0000, 0xfbffff) AM_ROMBANK(3)
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff8000, 0xff8007) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READWRITE(atariste_shifter_counter_r, atariste_shifter_counter_w)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff820c, 0xff820d) AM_READWRITE(atariste_shifter_base_low_r, atariste_shifter_base_low_w)
	AM_RANGE(0xff820e, 0xff820f) AM_READWRITE(atariste_shifter_lineofs_r, atariste_shifter_lineofs_w)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atariste_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8264, 0xff8265) AM_READWRITE(atariste_shifter_pixelofs_r, atariste_shifter_pixelofs_w)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_READWRITE8(AY8910_read_port_0_r, AY8910_control_port_0_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_WRITE8(AY8910_write_port_0_w, 0xff00)
	AM_RANGE(0xff8900, 0xff8901) AM_READWRITE(atariste_sound_dma_control_r, atariste_sound_dma_control_w)
	AM_RANGE(0xff8902, 0xff8907) AM_READWRITE(atariste_sound_dma_base_r, atariste_sound_dma_base_w)
	AM_RANGE(0xff8908, 0xff890d) AM_READ(atariste_sound_dma_counter_r)
	AM_RANGE(0xff890e, 0xff8913) AM_READWRITE(atariste_sound_dma_end_r, atariste_sound_dma_end_w)
	AM_RANGE(0xff8920, 0xff8921) AM_READWRITE(atariste_sound_mode_r, atariste_sound_mode_w)
	AM_RANGE(0xff8922, 0xff8923) AM_READWRITE(atariste_microwire_data_r, atariste_microwire_data_w)
	AM_RANGE(0xff8924, 0xff8925) AM_READWRITE(atariste_microwire_mask_r, atariste_microwire_mask_w)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xff8e20, 0xff8e21) AM_READWRITE(megaste_cache_r, megaste_cache_w)
	AM_RANGE(0xfffa00, 0xfffa3f) AM_DEVREADWRITE8(MC68901, MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
//  AM_RANGE(0xfffa40, 0xfffa5f) AM_READWRITE(megast_fpu_r, megast_fpu_w)
	AM_RANGE(0xff8c80, 0xff8c87) AM_READWRITE8(scc_r, scc_w, 0xff00)
	AM_RANGE(0xfffc00, 0xfffc01) AM_READWRITE8(acia6850_0_stat_r, acia6850_0_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_READWRITE8(acia6850_0_data_r, acia6850_0_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_READWRITE8(acia6850_1_stat_r, acia6850_1_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_READWRITE8(acia6850_1_data_r, acia6850_1_data_w, 0xff00)
	AM_RANGE(0xfffc20, 0xfffc3f) AM_READWRITE(rp5c15_r, rp5c15_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( stbook_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM
	AM_RANGE(0x000008, 0x1fffff) AM_RAMBANK(1)
	AM_RANGE(0x200000, 0x3fffff) AM_RAMBANK(2)
	AM_RANGE(0xd40000, 0xd7ffff) AM_ROM
	AM_RANGE(0xe00000, 0xe7ffff) AM_ROM
	AM_RANGE(0xe80000, 0xebffff) AM_ROM
//  AM_RANGE(0xf00000, 0xf1ffff) AM_READWRITE(stbook_ide_r, stbook_ide_w)
	AM_RANGE(0xfa0000, 0xfbffff) AM_ROMBANK(3)
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
/*  AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(stbook_mmu_r, stbook_mmu_w)
    AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(stbook_shifter_base_r, stbook_shifter_base_w)
    AM_RANGE(0xff8204, 0xff8209) AM_READWRITE(stbook_shifter_counter_r, stbook_shifter_counter_w)
    AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(stbook_shifter_sync_r, stbook_shifter_sync_w, 0xff00)
    AM_RANGE(0xff820c, 0xff820d) AM_READWRITE(stbook_shifter_base_low_r, stbook_shifter_base_low_w)
    AM_RANGE(0xff820e, 0xff820f) AM_READWRITE(stbook_shifter_lineofs_r, stbook_shifter_lineofs_w)
    AM_RANGE(0xff8240, 0xff8241) AM_READWRITE(stbook_shifter_palette_r, stbook_shifter_palette_w)
    AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(stbook_shifter_mode_r, stbook_shifter_mode_w, 0xff00)
    AM_RANGE(0xff8264, 0xff8265) AM_READWRITE(stbook_shifter_pixelofs_r, stbook_shifter_pixelofs_w)*/
	AM_RANGE(0xff827e, 0xff827f) AM_WRITE(stbook_lcd_control_w)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_READWRITE8(AY8910_read_port_0_r, AY8910_control_port_0_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_WRITE8(AY8910_write_port_0_w, 0xff00)
	AM_RANGE(0xff8900, 0xff8901) AM_READWRITE(atariste_sound_dma_control_r, atariste_sound_dma_control_w)
	AM_RANGE(0xff8902, 0xff8907) AM_READWRITE(atariste_sound_dma_base_r, atariste_sound_dma_base_w)
	AM_RANGE(0xff8908, 0xff890d) AM_READ(atariste_sound_dma_counter_r)
	AM_RANGE(0xff890e, 0xff8913) AM_READWRITE(atariste_sound_dma_end_r, atariste_sound_dma_end_w)
	AM_RANGE(0xff8920, 0xff8921) AM_READWRITE(atariste_sound_mode_r, atariste_sound_mode_w)
	AM_RANGE(0xff8922, 0xff8923) AM_READWRITE(atariste_microwire_data_r, atariste_microwire_data_w)
	AM_RANGE(0xff8924, 0xff8925) AM_READWRITE(atariste_microwire_mask_r, atariste_microwire_mask_w)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xff9200, 0xff9201) AM_READ(stbook_config_r)
/*  AM_RANGE(0xff9202, 0xff9203) AM_READWRITE(stbook_lcd_contrast_r, stbook_lcd_contrast_w)
    AM_RANGE(0xff9210, 0xff9211) AM_READWRITE(stbook_power_r, stbook_power_w)
    AM_RANGE(0xff9214, 0xff9215) AM_READWRITE(stbook_reference_r, stbook_reference_w)*/
	AM_RANGE(0xfffa00, 0xfffa2f) AM_DEVREADWRITE8(MC68901, MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
	AM_RANGE(0xfffc00, 0xfffc01) AM_READWRITE8(acia6850_0_stat_r, acia6850_0_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_READWRITE8(acia6850_0_data_r, acia6850_0_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_READWRITE8(acia6850_1_stat_r, acia6850_1_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_READWRITE8(acia6850_1_data_r, acia6850_1_data_w, 0xff00)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( ikbd )
	PORT_START_TAG("P31")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0xef, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P32")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("F1")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0xde, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P33")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("F2")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(DEF_STR( Alternate )) PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0xbe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P34")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_NAME("F3")
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START_TAG("P35")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_NAME("F4")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')

	PORT_START_TAG("P36")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_NAME("F5")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START_TAG("P37")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F6) PORT_NAME("F6")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START_TAG("P40")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F7) PORT_NAME("F7")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START_TAG("P41")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F8) PORT_NAME("F8")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START_TAG("P42")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F9) PORT_NAME("F9")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START_TAG("P43")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F10) PORT_NAME("F10")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00B4) PORT_CHAR('`')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START_TAG("P44")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Help") PORT_CODE(KEYCODE_F11)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Insert") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START_TAG("P45")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Undo") PORT_CODE(KEYCODE_F12)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Clr Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)

	PORT_START_TAG("P46")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad (")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad )")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)

	PORT_START_TAG("P47")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad /") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad *") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad Enter") PORT_CODE(KEYCODE_ENTER_PAD)
INPUT_PORTS_END

static INPUT_PORTS_START( atarist )
	PORT_START_TAG("config")
	PORT_CONFNAME( 0x01, 0x00, "Input Port 0 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x01, DEF_STR( Joystick ) )
	PORT_CONFNAME( 0x80, 0x80, "Monitor")
	PORT_CONFSETTING( 0x00, "Monochrome" )
	PORT_CONFSETTING( 0x80, "Color" )

	PORT_INCLUDE( ikbd )

	PORT_START_TAG("IKBD_JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY // XB
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY // XA
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY // YA
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY // YB
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY

	PORT_START_TAG("IKBD_JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IKBD_MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)

	PORT_START_TAG("IKBD_MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)
INPUT_PORTS_END

static INPUT_PORTS_START( atariste )
	PORT_START_TAG("config")
	PORT_CONFNAME( 0x01, 0x00, "Input Port 0 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x01, DEF_STR( Joystick ) )
	PORT_CONFNAME( 0x80, 0x80, "Monitor")
	PORT_CONFSETTING( 0x00, "Monochrome" )
	PORT_CONFSETTING( 0x80, "Color" )

	PORT_INCLUDE( ikbd )

	PORT_START_TAG("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("JOY1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(4) PORT_8WAY

	PORT_START_TAG("PADDLE0X")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START_TAG("PADDLE0Y")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START_TAG("PADDLE1X")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START_TAG("PADDLE1Y")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START_TAG("GUNX") // should be 10-bit
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("GUNY") // should be 10-bit
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

static INPUT_PORTS_START( stbook )
	PORT_START_TAG("SW400")
	PORT_DIPNAME( 0x80, 0x80, "DMA sound hardware")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, "WD1772 FDC")
	PORT_DIPSETTING( 0x40, "Low Speed (8 MHz)" )
	PORT_DIPSETTING( 0x00, "High Speed (16 MHz)" )
	PORT_DIPNAME( 0x20, 0x00, "Bypass Self Test")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/* Sound Interface */

static WRITE8_HANDLER( ym2149_port_a_w )
{
	wd17xx_set_side((data & 0x01) ? 0 : 1);

	if (!(data & 0x02))
	{
		wd17xx_set_drive(0);
	}

	if (!(data & 0x04))
	{
		wd17xx_set_drive(1);
	}

	// 0x08 = RTS
	// 0x10 = DTR

	centronics_write_handshake(0, (data & 0x20) ? 0 : CENTRONICS_STROBE, CENTRONICS_STROBE);

	// 0x40 = General Purpose Output
	// 0x80 = Reserved
}

static WRITE8_HANDLER( ym2149_port_b_w )
{
	centronics_write_data(0, data);
}

static const struct AY8910interface ym2149_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	0,
	0,
	ym2149_port_a_w,
	ym2149_port_b_w
};

/* Machine Drivers */

static int acia_irq;
static UINT8 acia_midi_rx = 1, acia_midi_tx = 1;

static void acia_interrupt(int state)
{
	acia_irq = state;
}

static const struct acia6850_interface acia_ikbd_intf =
{
	Y2/64,
	Y2/64,
	&ikbd.rx,
	&ikbd.tx,
	NULL,
	NULL,
	NULL,
	acia_interrupt
};

static const struct acia6850_interface acia_midi_intf =
{
	Y2/64,
	Y2/64,
	&acia_midi_rx,
	&acia_midi_tx,
	NULL,
	NULL,
	NULL,
	acia_interrupt
};

static MC68901_GPIO_READ( mfp_gpio_r )
{
	/*

        bit     description

        0       Centronics BUSY
        1       RS232 DCD
        2       RS232 CTS
        3       Blitter done
        4       Keyboard/MIDI
        5       FDC
        6       RS232 RI
        7       Monochrome monitor detect

    */

	UINT8 data = (centronics_read_handshake(0) & CENTRONICS_NOT_BUSY) >> 7;

	mc68901_tai_w(device, data & 0x01);

	data |= (acia_irq << 4);
	data |= (fdc.irq << 5);
	data |= (input_port_read(device->machine, "config") & 0x80);

	return data;
}

static IRQ_CALLBACK( atarist_int_ack )
{
	const device_config *mc68901 = device_list_find_by_tag(machine->config->devicelist, MC68901, MC68901_TAG);

	if (irqline == MC68000_IRQ_6)
	{
		return mc68901_get_vector(mc68901);
	}

	return MC68000_INT_ACK_AUTOVECTOR;
}

static MC68901_ON_IRQ_CHANGED( mfp_interrupt )
{
	cpunum_set_input_line(device->machine, 0, MC68000_IRQ_6, level);
}

static UINT8 mfp_rx, mfp_tx;

static MC68901_INTERFACE( mfp_intf )
{
	Y2/8,
	Y1,
	MC68901_TDO_LOOPBACK,
	MC68901_TDO_LOOPBACK,
	&mfp_rx,
	&mfp_tx,
	NULL,
	mfp_interrupt,
	mfp_gpio_r,
	NULL
};

static const CENTRONICS_CONFIG atarist_centronics_config[1] =
{
	{
		PRINTER_IBM,
		NULL
	}
};

static void atarist_configure_memory(running_machine *machine)
{
	switch (mess_ram_size)
	{
	case 256 * 1024:
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x000008, 0x03ffff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x040000, 0x3fffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;
	case 512 * 1024:
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x000008, 0x07ffff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x080000, 0x3fffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;
	case 1024 * 1024:
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x000008, 0x0fffff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x100000, 0x3fffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;
	case 2048 * 1024:
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x000008, 0x1fffff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x3fffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;
	case 4096 * 1024:
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x000008, 0x1fffff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x3fffff, 0, 0, SMH_BANK2, SMH_BANK2);
		break;
	}

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x000008, 0);
	memory_set_bank(1, 0);

	memory_configure_bank(2, 0, 1, memory_region(REGION_CPU1) + 0x200000, 0);
	memory_set_bank(2, 0);

	memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfa0000, 0xfbffff, 0, 0, SMH_UNMAP, SMH_UNMAP);

	memory_configure_bank(3, 0, 1, memory_region(REGION_CPU1) + 0xfa0000, 0);
	memory_set_bank(3, 0);
}

static void atarist_state_save(void)
{
	memset(&fdc, 0, sizeof(fdc));

	fdc.status |= ATARIST_FLOPPY_STATUS_DMA_ERROR;

	memset(&ikbd, 0, sizeof(ikbd));

	state_save_register_global(mmu);
	state_save_register_global(fdc.dmabase);
	state_save_register_global(fdc.status);
	state_save_register_global(fdc.mode);
	state_save_register_global(fdc.sectors);
	state_save_register_global(fdc.dmabytes);
	state_save_register_global(fdc.irq);
	state_save_register_global(ikbd.keylatch);
	state_save_register_global(ikbd.mouse_x);
	state_save_register_global(ikbd.mouse_y);
	state_save_register_global(ikbd.mouse_px);
	state_save_register_global(ikbd.mouse_py);
	state_save_register_global(ikbd.mouse_pc);
	state_save_register_global(ikbd.rx);
	state_save_register_global(ikbd.tx);
	state_save_register_global(acia_irq);
	state_save_register_global(acia_midi_rx);
	state_save_register_global(acia_midi_tx);
	state_save_register_global(mfp_rx);
	state_save_register_global(mfp_tx);
}

static MACHINE_START( atarist )
{
	atarist_configure_memory(machine);
	atarist_state_save();

	centronics_config(0, atarist_centronics_config);
	wd17xx_init(machine, WD_TYPE_1772, atarist_fdc_callback, NULL);
	acia6850_config(0, &acia_ikbd_intf);
	acia6850_config(1, &acia_midi_intf);

	cpunum_set_irq_callback(0, atarist_int_ack);
}

static const struct rp5c15_interface rtc_intf =
{
	NULL
};

static MACHINE_START( megast )
{
	MACHINE_START_CALL(atarist);
	rp5c15_init(machine, &rtc_intf);
}

static MC68901_GPIO_READ( atariste_mfp_gpio_r )
{
	/*

        bit     description

        0       Centronics BUSY
        1       RS232 DCD
        2       RS232 CTS
        3       Blitter done
        4       Keyboard/MIDI
        5       FDC
        6       RS232 RI
        7       Monochrome monitor detect / DMA sound active

    */

	UINT8 data = (centronics_read_handshake(0) & CENTRONICS_NOT_BUSY) >> 7;

	data |= (acia_irq << 4);
	data |= (fdc.irq << 5);
	data |= (input_port_read(device->machine, "config") & 0x80) ^ (dmasound.active << 7);

	return data;
}

static MC68901_INTERFACE( atariste_mfp_intf )
{
	Y2/8,
	Y1,
	MC68901_TDO_LOOPBACK,
	MC68901_TDO_LOOPBACK,
	&mfp_rx,
	&mfp_tx,
	NULL,
	mfp_interrupt,
	atariste_mfp_gpio_r,
	NULL
};

static void atariste_state_save(void)
{
	atarist_state_save();

	memset(&mwire, 0, sizeof(mwire));
	memset(&dmasound, 0, sizeof(dmasound));

	state_save_register_global(dmasound.base);
	state_save_register_global(dmasound.end);
	state_save_register_global(dmasound.cntr);
	state_save_register_global(dmasound.baselatch);
	state_save_register_global(dmasound.endlatch);
	state_save_register_global(dmasound.ctrl);
	state_save_register_global(dmasound.mode);
	state_save_register_global_array(dmasound.fifo);
	state_save_register_global(dmasound.samples);
	state_save_register_global(dmasound.active);

	state_save_register_global(mwire.data);
	state_save_register_global(mwire.mask);
	state_save_register_global(mwire.shift);
}

static MACHINE_START( atariste )
{
	atarist_configure_memory(machine);
	atariste_state_save();

	centronics_config(0, atarist_centronics_config);
	wd17xx_init(machine, WD_TYPE_1772, atarist_fdc_callback, NULL);
	acia6850_config(0, &acia_ikbd_intf);
	acia6850_config(1, &acia_midi_intf);

	cpunum_set_irq_callback(0, atarist_int_ack);

	dmasound_timer = timer_alloc(atariste_dmasound_tick, NULL);
	microwire_timer = timer_alloc(atariste_microwire_tick, NULL);
}

static MACHINE_START( megaste )
{
	machine_start_atariste(machine);
	state_save_register_global(megaste_cache);
	rp5c15_init(machine, &rtc_intf);
}

static void stbook_configure_memory(running_machine *machine)
{
	switch (mess_ram_size)
	{
	case 1024 * 1024:
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x000008, 0x07ffff, 0, 0x080000, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x100000, 0x3fffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;
	case 4096 * 1024:
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x000008, 0x1fffff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x3fffff, 0, 0, SMH_BANK2, SMH_BANK2);
		break;
	}

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x000008, 0);
	memory_set_bank(1, 0);

	memory_configure_bank(2, 0, 1, memory_region(REGION_CPU1) + 0x200000, 0);
	memory_set_bank(2, 0);

	memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfa0000, 0xfbffff, 0, 0, SMH_UNMAP, SMH_UNMAP);

	memory_configure_bank(3, 0, 1, memory_region(REGION_CPU1) + 0xfa0000, 0);
	memory_set_bank(3, 0);
}

static WRITE8_HANDLER( stbook_ym2149_port_a_w )
{
	wd17xx_set_side((data & 0x01) ? 0 : 1);

	if (!(data & 0x02))
	{
		wd17xx_set_drive(0);
	}

	if (!(data & 0x04))
	{
		wd17xx_set_drive(1);
	}

	// 0x08 = RTS
	// 0x10 = DTR

	centronics_write_handshake(0, (data & 0x20) ? 0 : CENTRONICS_STROBE, CENTRONICS_STROBE);

	// 0x40 = IDE RESET
	// 0x80 = FDD_DENSE_SEL
}

static const struct AY8910interface stbook_ym2149_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	0,
	0,
	stbook_ym2149_port_a_w,
	ym2149_port_b_w
};

static UINT8 krxd, ktxd;

static const struct acia6850_interface stbook_acia_ikbd_intf =
{
	U517/2/16, // 500kHz
	U517/2/2, // 1MHZ
	&krxd,
	&ktxd,
	NULL,
	NULL,
	NULL,
	acia_interrupt
};

static MC68901_GPIO_READ( stbook_mfp_gpio_r )
{
	/*

        bit     description

        0       Centronics BUSY
        1       RS232 DCD
        2       RS232 CTS
        3       Blitter done
        4       Keyboard/MIDI
        5       FDC
        6       RS232 RI
        7       POWER ALARMS

    */

	UINT8 data = (centronics_read_handshake(0) & CENTRONICS_NOT_BUSY) >> 7;

	data |= (acia_irq << 4);
	data |= (fdc.irq << 5);

	return data;
}

static MC68901_INTERFACE( stbook_mfp_intf )
{
	Y2/8,
	Y1,
	MC68901_TDO_LOOPBACK,
	MC68901_TDO_LOOPBACK,
	&mfp_rx,
	&mfp_tx,
	NULL,
	mfp_interrupt,
	stbook_mfp_gpio_r,
	NULL
};

static MACHINE_START( stbook )
{
	stbook_configure_memory(machine);
	atariste_state_save();

	state_save_register_global(krxd);
	state_save_register_global(ktxd);

	centronics_config(0, atarist_centronics_config);
	wd17xx_init(machine, WD_TYPE_1772, atarist_fdc_callback, NULL);
	acia6850_config(0, &stbook_acia_ikbd_intf);
	acia6850_config(1, &acia_midi_intf);
	rp5c15_init(machine, &rtc_intf);

	cpunum_set_irq_callback(0, atarist_int_ack);
}

static MACHINE_DRIVER_START( atarist )
	MDRV_DRIVER_DATA(atarist_state)

	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M68000, Y2/4)
	MDRV_CPU_PROGRAM_MAP(st_map, 0)

	MDRV_CPU_ADD(HD63701, XTAL_4MHz)  /* HD6301 */
	MDRV_CPU_PROGRAM_MAP(ikbd_map, 0)
	MDRV_CPU_IO_MAP(ikbd_io_map, 0)

	MDRV_MACHINE_START(atarist)

	// device hardware

	MDRV_DEVICE_ADD(MC68901_TAG, MC68901)
	MDRV_DEVICE_CONFIG(mfp_intf)

	// video hardware
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_PALETTE_LENGTH(16)
	MDRV_VIDEO_START( atarist )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SCREEN_RAW_PARAMS(Y2/4, ATARIST_HTOT_PAL, ATARIST_HBEND_PAL, ATARIST_HBSTART_PAL, ATARIST_VTOT_PAL, ATARIST_VBEND_PAL, ATARIST_VBSTART_PAL)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2149, Y2/16)
	MDRV_SOUND_CONFIG(ym2149_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( megast )
	MDRV_IMPORT_FROM(atarist)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(megast_map, 0)

	MDRV_MACHINE_START(megast)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( atariste )
	MDRV_DRIVER_DATA(atarist_state)

	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M68000, Y2/4)
	MDRV_CPU_PROGRAM_MAP(ste_map, 0)

	MDRV_CPU_ADD(HD63701, XTAL_4MHz)  /* HD6301 */
	MDRV_CPU_PROGRAM_MAP(ikbd_map, 0)
	MDRV_CPU_IO_MAP(ikbd_io_map, 0)

	MDRV_MACHINE_START(atariste)

	// device hardware

	MDRV_DEVICE_ADD(MC68901_TAG, MC68901)
	MDRV_DEVICE_CONFIG(atariste_mfp_intf)

	// video hardware
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_PALETTE_LENGTH(512)
	MDRV_VIDEO_START( atarist )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SCREEN_RAW_PARAMS(Y2/4, ATARIST_HTOT_PAL, ATARIST_HBEND_PAL, ATARIST_HBSTART_PAL, ATARIST_VTOT_PAL, ATARIST_VBEND_PAL, ATARIST_VBSTART_PAL)

	// sound hardware
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2149, Y2/16)
	MDRV_SOUND_CONFIG(ym2149_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(0, "right", 0.50)
/*
    MDRV_SOUND_ADD(CUSTOM, 0) // DAC
    MDRV_SOUND_ROUTE(0, "right", 0.50)
    MDRV_SOUND_ROUTE(1, "left", 0.50)
*/
	MDRV_DEVICE_ADD(LMC1992_TAG, LMC1992)
//	MDRV_DEVICE_CONFIG(atariste_lmc1992_intf)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( megaste )
	MDRV_IMPORT_FROM(atariste)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(megaste_map, 0)

	MDRV_MACHINE_START(megaste)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( stbook )
	MDRV_DRIVER_DATA(atarist_state)

	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M68000, U517/2)
	MDRV_CPU_PROGRAM_MAP(stbook_map, 0)

	//MDRV_CPU_ADD(COP888, Y700)

	MDRV_MACHINE_START(stbook)

	// device hardware

	MDRV_DEVICE_ADD(MC68901_TAG, MC68901)
	MDRV_DEVICE_CONFIG(stbook_mfp_intf)

	// video hardware
	MDRV_SCREEN_ADD("main", LCD)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 399)
	MDRV_PALETTE_LENGTH(2)

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )
	MDRV_PALETTE_INIT( black_and_white )

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM3439, U517/8)
	MDRV_SOUND_CONFIG(stbook_ym2149_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( atarist )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.img", 0xfc0000, 0x030000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102.img", 0xfc0000, 0x030000, BAD_DUMP CRC(3b5cd0c5) SHA1(87900a40a890fdf03bd08be6c60cc645855cbce5), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos100", "TOS 1.0 (ROM TOS)" )
	ROMX_LOAD( "tos100.img", 0xfc0000, 0x030000, BAD_DUMP CRC(1a586c64) SHA1(9a6e4c88533a9eaa4d55cdc040e47443e0226eb2), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "tos099", "TOS 0.99 (Disk TOS)" )
	ROMX_LOAD( "tos099.img", 0xfc0000, 0x008000, NO_DUMP, ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4, "tos100de", "TOS 1.0 (ROM TOS) (Germany)" )
	ROMX_LOAD( "st_7c1_a4.u4", 0xfc0000, 0x008000, CRC(867fdd7e) SHA1(320d12acf510301e6e9ab2e3cf3ee60b0334baa0), ROM_SKIP(1) | ROM_BIOS(5) )
	ROMX_LOAD( "st_7c1_a9.u7", 0xfc0001, 0x008000, CRC(30e8f982) SHA1(253f26ff64b202b2681ab68ffc9954125120baea), ROM_SKIP(1) | ROM_BIOS(5) )
	ROMX_LOAD( "st_7c1_b0.u3", 0xfd0000, 0x008000, CRC(b91337ed) SHA1(21a338f9bbd87bce4a12d38048e03a361f58d33e), ROM_SKIP(1) | ROM_BIOS(5) )
	ROMX_LOAD( "st_7a4_a6.u6", 0xfd0001, 0x008000, CRC(969d7bbe) SHA1(72b998c1f25211c2a96c81a038d71b6a390585c2), ROM_SKIP(1) | ROM_BIOS(5) )
	ROMX_LOAD( "st_7c1_a2.u2", 0xfe0000, 0x008000, CRC(d0513329) SHA1(49855a3585e2f75b2af932dd4414ed64e6d9501f), ROM_SKIP(1) | ROM_BIOS(5) )
	ROMX_LOAD( "st_7c1_b1.u5", 0xfe0001, 0x008000, CRC(c115cbc8) SHA1(2b52b81a1a4e0818d63f98ee4b25c30e2eba61cb), ROM_SKIP(1) | ROM_BIOS(5) )

	ROM_COPY( REGION_CPU1, 0xfc0000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.img", 0xfc0000, 0x030000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102.img", 0xfc0000, 0x030000, BAD_DUMP CRC(3b5cd0c5) SHA1(87900a40a890fdf03bd08be6c60cc645855cbce5), ROM_BIOS(2) )
	ROM_COPY( REGION_CPU1, 0xfc0000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( stacy )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.img", 0xfc0000, 0x030000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(1) )
	ROM_COPY( REGION_CPU1, 0xfc0000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( atariste )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "TOS 1.62 (STE TOS, Revision 2)" )
	ROMX_LOAD( "tos162.img", 0xe00000, 0x040000, BAD_DUMP CRC(d1c6f2fa) SHA1(70db24a7c252392755849f78940a41bfaebace71), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106.img", 0xe00000, 0x040000, BAD_DUMP CRC(d72fea29) SHA1(06f9ea322e74b682df0396acfaee8cb4d9c90cad), ROM_BIOS(2) )
	ROM_COPY( REGION_CPU1, 0xe00000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206.img", 0xe00000, 0x040000, BAD_DUMP CRC(08538e39) SHA1(2400ea95f547d6ea754a99d05d8530c03f8b28e3), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205.img", 0xe00000, 0x030000, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos202", "TOS 2.02 (Mega STE TOS)" )
	ROMX_LOAD( "tos202.img", 0xe00000, 0x030000, NO_DUMP, ROM_BIOS(3) )
	ROM_COPY( REGION_CPU1, 0xe00000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( stbook )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "tos208", "TOS 2.08" )
	ROMX_LOAD( "tos208.img", 0xe00000, 0x040000, NO_DUMP, ROM_BIOS(1) )
	ROM_COPY( REGION_CPU1, 0xe00000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "cop888c0.u703", 0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( stpad )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05" )
	ROMX_LOAD( "tos205.img", 0xe00000, 0x040000, NO_DUMP, ROM_BIOS(1) )
	ROM_COPY( REGION_CPU1, 0xe00000, 0x000000, 0x000008 )
ROM_END

ROM_START( tt030 )
	ROM_REGION32_BE( 0x4000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "TOS 3.06 (TT TOS)" )
	ROMX_LOAD( "tos306.img", 0xe00000, 0x080000, BAD_DUMP CRC(75dda215) SHA1(6325bdfd83f1b4d3afddb2b470a19428ca79478b), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos305", "TOS 3.05 (TT TOS)" )
	ROMX_LOAD( "tos305.img", 0xe00000, 0x080000, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos301", "TOS 3.01 (TT TOS)" )
	ROMX_LOAD( "tos301.img", 0xe00000, 0x080000, NO_DUMP, ROM_BIOS(3) )
	ROM_COPY( REGION_CPU1, 0xe00000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( fx1 )
	ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "tos207", "TOS 2.07" )
	ROMX_LOAD( "tos207.img", 0xe00000, 0x040000, NO_DUMP, ROM_BIOS(1) )
	ROM_COPY( REGION_CPU1, 0xe00000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( falcon )
	ROM_REGION32_BE( 0x4000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "TOS 4.04" )
	ROMX_LOAD( "tos404.img", 0xe00000, 0x080000, BAD_DUMP CRC(028b561d) SHA1(27dcdb31b0951af99023b2fb8c370d8447ba6ebc), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos402", "TOS 4.02" )
	ROMX_LOAD( "tos402.img", 0xe00000, 0x080000, BAD_DUMP CRC(63f82f23) SHA1(75de588f6bbc630fa9c814f738195da23b972cc6), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos401", "TOS 4.01" )
	ROMX_LOAD( "tos401.img", 0xe00000, 0x080000, NO_DUMP, ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "tos400", "TOS 4.00" )
	ROMX_LOAD( "tos400.img", 0xe00000, 0x080000, BAD_DUMP CRC(1fbc5396) SHA1(d74d09f11a0bf37a86ccb50c6e7f91aac4d4b11b), ROM_BIOS(4) )
	ROM_COPY( REGION_CPU1, 0xe00000, 0x000000, 0x000008 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0xf000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( falcon40 )
	ROM_REGION32_BE( 0x4000000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "tos492", "TOS 4.92" )
	ROMX_LOAD( "tos492.img", 0xe00000, 0x080000, BAD_DUMP CRC(bc8e497f) SHA1(747a38042844a6b632dcd9a76d8525fccb5eb892), ROM_BIOS(2) )
ROM_END

/* System Configuration */

static DEVICE_IMAGE_LOAD( atarist_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		UINT8 bootsector[512];
		image_fseek(image, 0, SEEK_SET);

		if (image_fread(image, bootsector, 512))
		{
			int sectors = bootsector[0x18];
			int heads = bootsector[0x1a];
			int tracks = (bootsector[0x13] | (bootsector[0x14] << 8)) / sectors / heads;

			/* drive, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, tracks, heads, sectors, 512, 1, 0, FALSE);

			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

static void atarist_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(atarist_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "st"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_IMAGE_LOAD( atarist_serial )
{
	/* filename specified */
	if (device_load_serial_device(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600, 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void atarist_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:							info->start = DEVICE_START_NAME(serial_device); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(atarist_serial); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(serial_device); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static void megaste_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:							info->start = DEVICE_START_NAME(serial_device); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(atarist_serial); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(serial_device); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static DEVICE_IMAGE_LOAD( atarist_cart )
{
	UINT8 *ptr = ((UINT8 *)memory_region(REGION_CPU1)) + 0xfa0000;
	int	filesize = image_length(image);

	if (filesize <= 128 * 1024)
	{
		if (image_fread(image, ptr, filesize) == filesize)
		{
			memory_install_readwrite16_handler(image->machine, 0, ADDRESS_SPACE_PROGRAM, 0xfa0000, 0xfbffff, 0, 0, SMH_BANK3, SMH_BANK3);

			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

static void atarist_cartslot_getinfo( const mess_device_class *devclass, UINT32 state, union devinfo *info )
{
	switch( state )
	{
	case MESS_DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case MESS_DEVINFO_PTR_LOAD:
		info->load = DEVICE_IMAGE_LOAD_NAME(atarist_cart);
		break;
	case MESS_DEVINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = device_temp_str(), "stc");
		break;
	default:
		cartslot_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START( atarist )
	CONFIG_RAM_DEFAULT(1024 * 1024) // 1040ST
	CONFIG_RAM		  ( 512 * 1024) //  520ST
	CONFIG_RAM		  ( 256 * 1024) //  260ST
	CONFIG_DEVICE(atarist_floppy_getinfo)
	CONFIG_DEVICE(atarist_serial_getinfo)
	CONFIG_DEVICE(atarist_cartslot_getinfo)
	// MIDI
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( megast )
	CONFIG_RAM_DEFAULT(4096 * 1024) // Mega ST 4
	CONFIG_RAM		  (2048 * 1024) // Mega ST 2
	CONFIG_RAM		  (1024 * 1024) // Mega ST 1
	CONFIG_DEVICE(atarist_floppy_getinfo)
	CONFIG_DEVICE(atarist_serial_getinfo)
	CONFIG_DEVICE(atarist_cartslot_getinfo)
	// MIDI
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( atariste )
	CONFIG_RAM_DEFAULT(1024 * 1024) // 1040STe
	CONFIG_RAM		  ( 512 * 1024) //  520STe
	CONFIG_DEVICE(atarist_floppy_getinfo)
	CONFIG_DEVICE(atarist_serial_getinfo)
	CONFIG_DEVICE(atarist_cartslot_getinfo)
	// MIDI
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( megaste )
	CONFIG_RAM_DEFAULT(4096 * 1024) // Mega STe 4
	CONFIG_RAM		  (2048 * 1024) // Mega STe 2
	CONFIG_RAM		  (1024 * 1024) // Mega STe 1
	CONFIG_DEVICE(atarist_floppy_getinfo)
	CONFIG_DEVICE(megaste_serial_getinfo)
	CONFIG_DEVICE(atarist_cartslot_getinfo)
	// MIDI
	// LAN
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( stbook )
	CONFIG_RAM_DEFAULT(4096 * 1024)
	CONFIG_RAM		  (1024 * 1024)
	CONFIG_DEVICE(atarist_floppy_getinfo)
	CONFIG_DEVICE(megaste_serial_getinfo)
	CONFIG_DEVICE(atarist_cartslot_getinfo)
	// MIDI
	// IDE Hard Disk
SYSTEM_CONFIG_END

/* System Drivers */

/*     YEAR  NAME    PARENT    COMPAT   MACHINE   INPUT     INIT    CONFIG   COMPANY    FULLNAME */
COMP( 1985, atarist,  0,        0,	atarist,  atarist,  0,     atarist,  "Atari", "ST", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1987, megast,   atarist,  0,	megast,   atarist,  0,     megast,   "Atari", "Mega ST", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE  )
/*
COMP( 1989, stacy,    atarist,  0,      stacy,    stacy,    0,     stacy,    "Atari", "Stacy", GAME_NOT_WORKING )
*/
COMP( 1989, atariste, 0,		0,		atariste, atariste, 0,     atariste, "Atari", "STE", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE  )
COMP( 1990, stbook,   atariste, 0,	stbook,   stbook,   0,     stbook,	 "Atari", "STBook", GAME_NOT_WORKING )
COMP( 1991, megaste,  atariste, 0,	megaste,  atarist,  0,     megaste,  "Atari", "Mega STE", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE  )
/*
COMP( 1991, stpad,    atariste, 0,      stpad,    stpad,    0,     stpad,    "Atari", "STPad (prototype)", GAME_NOT_WORKING )
COMP( 1990, tt030,    0,        0,      tt030,    tt030,    0,     tt030,    "Atari", "TT030", GAME_NOT_WORKING )
COMP( 1992, fx1,      0,        0,      falcon,   falcon,   0,     falcon,   "Atari", "FX-1 (prototype)", GAME_NOT_WORKING )
COMP( 1992, falcon,   0,        0,      falcon,   falcon,   0,     falcon,   "Atari", "Falcon030", GAME_NOT_WORKING )
COMP( 1992, falcon40, falcon,   0,      falcon40, falcon,   0,     falcon,   "Atari", "Falcon040 (prototype)", GAME_NOT_WORKING )
*/
