/************************************************************************************

Game Magic (c) 1997 Bally Gaming Co.

Preliminary driver by Grull Osgo

Game Magic

Is a Multigame machine build on a Bally's V8000 platform.

This is the first PC based gaming machine developed by Bally Gaming.

V8000 platform includes:

1 Motherboard MICRONICS M55Hi-Plus PCI/ISA, Chipset INTEL i430HX (TRITON II), 64 MB Ram (4 SIMM M x 16 MB SIMM)
On board Sound Blaster Vibra 16C chipset.
1 TOSHIBA CD-ROM or DVD-ROM Drive w/Bootable CD-ROM with Game.
1 OAK SVGA PCI Video Board.
1 Voodoo Graphics PCI Video Board, connected to the monitor.
1 21" SVGA Color Monitor, 16x9 Aspect, Vertical mount, with touchscreen.
1 Bally's IO-Board, Based on 68000 procesor as interface to all gaming devices
(Buttons, Lamps, Switches, Coin acceptor, Bill Validator, Hopper, Touchscreen, etc...)

PC and IO-Board communicates via RS-232 Serial Port.

Additional CD-ROM games: "99 Bottles of Beer"

*************************************************************************************/

#include "emu.h"
#include "cpu/i386/i386.h"
#include "machine/cr589.h"
#include "machine/8237dma.h"
#include "machine/pic8259.h"
//#include "machine/i82371sb.h"
//#include "machine/i82439tx.h"
#include "machine/pit8253.h"
#include "machine/mc146818.h"
#include "machine/pci.h"
#include "machine/8042kbdc.h"
#include "machine/pckeybrd.h"
#include "video/pc_vga.h"

#define ATAPI_CYCLES_PER_SECTOR (5000)  // plenty of time to allow DMA setup etc.  BIOS requires this be at least 2000, individual games may vary.

#define ATAPI_ERRFEAT_ABRT 0x04

#define ATAPI_STAT_BSY     0x80
#define ATAPI_STAT_DRDY    0x40
#define ATAPI_STAT_DMARDDF 0x20
#define ATAPI_STAT_SERVDSC 0x10
#define ATAPI_STAT_DRQ     0x08
#define ATAPI_STAT_CORR    0x04
#define ATAPI_STAT_CHECK   0x01

#define ATAPI_INTREASON_COMMAND 0x01
#define ATAPI_INTREASON_IO      0x02
#define ATAPI_INTREASON_RELEASE 0x04

#define ATAPI_REG_DATA      0
#define ATAPI_REG_ERRFEAT   1
#define ATAPI_REG_INTREASON 2
#define ATAPI_REG_SAMTAG    3
#define ATAPI_REG_COUNTLOW  4
#define ATAPI_REG_COUNTHIGH 5
#define ATAPI_REG_DRIVESEL  6
#define ATAPI_REG_CMDSTATUS 7
#define ATAPI_REG_MAX 16

#define ATAPI_DATA_SIZE ( 64 * 1024 )

#define MAX_TRANSFER_SIZE ( 63488 )

class gammagic_state : public driver_device
{
public:
	gammagic_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_pit8254(*this, "pit8254" ),
		m_pic8259_1(*this, "pic8259_1" ),
		m_pic8259_2(*this,  "pic8259_2" ),
		m_dma8237_1(*this, "dma8237_1" ),
		m_dma8237_2(*this, "dma8237_2" ),
		m_maincpu(*this, "maincpu") { }

	int m_dma_channel;
	UINT8 m_dma_offset[2][4];
	UINT8 m_at_pages[0x10];

	required_device<device_t> m_pit8254;
	required_device<device_t> m_pic8259_1;
	required_device<device_t> m_pic8259_2;
	required_device<i8237_device> m_dma8237_1;
	required_device<i8237_device> m_dma8237_2;

	emu_timer *m_atapi_timer;
	//SCSIInstance *m_inserted_cdrom;

