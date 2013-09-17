/***************************************************************************

    QX-10

    Preliminary driver by Mariusz Wojcieszek

    Status:
    Driver boots and load CP/M from floppy image. Needs upd7220 for gfx

    Done:
    - preliminary memory map
    - floppy (upd765)
    - DMA
    - Interrupts (pic8295)

    banking:
    - 0x1c = 0
    AM_RANGE(0x0000,0x1fff) ROM
    AM_RANGE(0x2000,0xdfff) NOP
    AM_RANGE(0xe000,0xffff) RAM
    - 0x1c = 1 0x20 = 1
    AM_RANGE(0x0000,0x7fff) RAM (0x18 selects bank)
    AM_RANGE(0x8000,0x87ff) CMOS
    AM_RANGE(0x8800,0xdfff) NOP or previous bank?
    AM_RANGE(0xe000,0xffff) RAM
    - 0x1c = 1 0x20 = 0
    AM_RANGE(0x0000,0xdfff) RAM (0x18 selects bank)
    AM_RANGE(0xe000,0xffff) RAM
****************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/z80dart.h"
#include "machine/mc146818.h"
#include "machine/i8255.h"
#include "machine/am9517a.h"
#include "machine/serial.h"
#include "video/upd7220.h"
#include "machine/upd765.h"
#include "machine/ram.h"
#include "machine/qx10kbd.h"

#define MAIN_CLK    15974400

#define RS232_TAG   "rs232"

/*
    Driver data
*/

class qx10_state : public driver_device
{
public:
	qx10_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_pit_1(*this, "pit8253_1"),
	m_pit_2(*this, "pit8253_2"),
	m_pic_m(*this, "pic8259_master"),
	m_pic_s(*this, "pic8259_slave"),
	m_scc(*this, "upd7201"),
	m_ppi(*this, "i8255"),
	m_dma_1(*this, "8237dma_1"),
	m_dma_2(*this, "8237dma_2"),
	m_fdc(*this, "upd765"),
	m_hgdc(*this, "upd7220"),
	m_rtc(*this, "rtc"),
	m_kbd(*this, "kbd"),
	m_vram_bank(0),
	m_maincpu(*this, "maincpu"),
	m_ram(*this, RAM_TAG) { }

	required_device<pit8253_device> m_pit_1;
	required_device<pit8253_device> m_pit_2;
	required_device<pic8259_device> m_pic_m;
	required_device<pic8259_device> m_pic_s;
	required_device<upd7201_device> m_scc;
	required_device<i8255_device> m_ppi;
	required_device<am9517a_device> m_dma_1;
	required_device<am9517a_device> m_dma_2;
	required_device<upd765a_device> m_fdc;
	required_device<upd7220_device> m_hgdc;
	required_device<mc146818_device> m_rtc;
	required_device<qx10_keyboard_device> m_kbd;
	UINT8 m_vram_bank;
	//required_shared_ptr<UINT8> m_video_ram;
	UINT8 *m_video_ram;

	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();

	void update_memory_mapping();

	DECLARE_WRITE8_MEMBER( qx10_18_w );
	DECLARE_WRITE8_MEMBER( prom_sel_w );
	DECLARE_WRITE8_MEMBER( cmos_sel_w );
	void qx10_upd765_interrupt(bool state);
	void drq_w(bool state);
	DECLARE_READ8_MEMBER( fdc_dma_r );
	DECLARE_WRITE8_MEMBER( fdc_dma_w );
	DECLARE_WRITE8_MEMBER( fdd_motor_w );
	DECLARE_READ8_MEMBER( qx10_30_r );
	DECLARE_READ8_MEMBER( gdc_dack_r );
	DECLARE_WRITE8_MEMBER( gdc_dack_w );
	DECLARE_WRITE_LINE_MEMBER( tc_w );
	DECLARE_READ8_MEMBER( mc146818_r );
	DECLARE_WRITE8_MEMBER( mc146818_w );
	DECLARE_READ8_MEMBER( get_slave_ack );
	DECLARE_READ8_MEMBER( vram_bank_r );
	DECLARE_WRITE8_MEMBER( vram_bank_w );
	DECLARE_READ8_MEMBER( vram_r );
	DECLARE_WRITE8_MEMBER( vram_w );
	DECLARE_READ8_MEMBER(memory_read_byte);
	DECLARE_WRITE8_MEMBER(memory_write_byte);
	DECLARE_WRITE_LINE_MEMBER(keyboard_clk);
	DECLARE_WRITE_LINE_MEMBER(keyboard_irq);

