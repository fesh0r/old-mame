/*
    Pinball 2000

    Skeleton by R. Belmont, based on mediagx.c by Ville Linde

    TODO:
        - Everything!
        - BIOS hangs waiting for port 0400h to return 0x80.  If you make that happy it jumps off into the weeds.
        - MediaGX features should be moved out to machine/ and shared with mediagx.c once we know what these games need

    Hardware:
        - Cyrix MediaGX processor/VGA
        - Cyrix CX5520 northbridge?
        - VS9824AG SuperI/O standard PC I/O chip
        - 1 ISA, 2 PCI slots, 2 IDE headers
        - "Prism" PCI card with PLX PCI9052 PCI-to-random stuff bridge
          Card also contains DCS2 Stereo sound system with ADSP-2104
*/

#include "emu.h"
#include "cpu/i386/i386.h"
#include "machine/8237dma.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/mc146818.h"
#include "machine/pci.h"
#include "machine/8042kbdc.h"
#include "machine/pckeybrd.h"
#include "machine/idectrl.h"
#include "video/ramdac.h"

class pinball2k_state : public driver_device
{
public:
	pinball2k_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_main_ram(*this, "main_ram"),
		m_cga_ram(*this, "cga_ram"),
		m_bios_ram(*this, "bios_ram"),
		m_vram(*this, "vram"),
		m_maincpu(*this, "maincpu") { }

	required_shared_ptr<UINT32> m_main_ram;
	required_shared_ptr<UINT32> m_cga_ram;
	required_shared_ptr<UINT32> m_bios_ram;
	required_shared_ptr<UINT32> m_vram;
	UINT8 m_pal[768];


	UINT32 m_disp_ctrl_reg[256/4];
	int m_frame_width;
	int m_frame_height;

	UINT32 m_memory_ctrl_reg[256/4];
	int m_pal_index;

	UINT32 m_biu_ctrl_reg[256/4];

	UINT8 m_mediagx_config_reg_sel;
	UINT8 m_mediagx_config_regs[256];

	//UINT8 m_controls_data;
	UINT8 m_parallel_pointer;
	UINT8 m_parallel_latched;
	UINT32 m_parport;
	//int m_control_num;
	//int m_control_num2;
	//int m_control_read;

	UINT32 m_cx5510_regs[256/4];

	pit8254_device  *m_pit8254;
	pic8259_device  *m_pic8259_1;
	pic8259_device  *m_pic8259_2;
	i8237_device    *m_dma8237_1;
	i8237_device    *m_dma8237_2;

	int m_dma_channel;
	UINT8 m_dma_offset[2][4];
	UINT8 m_at_pages[0x10];