	int m_atapi_data_ptr;
	int m_atapi_data_len;
	int m_atapi_xferlen;
	int m_atapi_xferbase;
	int m_atapi_cdata_wait;
	int m_atapi_xfermod;
	/* memory */
	UINT8 m_atapi_regs[ATAPI_REG_MAX];
	UINT8 m_atapi_data[ATAPI_DATA_SIZE];

	DECLARE_DRIVER_INIT(gammagic);
	IRQ_CALLBACK_MEMBER(irq_callback);
	DECLARE_READ8_MEMBER(get_out2);
	DECLARE_READ8_MEMBER(at_page8_r);
	DECLARE_WRITE8_MEMBER(at_page8_w);
	DECLARE_READ8_MEMBER(pc_dma_read_byte);
	DECLARE_WRITE8_MEMBER(pc_dma_write_byte);
	DECLARE_READ8_MEMBER(at_dma8237_2_r);
	DECLARE_WRITE8_MEMBER(at_dma8237_2_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dma_hrq_changed);
	void set_dma_channel(int channel, int state);
	DECLARE_WRITE_LINE_MEMBER(pc_dack0_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack1_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack2_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack3_w);
	DECLARE_WRITE_LINE_MEMBER(gammagic_pic8259_1_set_int_line);
	DECLARE_READ8_MEMBER(get_slave_ack);

	virtual void machine_start();
	virtual void machine_reset();
	void atapi_init();
	required_device<cpu_device> m_maincpu;
};

READ8_MEMBER(gammagic_state::at_dma8237_2_r)
{
	return m_dma8237_2->i8237_r(space, offset / 2);
}

WRITE8_MEMBER(gammagic_state::at_dma8237_2_w)
{
	m_dma8237_2->i8237_w(space, offset / 2, data);
}