	UINT8 *m_char_rom;

	/* FDD */
	int     m_fdcint;
	int     m_fdcmotor;
	int     m_fdcready;

	/* memory */
	int     m_membank;
	int     m_memprom;
	int     m_memcmos;
	UINT8   m_cmosram[0x800];

	UINT8 m_color_mode;

	struct{
		UINT8 rx;
	}m_rs232c;

	virtual void palette_init();
	DECLARE_INPUT_CHANGED_MEMBER(key_stroke);
	DECLARE_WRITE_LINE_MEMBER(dma_hrq_changed);
	IRQ_CALLBACK_MEMBER(irq_callback);
	required_device<cpu_device> m_maincpu;
	required_device<ram_device> m_ram;
};

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
	qx10_state *state = device->machine().driver_data<qx10_state>();
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
	int xi,gfx[3];
	UINT8 pen;

	if(state->m_color_mode)
	{
		gfx[0] = state->m_video_ram[(address) + 0x00000];
		gfx[1] = state->m_video_ram[(address) + 0x20000];
		gfx[2] = state->m_video_ram[(address) + 0x40000];
	}
	else
	{
		gfx[0] = state->m_video_ram[address];
		gfx[1] = 0;
		gfx[2] = 0;
	}

	for(xi=0;xi<8;xi++)
	{
		pen = ((gfx[0] >> (7-xi)) & 1) ? 1 : 0;
		pen|= ((gfx[1] >> (7-xi)) & 1) ? 2 : 0;
		pen|= ((gfx[2] >> (7-xi)) & 1) ? 4 : 0;

		bitmap.pix32(y, x + xi) = palette[pen];
	}
}

static UPD7220_DRAW_TEXT_LINE( hgdc_draw_text )
{
	qx10_state *state = device->machine().driver_data<qx10_state>();
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
	int x;
	int xi,yi;
	int tile;
	int attr;
	UINT8 color;
	UINT8 tile_data;
	UINT8 pen;

	for( x = 0; x < pitch; x++ )
	{
		tile = state->m_video_ram[(addr+x)*2];
		attr = state->m_video_ram[(addr+x)*2+1];

		color = (state->m_color_mode) ? 1 : (attr & 4) ? 2 : 1; /* TODO: color mode */

		for( yi = 0; yi < lr; yi++)
		{
			tile_data = (state->m_char_rom[tile*16+yi]);

			if(attr & 8)
				tile_data^=0xff;

			if(cursor_on && cursor_addr == addr+x) //TODO
				tile_data^=0xff;

			if(attr & 0x80 && device->machine().primary_screen->frame_number() & 0x10) //TODO: check for blinking interval
				tile_data=0;

			for( xi = 0; xi < 8; xi++)
			{
				int res_x,res_y;

				res_x = x * 8 + xi;
				res_y = y * lr + yi;

				if(!device->machine().primary_screen->visible_area().contains(res_x, res_y))
					continue;

				if(yi >= 16)
					pen = 0;
				else
					pen = ((tile_data >> xi) & 1) ? color : 0;

				if(pen)
					bitmap.pix32(res_y, res_x) = palette[pen];
			}
		}
	}
}

UINT32 qx10_state::screen_update( screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect )
{
	bitmap.fill(get_black_pen(machine()), cliprect);

	m_hgdc->screen_update(screen, bitmap, cliprect);

	return 0;
}

/*
    Memory
*/
void qx10_state::update_memory_mapping()
{
	int drambank = 0;

	if (m_membank & 1)
	{
		drambank = 0;
	}
	else if (m_membank & 2)
	{
		drambank = 1;
	}
	else if (m_membank & 4)
	{
		drambank = 2;
	}
	else if (m_membank & 8)
	{
		drambank = 3;
	}

	if (!m_memprom)
	{
		membank("bank1")->set_base(memregion("maincpu")->base());
	}
	else
	{
		membank("bank1")->set_base(m_ram->pointer() + drambank*64*1024);
	}
	if (m_memcmos)
	{
		membank("bank2")->set_base(m_cmosram);
	}
	else
	{
		membank("bank2")->set_base(m_ram->pointer() + drambank*64*1024 + 32*1024);
	}
}