	DECLARE_READ32_MEMBER(disp_ctrl_r);
	DECLARE_WRITE32_MEMBER(disp_ctrl_w);
	DECLARE_READ32_MEMBER(memory_ctrl_r);
	DECLARE_WRITE32_MEMBER(memory_ctrl_w);
	DECLARE_READ32_MEMBER(biu_ctrl_r);
	DECLARE_WRITE32_MEMBER(biu_ctrl_w);
	DECLARE_WRITE32_MEMBER(bios_ram_w);
	DECLARE_READ32_MEMBER(parallel_port_r);
	DECLARE_WRITE32_MEMBER(parallel_port_w);
	DECLARE_READ32_MEMBER(ad1847_r);
	DECLARE_WRITE32_MEMBER(ad1847_w);
	DECLARE_READ8_MEMBER(at_page8_r);
	DECLARE_WRITE8_MEMBER(at_page8_w);
	DECLARE_READ8_MEMBER(pc_dma_read_byte);
	DECLARE_WRITE8_MEMBER(pc_dma_write_byte);
	DECLARE_READ8_MEMBER(at_dma8237_2_r);
	DECLARE_WRITE8_MEMBER(at_dma8237_2_w);
	DECLARE_READ32_MEMBER(ide_r);
	DECLARE_WRITE32_MEMBER(ide_w);
	DECLARE_READ32_MEMBER(fdc_r);
	DECLARE_WRITE32_MEMBER(fdc_w);
	DECLARE_READ8_MEMBER(io20_r);
	DECLARE_WRITE8_MEMBER(io20_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dma_hrq_changed);
	DECLARE_WRITE_LINE_MEMBER(pc_dack0_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack1_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack2_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack3_w);
	DECLARE_WRITE_LINE_MEMBER(mediagx_pic8259_1_set_int_line);
	DECLARE_READ8_MEMBER(get_slave_ack);
	DECLARE_DRIVER_INIT(pinball2k);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update_mediagx(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	DECLARE_READ8_MEMBER(get_out2);
	IRQ_CALLBACK_MEMBER(irq_callback);
	void draw_char(bitmap_rgb32 &bitmap, const rectangle &cliprect, gfx_element *gfx, int ch, int att, int x, int y);
	void draw_framebuffer(bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void draw_cga(bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void init_mediagx();
	required_device<cpu_device> m_maincpu;
};

// Display controller registers
#define DC_UNLOCK               0x00/4
#define DC_GENERAL_CFG          0x04/4
#define DC_TIMING_CFG           0x08/4
#define DC_OUTPUT_CFG           0x0c/4
#define DC_FB_ST_OFFSET         0x10/4
#define DC_CB_ST_OFFSET         0x14/4
#define DC_CUR_ST_OFFSET        0x18/4
#define DC_VID_ST_OFFSET        0x20/4
#define DC_LINE_DELTA           0x24/4
#define DC_BUF_SIZE             0x28/4
#define DC_H_TIMING_1           0x30/4
#define DC_H_TIMING_2           0x34/4
#define DC_H_TIMING_3           0x38/4
#define DC_FP_H_TIMING          0x3c/4
#define DC_V_TIMING_1           0x40/4
#define DC_V_TIMING_2           0x44/4
#define DC_V_TIMING_3           0x48/4
#define DC_FP_V_TIMING          0x4c/4
#define DC_CURSOR_X             0x50/4
#define DC_V_LINE_CNT           0x54/4
#define DC_CURSOR_Y             0x58/4
#define DC_SS_LINE_CMP          0x5c/4
#define DC_PAL_ADDRESS          0x70/4
#define DC_PAL_DATA             0x74/4
#define DC_DFIFO_DIAG           0x78/4
#define DC_CFIFO_DIAG           0x7c/4






static const rgb_t cga_palette[16] =
{
	MAKE_RGB( 0x00, 0x00, 0x00 ), MAKE_RGB( 0x00, 0x00, 0xaa ), MAKE_RGB( 0x00, 0xaa, 0x00 ), MAKE_RGB( 0x00, 0xaa, 0xaa ),
	MAKE_RGB( 0xaa, 0x00, 0x00 ), MAKE_RGB( 0xaa, 0x00, 0xaa ), MAKE_RGB( 0xaa, 0x55, 0x00 ), MAKE_RGB( 0xaa, 0xaa, 0xaa ),
	MAKE_RGB( 0x55, 0x55, 0x55 ), MAKE_RGB( 0x55, 0x55, 0xff ), MAKE_RGB( 0x55, 0xff, 0x55 ), MAKE_RGB( 0x55, 0xff, 0xff ),
	MAKE_RGB( 0xff, 0x55, 0x55 ), MAKE_RGB( 0xff, 0x55, 0xff ), MAKE_RGB( 0xff, 0xff, 0x55 ), MAKE_RGB( 0xff, 0xff, 0xff ),
};

void pinball2k_state::video_start()
{
	int i;
	for (i=0; i < 16; i++)
	{
		palette_set_color(machine(), i, cga_palette[i]);
	}
}

void pinball2k_state::draw_char(bitmap_rgb32 &bitmap, const rectangle &cliprect, gfx_element *gfx, int ch, int att, int x, int y)
{
	int i,j;
	const UINT8 *dp;
	int index = 0;
	const pen_t *pens = gfx->machine().pens;

	dp = gfx->get_data(ch);

	for (j=y; j < y+8; j++)
	{
		UINT32 *p = &bitmap.pix32(j);
		for (i=x; i < x+8; i++)
		{
			UINT8 pen = dp[index++];
			if (pen)
				p[i] = pens[gfx->colorbase() + (att & 0xf)];
			else
			{
				if (((att >> 4) & 7) > 0)
					p[i] = pens[gfx->colorbase() + ((att >> 4) & 0x7)];
			}
		}
	}
}

void pinball2k_state::draw_framebuffer(bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	int i, j;
	int width, height;
	int line_delta = (m_disp_ctrl_reg[DC_LINE_DELTA] & 0x3ff) * 4;

	width = (m_disp_ctrl_reg[DC_H_TIMING_1] & 0x7ff) + 1;
	if (m_disp_ctrl_reg[DC_TIMING_CFG] & 0x8000)     // pixel double
	{
		width >>= 1;
	}
	width += 4;

	height = (m_disp_ctrl_reg[DC_V_TIMING_1] & 0x7ff) + 1;

	if ( (width != m_frame_width || height != m_frame_height) &&
			(width > 1 && height > 1 && width <= 640 && height <= 480) )
	{
		rectangle visarea;

		m_frame_width = width;
		m_frame_height = height;

		visarea.set(0, width - 1, 0, height - 1);
		machine().primary_screen->configure(width, height * 262 / 240, visarea, machine().primary_screen->frame_period().attoseconds);
	}

	if (m_disp_ctrl_reg[DC_OUTPUT_CFG] & 0x1)        // 8-bit mode
	{
		UINT8 *framebuf = (UINT8*)&m_vram[m_disp_ctrl_reg[DC_FB_ST_OFFSET]/4];
		UINT8 *pal = m_pal;

		for (j=0; j < m_frame_height; j++)
		{
			UINT32 *p = &bitmap.pix32(j);
			UINT8 *si = &framebuf[j * line_delta];
			for (i=0; i < m_frame_width; i++)
			{
				int c = *si++;
				int r = pal[(c*3)+0] << 2;
				int g = pal[(c*3)+1] << 2;
				int b = pal[(c*3)+2] << 2;

				p[i] = r << 16 | g << 8 | b;
			}
		}
	}
	else            // 16-bit
	{
		UINT16 *framebuf = (UINT16*)&m_vram[m_disp_ctrl_reg[DC_FB_ST_OFFSET]/4];

		// RGB 5-6-5 mode
		if ((m_disp_ctrl_reg[DC_OUTPUT_CFG] & 0x2) == 0)
		{
			for (j=0; j < m_frame_height; j++)
			{
				UINT32 *p = &bitmap.pix32(j);
				UINT16 *si = &framebuf[j * (line_delta/2)];
				for (i=0; i < m_frame_width; i++)
				{
					UINT16 c = *si++;
					int r = ((c >> 11) & 0x1f) << 3;
					int g = ((c >> 5) & 0x3f) << 2;
					int b = (c & 0x1f) << 3;

					p[i] = r << 16 | g << 8 | b;
				}
			}
		}
		// RGB 5-5-5 mode
		else
		{
			for (j=0; j < m_frame_height; j++)
			{
				UINT32 *p = &bitmap.pix32(j);
				UINT16 *si = &framebuf[j * (line_delta/2)];
				for (i=0; i < m_frame_width; i++)
				{
					UINT16 c = *si++;
					int r = ((c >> 10) & 0x1f) << 3;
					int g = ((c >> 5) & 0x1f) << 3;
					int b = (c & 0x1f) << 3;

					p[i] = r << 16 | g << 8 | b;
				}
			}
		}
	}
}

void pinball2k_state::draw_cga(bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	int i, j;
	gfx_element *gfx = machine().gfx[0];
	UINT32 *cga = m_cga_ram;
	int index = 0;

	for (j=0; j < 25; j++)
	{
		for (i=0; i < 80; i+=2)
		{
			int att0 = (cga[index] >> 8) & 0xff;
			int ch0 = (cga[index] >> 0) & 0xff;
			int att1 = (cga[index] >> 24) & 0xff;
			int ch1 = (cga[index] >> 16) & 0xff;

			draw_char(bitmap, cliprect, gfx, ch0, att0, i*8, j*8);
			draw_char(bitmap, cliprect, gfx, ch1, att1, (i*8)+8, j*8);
			index++;
		}
	}
}

UINT32 pinball2k_state::screen_update_mediagx(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0, cliprect);

	draw_framebuffer( bitmap, cliprect);

	if (m_disp_ctrl_reg[DC_OUTPUT_CFG] & 0x1)   // don't show MDA text screen on 16-bit mode. this is basically a hack
	{
		draw_cga(bitmap, cliprect);
	}
	return 0;
}

READ32_MEMBER(pinball2k_state::disp_ctrl_r)
{
	UINT32 r = m_disp_ctrl_reg[offset];

	switch (offset)
	{
		case DC_TIMING_CFG:
			r |= 0x40000000;

			if (machine().primary_screen->vpos() >= m_frame_height)
				r &= ~0x40000000;
			break;
	}

	return r;
}

WRITE32_MEMBER(pinball2k_state::disp_ctrl_w)
{
//  printf("disp_ctrl_w %08X, %08X, %08X\n", data, offset*4, mem_mask);
	COMBINE_DATA(m_disp_ctrl_reg + offset);
}


READ8_MEMBER(pinball2k_state::at_dma8237_2_r)
{
	return m_dma8237_2->i8237_r(space, offset / 2);
}

WRITE8_MEMBER(pinball2k_state::at_dma8237_2_w)
{
	m_dma8237_2->i8237_w(space, offset / 2, data);
}


READ32_MEMBER(pinball2k_state::ide_r)
{
	device_t *device = machine().device("ide");
	return ide_controller32_r(device, space, 0x1f0/4 + offset, mem_mask);
}

WRITE32_MEMBER(pinball2k_state::ide_w)
{
	device_t *device = machine().device("ide");
	ide_controller32_w(device, space, 0x1f0/4 + offset, data, mem_mask);
}

READ32_MEMBER(pinball2k_state::fdc_r)
{
	device_t *device = machine().device("ide");
	return ide_controller32_r(device, space, 0x3f0/4 + offset, mem_mask);
}

WRITE32_MEMBER(pinball2k_state::fdc_w)
{
	device_t *device = machine().device("ide");
	ide_controller32_w(device, space, 0x3f0/4 + offset, data, mem_mask);
}



READ32_MEMBER(pinball2k_state::memory_ctrl_r)
{
	return m_memory_ctrl_reg[offset];
}

WRITE32_MEMBER(pinball2k_state::memory_ctrl_w)
{
//  printf("memory_ctrl_w %08X, %08X, %08X\n", data, offset*4, mem_mask);
	if (offset == 0x20/4)
	{
		ramdac_device *ramdac = machine().device<ramdac_device>("ramdac");

		if((m_disp_ctrl_reg[DC_GENERAL_CFG] & 0x00e00000) == 0x00400000)
		{
			// guess: crtc params?
			// ...
		}
		else if((m_disp_ctrl_reg[DC_GENERAL_CFG] & 0x00f00000) == 0x00000000)
		{
			m_pal_index = data;
			ramdac->index_w( space, 0, data );
		}
		else if((m_disp_ctrl_reg[DC_GENERAL_CFG] & 0x00f00000) == 0x00100000)
		{
			m_pal[m_pal_index] = data & 0xff;
			m_pal_index++;
			if (m_pal_index >= 768)
			{
				m_pal_index = 0;
			}
			ramdac->pal_w( space, 0, data );
		}
	}
	else
	{
		COMBINE_DATA(m_memory_ctrl_reg + offset);
	}
}



READ32_MEMBER(pinball2k_state::biu_ctrl_r)
{
	if (offset == 0)
	{
		return 0xffffff;
	}
	return m_biu_ctrl_reg[offset];
}

WRITE32_MEMBER(pinball2k_state::biu_ctrl_w)
{
	//mame_printf_debug("biu_ctrl_w %08X, %08X, %08X\n", data, offset, mem_mask);
	COMBINE_DATA(m_biu_ctrl_reg + offset);

	if (offset == 3)        // BC_XMAP_3 register
	{
		//mame_printf_debug("BC_XMAP_3: %08X, %08X, %08X\n", data, offset, mem_mask);
	}
}

#ifdef UNUSED_FUNCTION
WRITE32_MEMBER(pinball2k_state::bios_ram_w)
{
}
#endif

static UINT8 mediagx_config_reg_r(device_t *device)
{
	pinball2k_state *state = device->machine().driver_data<pinball2k_state>();

	//mame_printf_debug("mediagx_config_reg_r %02X\n", mediagx_config_reg_sel);
	return state->m_mediagx_config_regs[state->m_mediagx_config_reg_sel];
}

static void mediagx_config_reg_w(device_t *device, UINT8 data)
{
	pinball2k_state *state = device->machine().driver_data<pinball2k_state>();

	//mame_printf_debug("mediagx_config_reg_w %02X, %02X\n", mediagx_config_reg_sel, data);
	state->m_mediagx_config_regs[state->m_mediagx_config_reg_sel] = data;
}

READ8_MEMBER(pinball2k_state::io20_r)
{
	pic8259_device *device = machine().device<pic8259_device>("pic8259_master");
	UINT8 r = 0;

	// 0x22, 0x23, Cyrix configuration registers
	if (offset == 0x02)
	{
	}
	else if (offset == 0x03)
	{
		r = mediagx_config_reg_r(device);
	}
	else
	{
		r = device->read(space, offset);
	}
	return r;
}

WRITE8_MEMBER(pinball2k_state::io20_w)
{
	pic8259_device *device = machine().device<pic8259_device>("pic8259_master");

	// 0x22, 0x23, Cyrix configuration registers
	if (offset == 0x02)
	{
		m_mediagx_config_reg_sel = data;
	}
	else if (offset == 0x03)
	{
		mediagx_config_reg_w(device, data);
	}
	else
	{
		device->write(space, offset, data);
	}
}

READ32_MEMBER(pinball2k_state::parallel_port_r)
{
	UINT32 r = 0;

	return r;
}

WRITE32_MEMBER(pinball2k_state::parallel_port_w)
{
}

static UINT32 cx5510_pci_r(device_t *busdevice, device_t *device, int function, int reg, UINT32 mem_mask)
{
	pinball2k_state *state = busdevice->machine().driver_data<pinball2k_state>();

	//mame_printf_debug("CX5510: PCI read %d, %02X, %08X\n", function, reg, mem_mask);
	switch (reg)
	{
		case 0:     return 0x00001078;
	}

	return state->m_cx5510_regs[reg/4];
}

static void cx5510_pci_w(device_t *busdevice, device_t *device, int function, int reg, UINT32 data, UINT32 mem_mask)
{
	pinball2k_state *state = busdevice->machine().driver_data<pinball2k_state>();

	//mame_printf_debug("CX5510: PCI write %d, %02X, %08X, %08X\n", function, reg, data, mem_mask);
	COMBINE_DATA(state->m_cx5510_regs + (reg/4));
}

/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/


READ8_MEMBER(pinball2k_state::at_page8_r)
{
	UINT8 data = m_at_pages[offset % 0x10];

	switch(offset % 8)
	{
	case 1:
		data = m_dma_offset[(offset / 8) & 1][2];
		break;
	case 2:
		data = m_dma_offset[(offset / 8) & 1][3];
		break;
	case 3:
		data = m_dma_offset[(offset / 8) & 1][1];
		break;
	case 7:
		data = m_dma_offset[(offset / 8) & 1][0];
		break;
	}
	return data;
}


WRITE8_MEMBER(pinball2k_state::at_page8_w)
{
	m_at_pages[offset % 0x10] = data;

	switch(offset % 8)
	{
	case 1:
		m_dma_offset[(offset / 8) & 1][2] = data;
		break;
	case 2:
		m_dma_offset[(offset / 8) & 1][3] = data;
		break;
	case 3:
		m_dma_offset[(offset / 8) & 1][1] = data;
		break;
	case 7:
		m_dma_offset[(offset / 8) & 1][0] = data;
		break;
	}
}


WRITE_LINE_MEMBER(pinball2k_state::pc_dma_hrq_changed)
{
	m_maincpu->set_input_line(INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	m_dma8237_1->i8237_hlda_w( state );
}


READ8_MEMBER(pinball2k_state::pc_dma_read_byte)
{
	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0xFF0000;

	return space.read_byte(page_offset + offset);
}


WRITE8_MEMBER(pinball2k_state::pc_dma_write_byte)
{
	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0xFF0000;

	space.write_byte(page_offset + offset, data);
}

static void set_dma_channel(device_t *device, int channel, int _state)
{
	pinball2k_state *state = device->machine().driver_data<pinball2k_state>();

	if (!_state) state->m_dma_channel = channel;
}

WRITE_LINE_MEMBER(pinball2k_state::pc_dack0_w){ set_dma_channel(m_dma8237_1, 0, state); }
WRITE_LINE_MEMBER(pinball2k_state::pc_dack1_w){ set_dma_channel(m_dma8237_1, 1, state); }
WRITE_LINE_MEMBER(pinball2k_state::pc_dack2_w){ set_dma_channel(m_dma8237_1, 2, state); }
WRITE_LINE_MEMBER(pinball2k_state::pc_dack3_w){ set_dma_channel(m_dma8237_1, 3, state); }

static I8237_INTERFACE( dma8237_1_config )
{
	DEVCB_DRIVER_LINE_MEMBER(pinball2k_state,pc_dma_hrq_changed),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pinball2k_state, pc_dma_read_byte),
	DEVCB_DRIVER_MEMBER(pinball2k_state, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_DRIVER_LINE_MEMBER(pinball2k_state,pc_dack0_w), DEVCB_DRIVER_LINE_MEMBER(pinball2k_state,pc_dack1_w), DEVCB_DRIVER_LINE_MEMBER(pinball2k_state,pc_dack2_w), DEVCB_DRIVER_LINE_MEMBER(pinball2k_state,pc_dack3_w) }
};

static I8237_INTERFACE( dma8237_2_config )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};


/*****************************************************************************/

static ADDRESS_MAP_START( mediagx_map, AS_PROGRAM, 32, pinball2k_state )
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAM AM_SHARE("main_ram")
	AM_RANGE(0x000a0000, 0x000affff) AM_RAM
	AM_RANGE(0x000b0000, 0x000b7fff) AM_RAM AM_SHARE("cga_ram")
	AM_RANGE(0x000c0000, 0x000fffff) AM_RAM AM_SHARE("bios_ram")
	AM_RANGE(0x00100000, 0x00ffffff) AM_RAM
	AM_RANGE(0x40008000, 0x400080ff) AM_READWRITE(biu_ctrl_r, biu_ctrl_w)
	AM_RANGE(0x40008300, 0x400083ff) AM_READWRITE(disp_ctrl_r, disp_ctrl_w)
	AM_RANGE(0x40008400, 0x400084ff) AM_READWRITE(memory_ctrl_r, memory_ctrl_w)
	AM_RANGE(0x40800000, 0x40bfffff) AM_RAM AM_SHARE("vram")
	AM_RANGE(0xfffc0000, 0xffffffff) AM_ROM AM_REGION("bios", 0)    /* System BIOS */
ADDRESS_MAP_END

static ADDRESS_MAP_START(mediagx_io, AS_IO, 32, pinball2k_state )
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_device, i8237_r, i8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE8(io20_r, io20_w, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8_LEGACY("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x006f) AM_DEVREADWRITE8("kbdc", kbdc8042_device, data_r, data_w, 0xffffffff)
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8("rtc", mc146818_device, read, write, 0xffffffff)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(at_page8_r,              at_page8_w, 0xffffffff)
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8("pic8259_slave", pic8259_device, read, write, 0xffffffff)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE8(at_dma8237_2_r, at_dma8237_2_w, 0xffffffff)
	AM_RANGE(0x00e8, 0x00eb) AM_NOP     // I/O delay port
	AM_RANGE(0x01f0, 0x01f7) AM_READWRITE(ide_r, ide_w)
	AM_RANGE(0x0378, 0x037b) AM_READWRITE(parallel_port_r, parallel_port_w)
	AM_RANGE(0x03f0, 0x03ff) AM_READWRITE(fdc_r, fdc_w)
	AM_RANGE(0x0cf8, 0x0cff) AM_DEVREADWRITE("pcibus", pci_bus_legacy_device, read, write)
ADDRESS_MAP_END

/*****************************************************************************/

static const gfx_layout CGA_charlayout =
{
	8,8,                    /* 8 x 16 characters */
	256,                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8,1*8,2*8,3*8,
		4*8,5*8,6*8,7*8 },
	8*8                     /* every char takes 8 bytes */
};

static GFXDECODE_START( CGA )
/* Support up to four CGA fonts */
	GFXDECODE_ENTRY( "gfx1", 0x0000, CGA_charlayout, 0, 256 )   /* Font 0 */
	GFXDECODE_ENTRY( "gfx1", 0x0800, CGA_charlayout, 0, 256 )   /* Font 1 */
	GFXDECODE_ENTRY( "gfx1", 0x1000, CGA_charlayout, 0, 256 )   /* Font 2 */
	GFXDECODE_ENTRY( "gfx1", 0x1800, CGA_charlayout, 0, 256 )   /* Font 3*/
GFXDECODE_END

static INPUT_PORTS_START(mediagx)
	PORT_START("IN0")
	PORT_SERVICE_NO_TOGGLE( 0x001, IP_ACTIVE_HIGH )
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_SERVICE2 )
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_VOLUME_DOWN )
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x400, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x800, IP_ACTIVE_HIGH, IPT_START4 )

	PORT_START("IN1")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_BUTTON3 )

	PORT_START("IN2")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_BUTTON6 )

	PORT_START("IN3")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_BUTTON7 )
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_BUTTON8 )
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_BUTTON9 )

	PORT_START("IN4")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START("IN5")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(3)

	PORT_START("IN6")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )

	PORT_START("IN7")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)

	PORT_START("IN8")
	PORT_BIT( 0x00f, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0f0, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0xf00, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
INPUT_PORTS_END

IRQ_CALLBACK_MEMBER(pinball2k_state::irq_callback)
{
	return m_pic8259_1->acknowledge();
}

void pinball2k_state::machine_start()
{
	m_pit8254 = machine().device<pit8254_device>( "pit8254" );
	m_pic8259_1 = machine().device<pic8259_device>( "pic8259_master" );
	m_pic8259_2 = machine().device<pic8259_device>( "pic8259_slave" );
	m_dma8237_1 = machine().device<i8237_device>( "dma8237_1" );
	m_dma8237_2 = machine().device<i8237_device>( "dma8237_2" );
}

void pinball2k_state::machine_reset()
{
	UINT8 *rom = memregion("bios")->base();

	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(pinball2k_state::irq_callback),this));

	memcpy(m_bios_ram, rom, 0x40000);
	m_maincpu->reset();

	machine().device("ide")->reset();
}