READ8_MEMBER(gammagic_state::at_page8_r)
{
	UINT8 data = m_at_pages[offset % 0x10];

	switch(offset % 8) {
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


WRITE8_MEMBER(gammagic_state::at_page8_w)
{
	m_at_pages[offset % 0x10] = data;

	switch(offset % 8) {
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


WRITE_LINE_MEMBER(gammagic_state::pc_dma_hrq_changed)
{
	m_maincpu->set_input_line(INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	m_dma8237_1->i8237_hlda_w(state);
}


READ8_MEMBER(gammagic_state::pc_dma_read_byte)
{
	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0xFF0000;

	return space.read_byte(page_offset + offset);
}


WRITE8_MEMBER(gammagic_state::pc_dma_write_byte)
{
	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0xFF0000;

	space.write_byte(page_offset + offset, data);
}

void gammagic_state::set_dma_channel(int channel, int state)
{
	if (!state) m_dma_channel = channel;
}

WRITE_LINE_MEMBER(gammagic_state::pc_dack0_w){ set_dma_channel(0, state); }
WRITE_LINE_MEMBER(gammagic_state::pc_dack1_w){ set_dma_channel(1, state); }
WRITE_LINE_MEMBER(gammagic_state::pc_dack2_w){ set_dma_channel(2, state); }
WRITE_LINE_MEMBER(gammagic_state::pc_dack3_w){ set_dma_channel(3, state); }

static I8237_INTERFACE( dma8237_1_config )
{
	DEVCB_DRIVER_LINE_MEMBER(gammagic_state,pc_dma_hrq_changed),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(gammagic_state, pc_dma_read_byte),
	DEVCB_DRIVER_MEMBER(gammagic_state, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_DRIVER_LINE_MEMBER(gammagic_state,pc_dack0_w), DEVCB_DRIVER_LINE_MEMBER(gammagic_state,pc_dack1_w), DEVCB_DRIVER_LINE_MEMBER(gammagic_state,pc_dack2_w), DEVCB_DRIVER_LINE_MEMBER(gammagic_state,pc_dack3_w) }
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
/*
READ32_MEMBER( gammagic_state::atapi_r )
{
    UINT8 *atapi_regs = m_atapi_regs;
    //running_machine &machine = space.machine();
    int reg, data;

    if (mem_mask == 0x0000ffff) // word-wide command read
    {
      logerror("ATAPI: packet read = %04x\n", m_atapi_data[m_atapi_data_ptr]);

        // assert IRQ and drop DRQ
        if (m_atapi_data_ptr == 0 && m_atapi_data_len == 0)
        {
            // get the data from the device
            if( m_atapi_xferlen > 0 )
            {
                SCSIReadData( m_inserted_cdrom, m_atapi_data, m_atapi_xferlen );
                m_atapi_data_len = m_atapi_xferlen;
            }

            if (m_atapi_xfermod > MAX_TRANSFER_SIZE)
            {
                m_atapi_xferlen = MAX_TRANSFER_SIZE;
                m_atapi_xfermod = m_atapi_xfermod - MAX_TRANSFER_SIZE;
            }
            else
            {
                m_atapi_xferlen = m_atapi_xfermod;
                m_atapi_xfermod = 0;
            }

            //verboselog\\( machine, 2, "atapi_r: atapi_xferlen=%d\n", m_atapi_xferlen );
            if( m_atapi_xferlen != 0 )
            {
                atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
                atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
            }
            else
            {
                logerror("ATAPI: dropping DRQ\n");
                atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
                atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
            }

            atapi_regs[ATAPI_REG_COUNTLOW] = m_atapi_xferlen & 0xff;
            atapi_regs[ATAPI_REG_COUNTHIGH] = (m_atapi_xferlen>>8)&0xff;

            atapi_irq(space.machine(), ASSERT_LINE);
        }

        if( m_atapi_data_ptr < m_atapi_data_len )
        {
            data = m_atapi_data[m_atapi_data_ptr++];
            data |= ( m_atapi_data[m_atapi_data_ptr++] << 8 );
            if( m_atapi_data_ptr >= m_atapi_data_len )
            {

                m_atapi_data_ptr = 0;
                m_atapi_data_len = 0;

                if( m_atapi_xferlen == 0 )
                {
                    atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
                    atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
                    atapi_irq(space.machine(), ASSERT_LINE);
                }
            }
        }
        else
        {
            data = 0;
        }
    }
    else
    {
        atapi_irq(space.machine(), CLEAR_LINE);
        int shift;
        shift = 0;
        reg = offset<<2;
        switch(mem_mask)
        {
        case 0x000000ff:
            break;
        case 0x0000ff00:
            reg+=1;
            data >>= 8;
            shift=8;
            break;
        case 0x00ff0000:
            reg+=2;
            data >>=16;
            shift=16;
            break;
        case 0xff000000:
            reg+=3;
            data >>=24;
            shift=24;
            break;
        }
        data = atapi_regs[reg];
            data <<= shift;
    }
    return data;
}

WRITE32_MEMBER( gammagic_state::atapi_w )
{
    UINT8 *atapi_regs = m_atapi_regs;
    UINT8 *atapi_data = m_atapi_data;
    int reg;
    if (mem_mask == 0x0000ffff) // word-wide command write
    {
        atapi_data[m_atapi_data_ptr++] = data & 0xff;
        atapi_data[m_atapi_data_ptr++] = data >> 8;

        if (m_atapi_cdata_wait)
        {
            logerror("ATAPI: waiting, ptr %d wait %d\n", m_atapi_data_ptr, m_atapi_cdata_wait);
            if (m_atapi_data_ptr == m_atapi_cdata_wait)
            {
                // send it to the device
                SCSIWriteData( m_inserted_cdrom, atapi_data, m_atapi_cdata_wait );

                // assert IRQ
                atapi_irq(space.machine(), ASSERT_LINE);

                // not sure here, but clear DRQ at least?
                atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
            }
        }

        else if ( m_atapi_data_ptr == 12 )
        {
            int phase;
            // reset data pointer for reading SCSI results
            m_atapi_data_ptr = 0;
            m_atapi_data_len = 0;

            // send it to the SCSI device
            SCSISetCommand( m_inserted_cdrom, m_atapi_data, 12 );
            SCSIExecCommand( m_inserted_cdrom, &m_atapi_xferlen );
            SCSIGetPhase( m_inserted_cdrom, &phase );

            if (m_atapi_xferlen != -1)
            {
                        logerror("ATAPI: SCSI command %02x returned %d bytes from the device\n", atapi_data[0]&0xff, m_atapi_xferlen);

                // store the returned command length in the ATAPI regs, splitting into
                // multiple transfers if necessary
                m_atapi_xfermod = 0;
                if (m_atapi_xferlen > MAX_TRANSFER_SIZE)
                {
                    m_atapi_xfermod = m_atapi_xferlen - MAX_TRANSFER_SIZE;
                    m_atapi_xferlen = MAX_TRANSFER_SIZE;
                }

                atapi_regs[ATAPI_REG_COUNTLOW] = m_atapi_xferlen & 0xff;
                atapi_regs[ATAPI_REG_COUNTHIGH] = (m_atapi_xferlen>>8)&0xff;

                if (m_atapi_xferlen == 0)
                {
                    // if no data to return, set the registers properly
                    atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRDY;
                    atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO|ATAPI_INTREASON_COMMAND;
                }
                else
                {
                    // indicate data ready: set DRQ and DMA ready, and IO in INTREASON
                    atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
                    atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
                }

                switch( phase )
                {
                case SCSI_PHASE_DATAOUT:
                    m_atapi_cdata_wait = m_atapi_xferlen;
                    break;
                }

                // perform special ATAPI processing of certain commands
                switch (atapi_data[0]&0xff)
                {
                    case 0x00: // BUS RESET / TEST UNIT READY
                    case 0xbb: // SET CDROM SPEED
                        atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
                        break;

                    case 0x45: // PLAY
                        atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_BSY;
                        m_atapi_timer->adjust( downcast<cpu_device *>(&space->device())->cycles_to_attotime( ATAPI_CYCLES_PER_SECTOR ) );
                        break;
                }

                // assert IRQ
                atapi_irq(space.machine(), ASSERT_LINE);
            }
            else
            {
                        logerror("ATAPI: SCSI device returned error!\n");

                atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_CHECK;
                atapi_regs[ATAPI_REG_ERRFEAT] = 0x50;   // sense key = ILLEGAL REQUEST
                atapi_regs[ATAPI_REG_COUNTLOW] = 0;
                atapi_regs[ATAPI_REG_COUNTHIGH] = 0;
            }
        }
    }
    else
    {
        reg = offset<<2;
        switch(mem_mask)
        {
        case 0x000000ff:
            break;
        case 0x0000ff00:
            reg+=1;
            data >>= 8;
            break;
        case 0x00ff0000:
            reg+=2;
            data >>=16;
            break;
        case 0xff000000:
            reg+=3;
            data >>=24;
            break;
        }

        atapi_regs[reg] = data;
            logerror("ATAPI: reg %d = %x (offset %x mask %x PC=%x)\n", reg, data, offset, mem_mask, cpu_get_pc(&space->device()));

        if (reg == ATAPI_REG_CMDSTATUS)
        {
                logerror("ATAPI command %x issued! (PC=%x)\n", data, cpu_get_pc(&space->device()));
            switch (data)
            {
                case 0xa0:  // PACKET
                    atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;
                    atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_COMMAND;

                    m_atapi_data_ptr = 0;
                    m_atapi_data_len = 0;

                    // we have no data
                    m_atapi_xferlen = 0;
                    m_atapi_xfermod = 0;

                    m_atapi_cdata_wait = 0;
                    break;

                case 0xa1:  // IDENTIFY PACKET DEVICE
                    atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;

                    m_atapi_data_ptr = 0;
                    m_atapi_data_len = 512;

                    // we have no data
                    m_atapi_xferlen = 0;
                    m_atapi_xfermod = 0;

                    memset( atapi_data, 0, m_atapi_data_len );

                    atapi_data[ 0 ^ 1 ] = 0x85; // ATAPI device, cmd set 5 compliant, DRQ within 3 ms of PACKET command
                    atapi_data[ 1 ^ 1 ] = 0x80; // ATAPI device, removable media

                    memset( &atapi_data[ 46 ], ' ', 8 );
                    atapi_data[ 46 ^ 1 ] = '1';
                    atapi_data[ 47 ^ 1 ] = '.';
                    atapi_data[ 48 ^ 1 ] = '0';


                    memset( &atapi_data[ 54 ], ' ', 40 );
                    atapi_data[ 54 ^ 1 ] = 'T';
                    atapi_data[ 55 ^ 1 ] = 'O';
                    atapi_data[ 56 ^ 1 ] = 'S';
                    atapi_data[ 57 ^ 1 ] = 'H';
                    atapi_data[ 58 ^ 1 ] = 'I';
                    atapi_data[ 59 ^ 1 ] = 'B';
                    atapi_data[ 60 ^ 1 ] = 'A';
                    atapi_data[ 61 ^ 1 ] = ' ';
                    atapi_data[ 62 ^ 1 ] = 'X';
                    atapi_data[ 63 ^ 1 ] = 'M';
                    atapi_data[ 64 ^ 1 ] = '-';
                    atapi_data[ 65 ^ 1 ] = '3';
                    atapi_data[ 66 ^ 1 ] = '3';
                    atapi_data[ 67 ^ 1 ] = '0';
                    atapi_data[ 68 ^ 1 ] = '1';
                    atapi_data[ 69 ^ 1 ] = ' ';

                    atapi_data[ 98 ^ 1 ] = 0x06; // Word 49=Capabilities, IORDY may be disabled (bit_10), LBA Supported mandatory (bit_9)
                    atapi_data[ 99 ^ 1 ] = 0x00;

                    atapi_regs[ATAPI_REG_COUNTLOW] = 0;
                    atapi_regs[ATAPI_REG_COUNTHIGH] = 2;

                    atapi_irq(space.machine(), ASSERT_LINE);
                    break;
                case 0xec:  //IDENTIFY DEVICE - Must abort here and set for packet data
                    atapi_regs[ATAPI_REG_ERRFEAT] = ATAPI_ERRFEAT_ABRT;
                    atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_CHECK;

                    atapi_irq(space.machine(), ASSERT_LINE);

                case 0xef:  // SET FEATURES
                    atapi_regs[ATAPI_REG_CMDSTATUS] = 0;

                    m_atapi_data_ptr = 0;
                    m_atapi_data_len = 0;

                    atapi_irq(space.machine(), ASSERT_LINE);
                    break;

                default:
                    logerror("ATAPI: Unknown IDE command %x\n", data);
                    break;
            }
        }
    }
}
*/
// Memory is mostly handled by the chipset
static ADDRESS_MAP_START( gammagic_map, AS_PROGRAM, 32, gammagic_state )
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAM
	AM_RANGE(0x000a0000, 0x000bffff) AM_DEVREADWRITE8("vga", vga_device, mem_r, mem_w, 0xffffffff)
	AM_RANGE(0x00100000, 0x07ffffff) AM_RAM
	AM_RANGE(0x08000000, 0xfffdffff) AM_NOP
	AM_RANGE(0xfffe0000, 0xffffffff) AM_ROM AM_REGION("user", 0x20000)/* System BIOS */
ADDRESS_MAP_END

static ADDRESS_MAP_START( gammagic_io, AS_IO, 32, gammagic_state)
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_device, i8237_r, i8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8_LEGACY("pic8259_1", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8_LEGACY("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x006f) AM_DEVREADWRITE8("kbdc", kbdc8042_device, data_r, data_w, 0xffffffff)
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8("rtc", mc146818_device, read, write, 0xffffffff)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(at_page8_r, at_page8_w, 0xffffffff)
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8_LEGACY("pic8259_2", pic8259_r, pic8259_w, 0xffffffff)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE8(at_dma8237_2_r, at_dma8237_2_w, 0xffffffff)
	AM_RANGE(0x00e8, 0x00ef) AM_NOP
	AM_RANGE(0x00f0, 0x01ef) AM_NOP
	//AM_RANGE(0x01f0, 0x01f7) AM_READWRITE(atapi_r, atapi_w)
	AM_RANGE(0x01f8, 0x03ef) AM_NOP
	AM_RANGE(0x03b0, 0x03bf) AM_DEVREADWRITE8("vga", vga_device, port_03b0_r, port_03b0_w, 0xffffffff)
	AM_RANGE(0x03c0, 0x03cf) AM_DEVREADWRITE8("vga", vga_device, port_03c0_r, port_03c0_w, 0xffffffff)
	AM_RANGE(0x03d0, 0x03df) AM_DEVREADWRITE8("vga", vga_device, port_03d0_r, port_03d0_w, 0xffffffff)
	AM_RANGE(0x03f0, 0x0cf7) AM_NOP
	AM_RANGE(0x0cf8, 0x0cff) AM_DEVREADWRITE("pcibus", pci_bus_device, read, write)
	AM_RANGE(0x0400, 0xffff) AM_NOP


ADDRESS_MAP_END

#define AT_KEYB_HELPER(bit, text, key1) \
	PORT_BIT( bit, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(text) PORT_CODE(key1)

#if 1
static INPUT_PORTS_START( gammagic )
	PORT_START("pc_keyboard_0")
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED )     /* unused scancode 0 */
	AT_KEYB_HELPER( 0x0002, "Esc",          KEYCODE_Q           ) /* Esc                         01  81 */

	PORT_START("pc_keyboard_1")
	AT_KEYB_HELPER( 0x0010, "T",            KEYCODE_T           ) /* T                           14  94 */
	AT_KEYB_HELPER( 0x0020, "Y",            KEYCODE_Y           ) /* Y                           15  95 */
	AT_KEYB_HELPER( 0x0100, "O",            KEYCODE_O           ) /* O                           18  98 */
	AT_KEYB_HELPER( 0x1000, "Enter",        KEYCODE_ENTER       ) /* Enter                       1C  9C */

	PORT_START("pc_keyboard_2")

	PORT_START("pc_keyboard_3")
	AT_KEYB_HELPER( 0x0001, "B",            KEYCODE_B           ) /* B                           30  B0 */
	AT_KEYB_HELPER( 0x0002, "N",            KEYCODE_N           ) /* N                           31  B1 */
	AT_KEYB_HELPER( 0x0800, "F1",           KEYCODE_S           ) /* F1                          3B  BB */
	AT_KEYB_HELPER( 0x1000, "F2",           KEYCODE_D           ) /* F2                          3C  BC */
	AT_KEYB_HELPER( 0x4000, "F4",           KEYCODE_F           ) /* F4                          3E  BE */


	PORT_START("pc_keyboard_4")
	AT_KEYB_HELPER( 0x0004, "F8",           KEYCODE_F8          ) // f8=42  /f10=44 /minus 4a /plus=4e
	AT_KEYB_HELPER( 0x0010, "F10",          KEYCODE_F10         ) // f8=42  /f10=44 /minus 4a /plus=4e
	AT_KEYB_HELPER( 0x0100, "KP 8(UP)",     KEYCODE_8_PAD       ) /* Keypad 8  (Up arrow)        48  C8 */
	AT_KEYB_HELPER( 0x0400, "KP -",         KEYCODE_MINUS_PAD   ) // f8=42  /f10=44 /minus 4a /plus=4e
	AT_KEYB_HELPER( 0x4000, "KP +",         KEYCODE_PLUS_PAD    ) // f8=42  /f10=44 /minus 4a /plus=4e

	PORT_START("pc_keyboard_5")
	AT_KEYB_HELPER( 0x0001, "KP 2(DN)",     KEYCODE_2_PAD       ) /* Keypad 2  (Down arrow)      50  D0 */

	PORT_START("pc_keyboard_6")
	AT_KEYB_HELPER( 0x0040, "(MF2)Cursor Up",       KEYCODE_UP          ) /* Up                          67  e7 */
	AT_KEYB_HELPER( 0x0080, "(MF2)Page Up",         KEYCODE_PGUP        ) /* Page Up                     68  e8 */
	AT_KEYB_HELPER( 0x0100, "(MF2)Cursor Left",     KEYCODE_LEFT        ) /* Left                        69  e9 */
	AT_KEYB_HELPER( 0x0200, "(MF2)Cursor Right",        KEYCODE_RIGHT       ) /* Right                       6a  ea */
	AT_KEYB_HELPER( 0x0800, "(MF2)Cursor Down",     KEYCODE_DOWN        ) /* Down                        6c  ec */
	AT_KEYB_HELPER( 0x1000, "(MF2)Page Down",       KEYCODE_PGDN        ) /* Page Down                   6d  ed */
	AT_KEYB_HELPER( 0x4000, "Del",                      KEYCODE_A           ) /* Delete                      6f  ef */

	PORT_START("pc_keyboard_7")

INPUT_PORTS_END
#endif

IRQ_CALLBACK_MEMBER(gammagic_state::irq_callback)
{
	return pic8259_acknowledge(m_pic8259_1);
}

void gammagic_state::machine_start()
{
	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(gammagic_state::irq_callback),this));
}

void gammagic_state::machine_reset()
{
	//void *cd;
	//SCSIGetDevice( m_inserted_cdrom, &cd );

}


/*void gammagic_state::atapi_irq(int state)
{
    pic8259_ir6_w(m_pic8259_2, state);
}

void gammagic_state::atapi_exit(running_machine& machine)
{
    SCSIDeleteInstance(m_inserted_cdrom);

}
*/

void gammagic_state::atapi_init()
{
	m_atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
	m_atapi_regs[ATAPI_REG_ERRFEAT] = 1;
	m_atapi_regs[ATAPI_REG_COUNTLOW] = 0x14;
	m_atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb;
	m_atapi_data_ptr = 0;
	m_atapi_data_len = 0;
	m_atapi_cdata_wait = 0;

	//SCSIAllocInstance( machine, &SCSIClassCr589, &m_inserted_cdrom, ":cdrom" );

	//machine().add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(atapi_exit), &machine));

}


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

WRITE_LINE_MEMBER(gammagic_state::gammagic_pic8259_1_set_int_line)
{
	m_maincpu->set_input_line(0, state ? HOLD_LINE : CLEAR_LINE);
}

READ8_MEMBER(gammagic_state::get_slave_ack)
{
	if (offset==2) {
		return pic8259_acknowledge(m_pic8259_2);
	}
	return 0x00;
}

static const struct pic8259_interface gammagic_pic8259_1_config =
{
	DEVCB_DRIVER_LINE_MEMBER(gammagic_state,gammagic_pic8259_1_set_int_line),
	DEVCB_LINE_VCC,
	DEVCB_DRIVER_MEMBER(gammagic_state,get_slave_ack)
};

static const struct pic8259_interface gammagic_pic8259_2_config =
{
	DEVCB_DEVICE_LINE("pic8259_1", pic8259_ir2_w),
	DEVCB_LINE_GND,
	DEVCB_NULL
};

/*************************************************************
 *
 * pit8254 configuration
 *
 *************************************************************/

static const struct pit8253_config gammagic_pit8254_config =
{
	{
		{
			4772720/4,              /* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259_1", pic8259_ir0_w)
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

READ8_MEMBER(gammagic_state::get_out2)
{
	return pit8253_get_output( m_pit8254, 2 );
}

static const struct kbdc8042_interface at8042 =
{
	KBDC8042_AT386,
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_RESET),
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_A20),
	DEVCB_DEVICE_LINE_MEMBER("pic8259_1", pic8259_device, ir1_w),
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(gammagic_state,get_out2)
};

static MACHINE_CONFIG_START( gammagic, gammagic_state )
	MCFG_CPU_ADD("maincpu", PENTIUM, 133000000) // Intel Pentium 133
	MCFG_CPU_PROGRAM_MAP(gammagic_map)
	MCFG_CPU_IO_MAP(gammagic_io)
	MCFG_PIT8254_ADD( "pit8254", gammagic_pit8254_config )
	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, dma8237_1_config )
	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, dma8237_2_config )
	MCFG_PIC8259_ADD( "pic8259_1", gammagic_pic8259_1_config )
	MCFG_PIC8259_ADD( "pic8259_2", gammagic_pic8259_2_config )
	MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )
//  MCFG_I82371SB_ADD("i82371sb")
//  MCFG_I82439TX_ADD("i82439tx", "maincpu", "user")
	MCFG_PCI_BUS_ADD("pcibus", 0)
//  MCFG_PCI_BUS_DEVICE(0, "i82439tx", i82439tx_pci_read, i82439tx_pci_write)
//  MCFG_PCI_BUS_DEVICE(1, "i82371sb", i82371sb_pci_read, i82371sb_pci_write)
	/* video hardware */
	MCFG_FRAGMENT_ADD( pcvideo_vga )

	MCFG_KBDC8042_ADD("kbdc", at8042)

MACHINE_CONFIG_END


DRIVER_INIT_MEMBER(gammagic_state,gammagic)
{
	atapi_init();
}

ROM_START( gammagic )
	ROM_REGION32_LE(0x40000, "user", 0)
	//Original Memory Set
	//ROM_LOAD("m7s04.rom",   0, 0x40000, CRC(3689f5a9) SHA1(8daacdb0dc6783d2161680564ffe83ac2515f7ef))
	//ROM_LOAD("otivga_tx2953526.rom", 0x0000, 0x8000, CRC(916491af) SHA1(d64e3a43a035d70ace7a2d0603fc078f22d237e1))

	//Temp. Memory Set (Only for initial driver development stage)
	ROM_LOAD16_BYTE( "trident_tgui9680_bios.bin", 0x0000, 0x4000, CRC(1eebde64) SHA1(67896a854d43a575037613b3506aea6dae5d6a19) )
	ROM_CONTINUE(                                 0x0001, 0x4000 )
	ROM_LOAD("5hx29.bin",   0x20000, 0x20000, CRC(07719a55) SHA1(b63993fd5186cdb4f28c117428a507cd069e1f68))

	DISK_REGION( "cdrom" )
	DISK_IMAGE_READONLY( "gammagic", 0,SHA1(caa8fc885d84dbc07fb0604c76cd23c873a65ce6) )
ROM_END

ROM_START( 99bottles )
	ROM_REGION32_LE(0x40000, "user", 0)
	//Original BIOS/VGA-BIOS Rom Set
	//ROM_LOAD("m7s04.rom",   0, 0x40000, CRC(3689f5a9) SHA1(8daacdb0dc6783d2161680564ffe83ac2515f7ef))
	//ROM_LOAD("otivga_tx2953526.rom", 0x0000, 0x8000, CRC(916491af) SHA1(d64e3a43a035d70ace7a2d0603fc078f22d237e1))

	//Temporary (Chipset compatible Rom Set, only for driver development stage)
	ROM_LOAD16_BYTE( "trident_tgui9680_bios.bin", 0x0000, 0x4000, CRC(1eebde64) SHA1(67896a854d43a575037613b3506aea6dae5d6a19) )
	ROM_CONTINUE(                                 0x0001, 0x4000 )
	ROM_LOAD("5hx29.bin",   0x20000, 0x20000, CRC(07719a55) SHA1(b63993fd5186cdb4f28c117428a507cd069e1f68))

	DISK_REGION( "cdrom" )
	DISK_IMAGE_READONLY( "99bottles", 0, SHA1(0b874178c8dd3cfc451deb53dc7936dc4ad5a04f))
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
/*************************
*      Game Drivers      *
*************************/

/*     YEAR NAME       PARENT    MACHINE   INPUT     INIT       ROT   COMPANY             FULLNAME              FLAGS           */
GAME( 1999,  gammagic, 0,        gammagic, gammagic, gammagic_state, gammagic , ROT0, "Bally Gaming Co.", "Game Magic",         GAME_IS_SKELETON )
GAME( 1999, 99bottles, gammagic, gammagic, gammagic, gammagic_state, gammagic , ROT0, "Bally Gaming Co.", "99 Bottles of Beer", GAME_IS_SKELETON )