READ8_MEMBER( qx10_state::fdc_dma_r )
{
	return m_fdc->dma_r();
}

WRITE8_MEMBER( qx10_state::fdc_dma_w )
{
	m_fdc->dma_w(data);
}


WRITE8_MEMBER( qx10_state::qx10_18_w )
{
	m_membank = (data >> 4) & 0x0f;
	update_memory_mapping();
}

WRITE8_MEMBER( qx10_state::prom_sel_w )
{
	m_memprom = data & 1;
	update_memory_mapping();
}

WRITE8_MEMBER( qx10_state::cmos_sel_w )
{
	m_memcmos = data & 1;
	update_memory_mapping();
}

/*
    FDD
*/

static SLOT_INTERFACE_START( qx10_floppies )
	SLOT_INTERFACE( "525dd", FLOPPY_525_DD )
SLOT_INTERFACE_END

void qx10_state::qx10_upd765_interrupt(bool state)
{
	m_fdcint = state;

	//logerror("Interrupt from upd765: %d\n", state);
	// signal interrupt
	m_pic_m->ir6_w(state);
}

void qx10_state::drq_w(bool state)
{
	m_dma_1->dreq0_w(!state);
}

WRITE8_MEMBER( qx10_state::fdd_motor_w )
{
	m_fdcmotor = 1;

	machine().device<floppy_connector>("upd765:0")->get_device()->mon_w(false);
	// motor off controlled by clock
}

READ8_MEMBER( qx10_state::qx10_30_r )
{
	floppy_image_device *floppy1,*floppy2;

	floppy1 = machine().device<floppy_connector>("upd765:0")->get_device();
	floppy2 = machine().device<floppy_connector>("upd765:1")->get_device();

	return m_fdcint |
			/*m_fdcmotor*/ 0 << 1 |
			((floppy1 != NULL) || (floppy2 != NULL) ? 1 : 0) << 3 |
			m_membank << 4;
}

/*
    DMA8237
*/
WRITE_LINE_MEMBER(qx10_state::dma_hrq_changed)
{
	/* Assert HLDA */
	m_dma_1->hack_w(state);
}

READ8_MEMBER( qx10_state::gdc_dack_r )
{
	logerror("GDC DACK read\n");
	return 0;
}

WRITE8_MEMBER( qx10_state::gdc_dack_w )
{
	logerror("GDC DACK write %02x\n", data);
}

WRITE_LINE_MEMBER( qx10_state::tc_w )
{
	/* floppy terminal count */
	m_fdc->tc_w(!state);
}

/*
    8237 DMA (Master)
    Channel 1: Floppy disk
    Channel 2: GDC
    Channel 3: Option slots
*/
READ8_MEMBER(qx10_state::memory_read_byte)
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM);
	return prog_space.read_byte(offset);
}

WRITE8_MEMBER(qx10_state::memory_write_byte)
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM);
	return prog_space.write_byte(offset, data);
}

static I8237_INTERFACE( qx10_dma8237_1_interface )
{
	DEVCB_DRIVER_LINE_MEMBER(qx10_state,dma_hrq_changed),
	DEVCB_DRIVER_LINE_MEMBER(qx10_state, tc_w),
	DEVCB_DRIVER_MEMBER(qx10_state, memory_read_byte),
	DEVCB_DRIVER_MEMBER(qx10_state, memory_write_byte),
	{ DEVCB_DRIVER_MEMBER(qx10_state, fdc_dma_r), DEVCB_DRIVER_MEMBER(qx10_state, gdc_dack_r),/*DEVCB_DEVICE_HANDLER("upd7220", upd7220_dack_r)*/ DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_DRIVER_MEMBER(qx10_state, fdc_dma_w), DEVCB_DRIVER_MEMBER(qx10_state, gdc_dack_w),/*DEVCB_DEVICE_HANDLER("upd7220", upd7220_dack_w)*/ DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};

/*
    8237 DMA (Slave)
    Channel 1: Option slots #1
    Channel 2: Option slots #2
    Channel 3: Option slots #3
    Channel 4: Option slots #4
*/
static I8237_INTERFACE( qx10_dma8237_2_interface )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};

