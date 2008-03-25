#include "driver.h"
#include "deprecat.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/beep.h"
#include "video/cdp1864.h"


static bitmap_t *cdptmpbitmap;

static emu_timer *cdp1864_int_timer;
static emu_timer *cdp1864_efx_timer;
static emu_timer *cdp1864_dma_timer;

typedef struct
{
	int disp;
	int dmaout;
	int dmaptr;
	int audio;
	int bgcolor;
	int latch;
	int (*colorram_r)(UINT16 addr);
} CDP1864_VIDEO_CONFIG;

static CDP1864_VIDEO_CONFIG cdp1864;

static const int cdp1864_bgcolseq[] = { 2, 0, 1, 4 };

int cdp1864_efx;

static TIMER_CALLBACK(cdp1864_int_tick)
{
	int scanline = video_screen_get_vpos(machine->primary_screen);

	if (scanline == CDP1864_SCANLINE_INT_START)
	{
		if (cdp1864.disp)
		{
			cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_INT, HOLD_LINE);
			cdp1864.dmaptr = 0;
		}

		timer_adjust_oneshot(cdp1864_int_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_INT_END, 0), 0);
	}
	else
	{
		if (cdp1864.disp)
		{
			cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
		}

		timer_adjust_oneshot(cdp1864_int_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_INT_START, 0), 0);
	}
}

static TIMER_CALLBACK(cdp1864_efx_tick)
{
	int scanline = video_screen_get_vpos(machine->primary_screen);

	switch (scanline)
	{
	case CDP1864_SCANLINE_EFX_TOP_START:
		cdp1864_efx = ASSERT_LINE;
		timer_adjust_oneshot(cdp1864_efx_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_EFX_TOP_END, 0), 0);
		break;

	case CDP1864_SCANLINE_EFX_TOP_END:
		cdp1864_efx = CLEAR_LINE;
		timer_adjust_oneshot(cdp1864_efx_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_EFX_BOTTOM_START, 0), 0);
		break;

	case CDP1864_SCANLINE_EFX_BOTTOM_START:
		cdp1864_efx = ASSERT_LINE;
		timer_adjust_oneshot(cdp1864_efx_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_EFX_BOTTOM_END, 0), 0);
		break;

	case CDP1864_SCANLINE_EFX_BOTTOM_END:
		cdp1864_efx = CLEAR_LINE;
		timer_adjust_oneshot(cdp1864_efx_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_EFX_TOP_START, 0), 0);
		break;
	}
}

static TIMER_CALLBACK(cdp1864_dma_tick)
{
	int scanline = video_screen_get_vpos(machine->primary_screen);

	if (cdp1864.dmaout)
	{
		if (cdp1864.disp)
		{
			if (scanline >= CDP1864_SCANLINE_DISPLAY_START && scanline < CDP1864_SCANLINE_DISPLAY_END)
			{
				cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
			}
		}

		timer_adjust_oneshot(cdp1864_dma_timer, ATTOTIME_IN_CYCLES(CDP1864_CYCLES_DMA_WAIT, 0), 0);

		cdp1864.dmaout = 0;
	}
	else
	{
		if (cdp1864.disp)
		{
			if (scanline >= CDP1864_SCANLINE_DISPLAY_START && scanline < CDP1864_SCANLINE_DISPLAY_END)
			{
				cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_DMAOUT, HOLD_LINE);
			}
		}

		timer_adjust_oneshot(cdp1864_dma_timer, ATTOTIME_IN_CYCLES(CDP1864_CYCLES_DMA_ACTIVE, 0), 0);

		cdp1864.dmaout = 1;
	}
}

void cdp1864_dma_w(UINT8 data)
{
	int sx = video_screen_get_hpos(Machine->primary_screen) + 4;
	int y = video_screen_get_vpos(Machine->primary_screen);
	int x;

	for (x = 0; x < 8; x++)
	{
		int color = cdp1864_bgcolseq[cdp1864.bgcolor];

		if (data & 0x80)
		{
			color = cdp1864.colorram_r(cdp1864.dmaptr);
		}

		*BITMAP_ADDR16(cdptmpbitmap, y, sx + x) = Machine->pens[color];

		data <<= 1;
	}

	cdp1864.dmaptr++;
}