/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

WRITE_LINE_MEMBER(pinball2k_state::mediagx_pic8259_1_set_int_line)
{
	m_maincpu->set_input_line(0, state ? HOLD_LINE : CLEAR_LINE);
}

READ8_MEMBER(pinball2k_state::get_slave_ack)
{
	if (offset==2) { // IRQ = 2
		return m_pic8259_2->acknowledge();
	}
	return 0x00;
}


/*************************************************************
 *
 * pit8254 configuration
 *
 *************************************************************/

static const struct pit8253_config mediagx_pit8254_config =
{
	{
		{
			4772720/4,              /* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE_MEMBER("pic8259_master", pic8259_device, ir0_w)
		}, {
			4772720/4,              /* dram refresh */
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			4772720/4,              /* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_NULL
		}
	}
};

static ADDRESS_MAP_START( ramdac_map, AS_0, 8, pinball2k_state )
	AM_RANGE(0x000, 0x3ff) AM_DEVREADWRITE("ramdac",ramdac_device,ramdac_pal_r,ramdac_rgb666_w)
ADDRESS_MAP_END

static RAMDAC_INTERFACE( ramdac_intf )
{
	0
};

READ8_MEMBER(pinball2k_state::get_out2)
{
	return pit8253_get_output( m_pit8254, 2 );
}

static const struct kbdc8042_interface at8042 =
{
	KBDC8042_AT386,
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_RESET),
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_A20),
	DEVCB_DEVICE_LINE_MEMBER("pic8259_master", pic8259_device, ir1_w),
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pinball2k_state,get_out2)
};

static MACHINE_CONFIG_START( mediagx, pinball2k_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", MEDIAGX, 166000000)
	MCFG_CPU_PROGRAM_MAP(mediagx_map)
	MCFG_CPU_IO_MAP(mediagx_io)


	MCFG_PCI_BUS_LEGACY_ADD("pcibus", 0)
	MCFG_PCI_BUS_LEGACY_DEVICE(18, NULL, cx5510_pci_r, cx5510_pci_w)

	MCFG_PIT8254_ADD( "pit8254", mediagx_pit8254_config )

	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, dma8237_1_config )

	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, dma8237_2_config )

	MCFG_PIC8259_ADD( "pic8259_master", WRITELINE(pinball2k_state,mediagx_pic8259_1_set_int_line), VCC, READ8(pinball2k_state,get_slave_ack) )

	MCFG_PIC8259_ADD( "pic8259_slave", DEVWRITELINE("pic8259_master", pic8259_device, ir2_w), GND, NULL )

	MCFG_IDE_CONTROLLER_ADD("ide", ide_devices, "hdd", NULL, true)
	MCFG_IDE_CONTROLLER_IRQ_HANDLER(DEVWRITELINE("pic8259_slave", pic8259_device, ir6_w))

	MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )

	MCFG_RAMDAC_ADD("ramdac", ramdac_intf, ramdac_map)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 239)
	MCFG_SCREEN_UPDATE_DRIVER(pinball2k_state, screen_update_mediagx)

	MCFG_GFXDECODE(CGA)
	MCFG_PALETTE_LENGTH(256)

	MCFG_KBDC8042_ADD("kbdc", at8042)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
MACHINE_CONFIG_END


void pinball2k_state::init_mediagx()
{
	m_frame_width = m_frame_height = 1;
}

DRIVER_INIT_MEMBER(pinball2k_state, pinball2k)
{
	init_mediagx();
}

/*****************************************************************************/

ROM_START( swe1pb )
	ROM_REGION32_LE(0x40000, "bios", 0)
	ROM_LOAD( "awdbios.bin",  0x000000, 0x040000, CRC(854ce8c6) SHA1(7826de74026e052dacce8516382f664004c327ad) )

	ROM_REGION(0x4800000, "prism", 0)
	ROM_LOAD( "swe1_u100.rom", 0x0000000, 0x800000, CRC(db2c9709) SHA1(14e8db2c0b09c4da6306a4a1f7fe54b2a334c5ed) )
	ROM_LOAD( "swe1_u101.rom", 0x0800000, 0x800000, CRC(a039e80d) SHA1(8f63e8ab83e043232fc17ed3dff1f251396a178a) )
	ROM_LOAD( "swe1_u102.rom", 0x1000000, 0x800000, CRC(c9feb7bc) SHA1(a34acd34c3f91f082b67e385b1f4da2e5b6e5087) )
	ROM_LOAD( "swe1_u103.rom", 0x1800000, 0x800000, CRC(7a692466) SHA1(9adf5ae9c12bd5b6314913f6c01d4566ee453fe1) )
	ROM_LOAD( "swe1_u104.rom", 0x2000000, 0x800000, CRC(76e2dd7e) SHA1(9bc20a1423b11c46eb2f5a514e985151defb5651) )
	ROM_LOAD( "swe1_u105.rom", 0x2800000, 0x800000, CRC(87f2460c) SHA1(cdc05e017367f61280e3d5682096e67e4c200150) )
	ROM_LOAD( "swe1_u106.rom", 0x3000000, 0x800000, CRC(84877e2f) SHA1(6dd8c761b2e26313ae9e159690b3a4a170cb3bd8) )
	ROM_LOAD( "swe1_u107.rom", 0x3800000, 0x800000, CRC(dc433c89) SHA1(9f1273debc9168c04202078503cfc4f1ca8cb30b) )
	ROM_LOAD( "swe1_u109.rom", 0x4000000, 0x400000, CRC(cc08936b) SHA1(fc428393e8a0cf37b800dd475fd293a1a98c4bcf) )
	ROM_LOAD( "swe1_u110.rom", 0x4400000, 0x400000, CRC(6011ecd9) SHA1(8575958c8942a6cbcb2ac18f291fcada6f8cbc09) )

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( rfmpb )
	ROM_REGION32_LE(0x40000, "bios", 0)
	ROM_LOAD( "awdbios.bin",  0x000000, 0x040000, CRC(854ce8c6) SHA1(7826de74026e052dacce8516382f664004c327ad) )

	ROM_REGION(0x4000000, "prism", 0)
	ROM_LOAD( "rfm_u100.rom", 0x0000000, 0x800000, CRC(b3548b1b) SHA1(874a16282bb778886cea2567d68ec7024dc5ed22) )
	ROM_LOAD( "rfm_u101.rom", 0x0800000, 0x800000, CRC(8bef301d) SHA1(2eade00b1a4cd3f5e98ebe8ed8f549e328188e77) )
	ROM_LOAD( "rfm_u102.rom", 0x1000000, 0x800000, CRC(749f5c59) SHA1(2d8850e7f8ea3e07e8b444d7dd4dc4195a547ae7) )
	ROM_LOAD( "rfm_u103.rom", 0x1800000, 0x800000, CRC(a9ec5e97) SHA1(ce7c38dcbf34ce10d6e204a3176cd2c7a83b525a) )
	ROM_LOAD( "rfm_u104.rom", 0x2000000, 0x800000, CRC(0a1acd70) SHA1(dcca4de92eadeb82ac776953326410a9687838cb) )
	ROM_LOAD( "rfm_u105.rom", 0x2800000, 0x800000, CRC(1ef31684) SHA1(141900a7426ad483384606cddb018d186952f439) )
	ROM_LOAD( "rfm_u106.rom", 0x3000000, 0x800000, CRC(daf4e1dc) SHA1(0612495468fb962b833057e50f620c5f69cd5840) )
	ROM_LOAD( "rfm_u107.rom", 0x3800000, 0x800000, CRC(e737ab39) SHA1(0e978923db19e2893fdb4aae69d6ed3c3f664a31) )

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( rfmpbr2 )
	ROM_REGION32_LE(0x40000, "bios", 0)
	ROM_LOAD( "awdbios.bin",  0x000000, 0x040000, CRC(854ce8c6) SHA1(7826de74026e052dacce8516382f664004c327ad) )

	ROM_REGION(0x4800000, "prism", 0)
	ROM_LOAD( "rfm_u100r2.rom", 0x0000000, 0x800000, CRC(d4278a9b) SHA1(ec07b97190acb6b34b9ed6cda505ee8fefd66fec) )
	ROM_LOAD( "rfm_u101r2.rom", 0x0800000, 0x800000, CRC(e5d4c0ed) SHA1(cfc7d9d2324cc02c9eaf53fd674f7db24736699c) )
	ROM_LOAD( "rfm_u102.rom",   0x1000000, 0x800000, CRC(749f5c59) SHA1(2d8850e7f8ea3e07e8b444d7dd4dc4195a547ae7) )
	ROM_LOAD( "rfm_u103.rom",   0x1800000, 0x800000, CRC(a9ec5e97) SHA1(ce7c38dcbf34ce10d6e204a3176cd2c7a83b525a) )
	ROM_LOAD( "rfm_u104.rom",   0x2000000, 0x800000, CRC(0a1acd70) SHA1(dcca4de92eadeb82ac776953326410a9687838cb) )
	ROM_LOAD( "rfm_u105.rom",   0x2800000, 0x800000, CRC(1ef31684) SHA1(141900a7426ad483384606cddb018d186952f439) )
	ROM_LOAD( "rfm_u106.rom",   0x3000000, 0x800000, CRC(daf4e1dc) SHA1(0612495468fb962b833057e50f620c5f69cd5840) )
	ROM_LOAD( "rfm_u107.rom",   0x3800000, 0x800000, CRC(e737ab39) SHA1(0e978923db19e2893fdb4aae69d6ed3c3f664a31) )
	ROM_LOAD( "rfm_u109.bin",   0x4000000, 0x400000, CRC(a20b2abb) SHA1(0010d7dbf60b03f50cc1d314fdf786721161b064) )
	ROM_LOAD( "rfm_u110.bin",   0x4400000, 0x400000, CRC(095abec9) SHA1(87ce156bbf673ebd50bbd7dcca4c6924d24fc823) )

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

/*****************************************************************************/

GAME( 1999, swe1pb,   0       , mediagx, mediagx, pinball2k_state, pinball2k, ROT0,   "Midway",  "Pinball 2000: Star Wars Episode 1", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_MECHANICAL )
GAME( 1999, rfmpb,    0       , mediagx, mediagx, pinball2k_state, pinball2k, ROT0,   "Midway",  "Pinball 2000: Revenge From Mars (rev. 1)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_MECHANICAL )
GAME( 1999, rfmpbr2,  rfmpb   , mediagx, mediagx, pinball2k_state, pinball2k, ROT0,   "Midway",  "Pinball 2000: Revenge From Mars (rev. 2)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_MECHANICAL )