/*
    8255
*/
static I8255_INTERFACE(qx10_i8255_interface)
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/*
    MC146818
*/

WRITE8_MEMBER(qx10_state::mc146818_w)
{
	m_rtc->write(space, !offset, data);
}

READ8_MEMBER(qx10_state::mc146818_r)
{
	return m_rtc->read(space, !offset);
}

/*
    UPD7201
    Channel A: Keyboard
    Channel B: RS232
*/

static UPD7201_INTERFACE(qx10_upd7201_interface)
{
	0, 0, 0, 0, // channel b clock set by pit2 channel 2

	DEVCB_DEVICE_LINE_MEMBER("kbd", serial_keyboard_device, tx_r),
	DEVCB_DEVICE_LINE_MEMBER("kbd", serial_keyboard_device, rx_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE_MEMBER(RS232_TAG, serial_port_device, rx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_TAG, serial_port_device, tx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_TAG, rs232_port_device, dtr_w),
	DEVCB_DEVICE_LINE_MEMBER(RS232_TAG, rs232_port_device, rts_w),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_LINE_MEMBER(qx10_state, keyboard_irq)
};

static struct serial_keyboard_interface qx10_keyboard_interface =
{
	DEVCB_NULL
};

WRITE_LINE_MEMBER(qx10_state::keyboard_irq)
{
	m_scc->m1_r(); // always set
	m_pic_m->ir4_w(state);
}

WRITE_LINE_MEMBER(qx10_state::keyboard_clk)
{
	// clock keyboard too
	m_scc->rxca_w(state);
	m_scc->txca_w(state);
}

/*
    Timer 0
    Counter CLK                         Gate                    OUT             Operation
    0       Keyboard clock (1200bps)    Memory register D0      Speaker timer   Speaker timer (100ms)
    1       Keyboard clock (1200bps)    +5V                     8259A (10E) IR5 Software timer
    2       Clock 1,9668MHz             Memory register D7      8259 (12E) IR1  Software timer
*/

static const struct pit8253_interface qx10_pit8253_1_config =
{
	{
		{ 1200,         DEVCB_NULL,     DEVCB_NULL },
		{ 1200,         DEVCB_LINE_VCC, DEVCB_NULL },
		{ MAIN_CLK / 8, DEVCB_NULL,     DEVCB_NULL },
	}
};

/*
    Timer 1
    Counter CLK                 Gate        OUT                 Operation
    0       Clock 1,9668MHz     +5V         Speaker frequency   1kHz
    1       Clock 1,9668MHz     +5V         Keyboard clock      1200bps (Clock / 1664)
    2       Clock 1,9668MHz     +5V         RS-232C baud rate   9600bps (Clock / 208)
*/
static const struct pit8253_interface qx10_pit8253_2_config =
{
	{
		{ MAIN_CLK / 8, DEVCB_LINE_VCC, DEVCB_NULL },
		{ MAIN_CLK / 8, DEVCB_LINE_VCC, DEVCB_DRIVER_LINE_MEMBER(qx10_state, keyboard_clk) },
		{ MAIN_CLK / 8, DEVCB_LINE_VCC, DEVCB_DEVICE_LINE_MEMBER("upd7201", z80dart_device, rxtxcb_w) },
	}
};


/*
    Master PIC8259
    IR0     Power down detection interrupt
    IR1     Software timer #1 interrupt
    IR2     External interrupt INTF1
    IR3     External interrupt INTF2
    IR4     Keyboard/RS232 interrupt
    IR5     CRT/lightpen interrupt
    IR6     Floppy controller interrupt
    IR7     Slave cascade
*/

READ8_MEMBER( qx10_state::get_slave_ack )
{
	if (offset==7) { // IRQ = 7
		return m_pic_s->inta_r();
	}
	return 0x00;
}


/*
    Slave PIC8259
    IR0     Printer interrupt
    IR1     External interrupt #1
    IR2     Calendar clock interrupt
    IR3     External interrupt #2
    IR4     External interrupt #3
    IR5     Software timer #2 interrupt
    IR6     External interrupt #4
    IR7     External interrupt #5

*/

IRQ_CALLBACK_MEMBER(qx10_state::irq_callback)
{
	return m_pic_m->acknowledge();
}

#if 0
READ8_MEMBER( qx10_state::upd7201_r )
{
	if((offset & 2) == 0)
	{
		return m_keyb.rx;
	}
	//printf("R [%02x]\n",offset);

	return m_rs232c.rx;
}

WRITE8_MEMBER( qx10_state::upd7201_w )
{
	if((offset & 2) == 0) //keyb TX
	{
		switch(data & 0xe0)
		{
			case 0x00:
				m_keyb.repeat_start_time = 300+(data & 0x1f)*25;
				printf("keyb Set repeat start time, %d ms\n",m_keyb.repeat_start_time);
				break;
			case 0x20:
				m_keyb.repeat_interval = 30+(data & 0x1f)*5;
				printf("keyb Set repeat interval, %d ms\n",m_keyb.repeat_interval);
				break;
			case 0x40:
				m_keyb.led[(data & 0xe) >> 1] = data & 1;
				printf("keyb Set led %02x %s\n",((data & 0xe) >> 1),data & 1 ? "on" : "off");
				m_keyb.rx = (data & 0xf) | 0xc0;
				m_pic_m->ir4_w(1);
				break;
			case 0x60:
				printf("keyb Read LED status\n");
				// 0x80 + data
				break;
			case 0x80:
				printf("keyb Read SW status\n");
				// 0xc0 + data
				break;
			case 0xa0:
				m_keyb.repeat = data & 1;
				//printf("keyb repeat flag issued %s\n",data & 1 ? "on" : "off");
				break;
			case 0xc0:
				m_keyb.enable = data & 1;
				printf("keyb Enable flag issued %s\n",data & 1 ? "on" : "off");
				break;
			case 0xe0:
				printf("keyb Reset Issued, diagnostic is %s\n",data & 1 ? "on" : "off");
				m_keyb.rx = 0;
				break;
		}
	}
	else //RS-232c TX
	{
		//printf("RS-232c W %02x\n",data);
		if(data == 0x01) //cheap, but needed for working inputs in "The QX-10 Diagnostic"
			m_rs232c.rx = 0x04;
		else if(data == 0x00)
			m_rs232c.rx = 0xfe;
		else
			m_rs232c.rx = 0xff;
	}

}
#endif

READ8_MEMBER( qx10_state::vram_bank_r )
{
	return m_vram_bank;
}

WRITE8_MEMBER( qx10_state::vram_bank_w )
{
	if(m_color_mode)
	{
		m_vram_bank = data & 7;
		if(data != 1 && data != 2 && data != 4)
			printf("%02x\n",data);
	}
}

static ADDRESS_MAP_START(qx10_mem, AS_PROGRAM, 8, qx10_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_RAMBANK("bank1")
	AM_RANGE( 0x8000, 0xdfff ) AM_RAMBANK("bank2")
	AM_RANGE( 0xe000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( qx10_io , AS_IO, 8, qx10_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE("pit8253_1", pit8253_device, read, write)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("pit8253_2", pit8253_device, read, write)
	AM_RANGE(0x08, 0x09) AM_DEVREADWRITE("pic8259_master", pic8259_device, read, write)
	AM_RANGE(0x0c, 0x0d) AM_DEVREADWRITE("pic8259_slave", pic8259_device, read, write)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE("upd7201", z80dart_device, cd_ba_r, cd_ba_w)
	AM_RANGE(0x14, 0x17) AM_DEVREADWRITE("i8255", i8255_device, read, write)
	AM_RANGE(0x18, 0x1b) AM_READ_PORT("DSW") AM_WRITE(qx10_18_w)
	AM_RANGE(0x1c, 0x1f) AM_WRITE(prom_sel_w)
	AM_RANGE(0x20, 0x23) AM_WRITE(cmos_sel_w)
	AM_RANGE(0x2c, 0x2c) AM_READ_PORT("CONFIG")
	AM_RANGE(0x2d, 0x2d) AM_READWRITE(vram_bank_r,vram_bank_w)
	AM_RANGE(0x30, 0x33) AM_READWRITE(qx10_30_r, fdd_motor_w)
	AM_RANGE(0x34, 0x35) AM_DEVICE("upd765", upd765a_device, map)
	AM_RANGE(0x38, 0x39) AM_DEVREADWRITE("upd7220", upd7220_device, read, write)
//  AM_RANGE(0x3a, 0x3a) GDC zoom
//  AM_RANGE(0x3b, 0x3b) GDC light pen req
	AM_RANGE(0x3c, 0x3d) AM_READWRITE(mc146818_r, mc146818_w)
	AM_RANGE(0x40, 0x4f) AM_DEVREADWRITE("8237dma_1", am9517a_device, read, write)
	AM_RANGE(0x50, 0x5f) AM_DEVREADWRITE("8237dma_2", am9517a_device, read, write)
//  AM_RANGE(0xfc, 0xfd) Multi-Font comms
ADDRESS_MAP_END

/* Input ports */
/* TODO: shift break */
/*INPUT_CHANGED_MEMBER(qx10_state::key_stroke)
{
    if(newval && !oldval)
    {
        m_keyb.rx = (UINT8)(FPTR)(param) & 0x7f;
        m_pic_m->ir4_w(1);
    }

    if(oldval && !newval)
        m_keyb.rx = 0;
}*/

static INPUT_PORTS_START( qx10 )
	/* TODO: All of those have unknown meaning */
	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x00, "DSW" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) ) //CMOS related
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("CONFIG")
	PORT_CONFNAME( 0x03, 0x02, "Video Board" )
	PORT_CONFSETTING( 0x02, "Monochrome" )
	PORT_CONFSETTING( 0x01, "Color" )
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


void qx10_state::machine_start()
{
	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(qx10_state::irq_callback),this));
	m_fdc->setup_intrq_cb(upd765a_device::line_cb(FUNC(qx10_state::qx10_upd765_interrupt), this));
	m_fdc->setup_drq_cb(upd765a_device::line_cb(FUNC(qx10_state::drq_w), this));
}