WRITE8_HANDLER( cdp1864_step_bgcolor_w )
{
	cdp1864.disp = 1;
	if (++cdp1864.bgcolor > 3) cdp1864.bgcolor = 0;
}

WRITE8_HANDLER( cdp1864_tone_latch_w )
{
	cdp1864.latch = data;
	beep_set_frequency(0, CDP1864_CLK_FREQ / 8 / 4 / (data + 1) / 2);
}

void cdp1864_audio_output_enable(int value)
{
	if (value == 0)
	{
		cdp1864.latch = CDP1864_DEFAULT_LATCH;
	}

	cdp1864.audio = value;

	beep_set_state(0, value);
}

READ8_HANDLER( cdp1864_dispon_r )
{
	cdp1864.disp = 1;

	return 0xff;
}

READ8_HANDLER( cdp1864_dispoff_r )
{
	cdp1864.disp = 0;
	cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
	cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);

	return 0xff;
}

MACHINE_RESET( cdp1864 )
{
	timer_adjust_oneshot(cdp1864_int_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_INT_START, 0), 0);
	timer_adjust_oneshot(cdp1864_efx_timer, video_screen_get_time_until_pos(machine->primary_screen, CDP1864_SCANLINE_EFX_TOP_START, 0), 0);
	timer_adjust_oneshot(cdp1864_dma_timer, ATTOTIME_IN_CYCLES(CDP1864_CYCLES_DMA_START, 0), 0);

	cdp1864.disp = 0;
	cdp1864.dmaout = 0;
	cdp1864.dmaptr = 0;
	cdp1864.bgcolor = 0;

	cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
	cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
	cdp1864_efx = CLEAR_LINE;

	cdp1864_audio_output_enable(0);
}

VIDEO_START( cdp1864 )
{
	const device_config *screen = video_screen_first(machine->config);
	int width = video_screen_get_width(screen);
	int height = video_screen_get_height(screen);

	cdp1864_int_timer = timer_alloc(cdp1864_int_tick, NULL);
	cdp1864_efx_timer = timer_alloc(cdp1864_efx_tick, NULL);
	cdp1864_dma_timer = timer_alloc(cdp1864_dma_tick, NULL);

	/* allocate the temporary bitmap */
	cdptmpbitmap = auto_bitmap_alloc(width, height, video_screen_get_format(screen));

	/* ensure the contents of the bitmap are saved */
	state_save_register_bitmap("video", 0, "cdptmpbitmap", cdptmpbitmap);

	state_save_register_global(cdp1864.disp);
	state_save_register_global(cdp1864.dmaout);
	state_save_register_global(cdp1864.dmaptr);
	state_save_register_global(cdp1864.audio);
	state_save_register_global(cdp1864.bgcolor);
	state_save_register_global(cdp1864.latch);
}

VIDEO_UPDATE( cdp1864 )
{
	if (cdp1864.disp)
	{
		fillbitmap(bitmap, screen->machine->pens[cdp1864_bgcolseq[cdp1864.bgcolor]], cliprect);
		copybitmap_trans(bitmap, cdptmpbitmap, 0, 0, 0, 0, cliprect, cdp1864.bgcolor);
	}
	else
	{
		fillbitmap(bitmap, get_black_pen(screen->machine), cliprect);
	}

	return 0;
}

static void cdp1864_init_palette(double res_r, double res_g, double res_b, double res_bkg)
{
	int i, r, g, b, luma;

	double res_total = res_r + res_g + res_b;

	int weight_r = (res_r / res_total) * 100;
	int weight_g = (res_g / res_total) * 100;
	int weight_b = (res_b / res_total) * 100;

	for (i = 0; i < 8; i++)
	{
		luma = (i & 4) ? weight_r : 0;
		luma += (i & 1) ? weight_g : 0;
		luma += (i & 2) ? weight_b : 0;

		r = (i & 4) ? luma : 0;
		g = (i & 1) ? luma : 0;
		b = (i & 2) ? luma : 0;

		luma = (luma * 0xff) / 100;

		palette_set_color_rgb( Machine, i, r, g, b );
	}
}

void cdp1864_configure(const CDP1864_interface *intf)
{
	cdp1864.colorram_r = intf->colorram_r;
	cdp1864_init_palette(intf->res_r, intf->res_g, intf->res_b, intf->res_bkg);
}
