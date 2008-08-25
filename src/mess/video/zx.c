/***************************************************************************
	zx.c

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	The ZX has a very unorthodox video RAM system.  To start a scanline,
	the CPU must jump to video RAM at 0xC000, which is a mirror of the
	RAM at 0x4000.  The video chip (ULA?) pulls a switcharoo and changes
	the video bytes as they are fetched by the CPU.

	The video chip draws the scanline until a HALT instruction (0x76) is
	reached, which indicates no further video RAM for this scanline.  Any
	other video byte is used to generate a tile and at the same time,
	appears to the CPU as a NOP (0x00) instruction.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"
#include "sound/dac.h"

emu_timer *ula_nmi = NULL;
//emu_timer *ula_irq = NULL;
int ula_nmi_active, ula_irq_active;
int ula_frame_vsync = 0;
//int ula_scancode_count = 0;
int ula_scanline_count = 0;
static int old_x = 0;
static int old_y = 0;
static int old_c = 0;
static UINT8 charline[32];
static UINT8 charline_ptr;
static int offs1 = 0;

/*
 * Toggle the video output between black and white.
 * This happens whenever the ULA scanline IRQs are enabled/disabled.
 * Normally this is done during the synchronized zx_ula_r() function,
 * which outputs 8 pixels per code, but if the video sync is off
 * (during tape IO or sound output) zx_ula_bkgnd() is used to
 * simulate the display of a ZX80/ZX81.
 */
void zx_ula_bkgnd(running_machine *machine, int color)
{
	const device_config *screen = video_screen_first(machine->config);
	int width = video_screen_get_width(screen);
	int height = video_screen_get_height(screen);
	const rectangle *visarea = video_screen_get_visible_area(screen);

	if (ula_frame_vsync == 0 && color != old_c)
	{
		int y, new_x, new_y;
		rectangle r;
		bitmap_t *bitmap = tmpbitmap;

		new_y = video_screen_get_vpos(machine->primary_screen);
		new_x = video_screen_get_hpos(machine->primary_screen);
/*		logerror("zx_ula_bkgnd: %3d,%3d - %3d,%3d\n", old_x, old_y, new_x, new_y);*/
		y = old_y;
		for (;;)
		{
			if (y == new_y)
			{
				r.min_x = old_x;
				r.max_x = new_x;
				r.min_y = r.max_y = y;
				fillbitmap(bitmap, color, &r);
				break;
			}
			else
			{
				r.min_x = old_x;
				r.max_x = visarea->max_x;
				r.min_y = r.max_y = y;
				fillbitmap(bitmap, color, &r);
				old_x = 0;
			}
			if (++y == height)
				y = 0;
		}
		old_x = (new_x + 1) % width;
		old_y = new_y;
		old_c = color;
		dac_data_w(0, color ? 255 : 0);
	}
}

/*
 * PAL:  310 total lines,
 *			  0.. 55 vblank
 *			 56..247 192 visible lines
 *			248..303 vblank
 *			304...	 vsync
 * NTSC: 262 total lines
 *			  0.. 31 vblank
 *			 32..223 192 visible lines
 *			224..233 vblank
 */
static TIMER_CALLBACK(zx_ula_nmi)
{
	/*
	 * An NMI is issued on the ZX81 every 64us for the blanked
	 * scanlines at the top and bottom of the display.
	 */
	const device_config *screen = video_screen_first(machine->config);
	int height = video_screen_get_height(screen);
	rectangle r = *video_screen_get_visible_area(screen);
	bitmap_t *bitmap = tmpbitmap;

	r.min_y = r.max_y = ula_scanline_count;
	fillbitmap(bitmap, 1, &r);
//	logerror("ULA %3d[%d] NMI, R:$%02X, $%04x\n", video_screen_get_vpos(machine->primary_screen), ula_scancode_count, (unsigned) cpunum_get_reg(0, Z80_R), (unsigned) cpunum_get_reg(0, Z80_PC));
	cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, PULSE_LINE);
	if (++ula_scanline_count == height)
		ula_scanline_count = 0;
}

static TIMER_CALLBACK(zx_ula_irq)
{

	/*
	 * An IRQ is issued on the ZX80/81 whenever the R registers
	 * bit 6 goes low. In MESS this IRQ timed from the first read
	 * from the copy of the DFILE in the upper 32K in zx_ula_r().
	 */
	if (ula_irq_active)
	{
//		logerror("ULA %3d[%d] IRQ, R:$%02X, $%04x\n", video_screen_get_vpos(machine->primary_screen), ula_scancode_count, (unsigned) cpunum_get_reg(0, Z80_R), (unsigned) cpunum_get_reg(0, Z80_PC));

		ula_irq_active = 0;
		cpunum_set_input_line(machine, 0, 0, HOLD_LINE);
	}
}

void zx_ula_r(running_machine *machine, int offs, const char *region)
{
	const device_config *screen = video_screen_first(machine->config);
	int offs0 = offs & 0x7fff;
	UINT8 *rom = memory_region(machine, "main");
	UINT8 chr = rom[offs0];

	if ((!ula_irq_active) && (chr == 0x76))
	{
		bitmap_t *bitmap = tmpbitmap;
		UINT16 y, *scanline;
		UINT16 ireg = cpunum_get_reg(0, Z80_I) << 8;
		UINT8 creg = cpunum_get_reg(0, Z80_C);
		UINT8 data, *chrgen;
		chrgen = memory_region(machine, region);

		if ((++ula_scanline_count == video_screen_get_height(screen)) || (creg == 32))
		{
			ula_scanline_count = 0;
			offs1 = offs0;
		}

		ula_frame_vsync = 3;

		charline_ptr = 0;

		for (y = offs1+1; ((y < offs0) && (charline_ptr < ARRAY_LENGTH(charline))); y++)
		{
			charline[charline_ptr] = rom[y];
			charline_ptr++;
		}
		for (y = charline_ptr; y < ARRAY_LENGTH(charline); y++)
			charline[y] = 0;

		timer_set(ATTOTIME_IN_CYCLES(((32 - charline_ptr) << 2), 0), NULL, 0, zx_ula_irq);
		ula_irq_active++;

		scanline = BITMAP_ADDR16(bitmap, ula_scanline_count, 0);
		y = 0;

		for (charline_ptr = 0; charline_ptr < ARRAY_LENGTH(charline); charline_ptr++)
		{
			chr = charline[charline_ptr];
			data = chrgen[ireg | ((chr & 0x3f) << 3) | ((8 - creg)&7) ];
			if (chr & 0x80) data ^= 0xff;

			scanline[y++] = (data >> 7) & 1;
			scanline[y++] = (data >> 6) & 1;
			scanline[y++] = (data >> 5) & 1;
			scanline[y++] = (data >> 4) & 1;
			scanline[y++] = (data >> 3) & 1;
			scanline[y++] = (data >> 2) & 1;
			scanline[y++] = (data >> 1) & 1;
			scanline[y++] = (data >> 0) & 1;
			charline[charline_ptr] = 0;
		}

		if (creg == 1) offs1 = offs0;
	}
}

VIDEO_START( zx )
{
	ula_nmi = timer_alloc(zx_ula_nmi, NULL);
	ula_irq_active = 0;
	VIDEO_START_CALL(generic_bitmapped);
}

VIDEO_EOF( zx )
{
	/* decrement video synchronization counter */
	if (ula_frame_vsync)
		--ula_frame_vsync;
}