void qx10_state::machine_reset()
{
	m_dma_1->dreq0_w(1);

	m_memprom = 0;
	m_memcmos = 0;
	m_membank = 0;
	update_memory_mapping();

	{
		int i;

		/* TODO: is there a bit that sets this up? */
		m_color_mode = ioport("CONFIG")->read() & 1;

		if(m_color_mode) //color
		{
			for ( i = 0; i < 8; i++ )
				palette_set_color_rgb(machine(), i, pal1bit((i >> 2) & 1), pal1bit((i >> 1) & 1), pal1bit((i >> 0) & 1));
		}
		else //monochrome
		{
			for ( i = 0; i < 8; i++ )
				palette_set_color_rgb(machine(), i, pal1bit(0), pal1bit(0), pal1bit(0));

			palette_set_color_rgb(machine(), 1, 0x00, 0x9f, 0x00);
			palette_set_color_rgb(machine(), 2, 0x00, 0xff, 0x00);
			m_vram_bank = 0;
		}
	}
}

/* F4 Character Displayer */
static const gfx_layout qx10_charlayout =
{
	8, 16,                  /* 8 x 16 characters */
	RGN_FRAC(1,1),          /* 128 characters */
	1,                  /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8, 8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16                    /* every char takes 16 bytes */
};

static GFXDECODE_START( qx10 )
	GFXDECODE_ENTRY( "chargen", 0x0000, qx10_charlayout, 1, 1 )
GFXDECODE_END

void qx10_state::video_start()
{
	// allocate memory
	m_video_ram = auto_alloc_array_clear(machine(), UINT8, 0x60000);

	// find memory regions
	m_char_rom = memregion("chargen")->base();
}

static UPD7220_INTERFACE( hgdc_intf )
{
	hgdc_display_pixels,
	hgdc_draw_text,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

void qx10_state::palette_init()
{
	// ...
}

READ8_MEMBER( qx10_state::vram_r )
{
	int bank = 0;

	if (m_vram_bank & 1)     { bank = 0; } // B
	else if(m_vram_bank & 2) { bank = 1; } // G
	else if(m_vram_bank & 4) { bank = 2; } // R

	return m_video_ram[offset + (0x20000 * bank)];
}

WRITE8_MEMBER( qx10_state::vram_w )
{
	int bank = 0;

	if (m_vram_bank & 1)     { bank = 0; } // B
	else if(m_vram_bank & 2) { bank = 1; } // G
	else if(m_vram_bank & 4) { bank = 2; } // R

	m_video_ram[offset + (0x20000 * bank)] = data;
}

static ADDRESS_MAP_START( upd7220_map, AS_0, 8, qx10_state )
	AM_RANGE(0x00000, 0x5ffff) AM_READWRITE(vram_r,vram_w)
ADDRESS_MAP_END

//-------------------------------------------------
//  rs232_port_interface rs232_intf
//-------------------------------------------------

static const rs232_port_interface rs232_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_CONFIG_START( qx10, qx10_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, MAIN_CLK / 4)
	MCFG_CPU_PROGRAM_MAP(qx10_mem)
	MCFG_CPU_IO_MAP(qx10_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(qx10_state, screen_update)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_GFXDECODE(qx10)
	MCFG_PALETTE_LENGTH(8)

	/* Devices */
	MCFG_PIT8253_ADD("pit8253_1", qx10_pit8253_1_config)
	MCFG_PIT8253_ADD("pit8253_2", qx10_pit8253_2_config)
	MCFG_PIC8259_ADD("pic8259_master", INPUTLINE("maincpu", 0), VCC, READ8(qx10_state, get_slave_ack))
	MCFG_PIC8259_ADD("pic8259_slave", DEVWRITELINE("pic8259_master", pic8259_device, ir7_w), GND, NULL)
	MCFG_UPD7201_ADD("upd7201", MAIN_CLK/4, qx10_upd7201_interface)
	MCFG_I8255_ADD("i8255", qx10_i8255_interface)
	MCFG_I8237_ADD("8237dma_1", MAIN_CLK/4, qx10_dma8237_1_interface)
	MCFG_I8237_ADD("8237dma_2", MAIN_CLK/4, qx10_dma8237_2_interface)
	MCFG_UPD7220_ADD("upd7220", MAIN_CLK/6, hgdc_intf, upd7220_map) // unk clock
	MCFG_MC146818_IRQ_ADD( "rtc", MC146818_STANDARD, DEVWRITELINE("pic8259_slave", pic8259_device, ir2_w))
	MCFG_UPD765A_ADD("upd765", true, true)
	MCFG_FLOPPY_DRIVE_ADD("upd765:0", qx10_floppies, "525dd", floppy_image_device::default_floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("upd765:1", qx10_floppies, "525dd", floppy_image_device::default_floppy_formats)
	MCFG_RS232_PORT_ADD(RS232_TAG, rs232_intf, default_rs232_devices, NULL)
	MCFG_QX10_KEYBOARD_ADD("kbd", qx10_keyboard_interface)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("256K")

	// software lists
	MCFG_SOFTWARE_LIST_ADD("flop_list", "qx10_flop")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( qx10 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v006", "v0.06")
	ROMX_LOAD( "ipl006.bin", 0x0000, 0x0800, CRC(3155056a) SHA1(67cc0ae5055d472aa42eb40cddff6da69ffc6553), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v003", "v0.03")
	ROMX_LOAD( "ipl003.bin", 0x0000, 0x0800, CRC(3cbc4008) SHA1(cc8c7d1aa0cca8f9753d40698b2dc6802fd5f890), ROM_BIOS(2))

	/* This is probably the i8039 program ROM for the Q10MF Multifont card, and the actual font ROMs are missing (6 * HM43128) */
	/* The first part of this rom looks like code for an embedded controller?
	    From 0300 on, is a keyboard lookup table */
	ROM_REGION( 0x0800, "i8039", 0 )
	ROM_LOAD( "m12020a.3e", 0x0000, 0x0800, CRC(fa27f333) SHA1(73d27084ca7b002d5f370220d8da6623a6e82132))

	ROM_REGION( 0x1000, "chargen", 0 )
//  ROM_LOAD( "qge.2e",   0x0000, 0x0800, BAD_DUMP CRC(ed93cb81) SHA1(579e68bde3f4184ded7d89b72c6936824f48d10b))  //this one contains special characters only
	ROM_LOAD( "qge.2e",   0x0000, 0x1000, BAD_DUMP CRC(eb31a2d5) SHA1(6dc581bf2854a07ae93b23b6dfc9c7abd3c0569e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, qx10,  0,       0,  qx10,   qx10, driver_device,     0,       "Epson",   "QX-10",       GAME_NOT_WORKING | GAME_NO_SOUND )
