/******************************************************************************
*
*  Bare bones Realvoice PC driver
*  By Jonathan Gevaryahu AKA Lord Nightmare
*  Binary supplied by Kevin 'kevtris' Horton
*

******************************************************************************/

/* Core includes */
#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "machine/6551.h"
//#include "dectalk.lh" //  hack to avoid screenless system crash
#include "machine/terminal.h"

/* Defines */

/* Components */
/*static UINT8 *main_ram; */

static struct
{
UINT8 data[8];
UINT8 P1DDR;
UINT8 P2DDR;
UINT8 PORT1;
UINT8 PORT2;
UINT8 P3DDR;
UINT8 P4DDR;
UINT8 PORT3;
UINT8 PORT4;
UINT8 TCSR1;
UINT8 FRCH;
UINT8 FRCL;
UINT8 OCR1H;
UINT8 OCR1L;
UINT8 ICRH;
UINT8 ICRL;
UINT8 TCSR2;
UINT8 RMCR;
UINT8 TRCSR1;
UINT8 RDR;
UINT8 TDR;
UINT8 RP5CR;
UINT8 PORT5;
UINT8 P6DDR;
UINT8 PORT6;
UINT8 PORT7;
UINT8 OCR2H;
UINT8 OCR2L;
UINT8 TCSR3;
UINT8 TCONR;
UINT8 T2CNT;
UINT8 TRCSR2;
UINT8 TSTREG;
UINT8 P5DDR;
UINT8 P6CSR;
} hd63701y0={ {0}};

static struct
{
UINT8 data[8];
UINT8 port1;
UINT8 port2;
UINT8 port3;
UINT8 port4;
UINT8 port5;
UINT8 port6;
UINT8 port7;
} rvoicepc={ {0}};
/* Devices */

static DRIVER_INIT( rvoicepc )
{
}

static MACHINE_RESET( rvoicepc )
{
/* following is from datasheet at http://datasheets.chipdb.org/Hitachi/63701/HD63701Y0.pdf */
hd63701y0.P1DDR = 0xFE; // port 1 ddr, W
hd63701y0.P2DDR = 0x00; // port 2 ddr, W
hd63701y0.PORT1 = 0x00; // port 1, R/W
hd63701y0.PORT2 = 0x00; // port 2, R/W
hd63701y0.P3DDR = 0xFE; // port 3 ddr, W
hd63701y0.P4DDR = 0x00; // port 4 ddr, W
hd63701y0.PORT3 = 0x00; // port 3, R/W
hd63701y0.PORT4 = 0x00; // port 4, R/W
hd63701y0.TCSR1 = 0x00; // timer control/status, R/W
hd63701y0.FRCH = 0x00;
hd63701y0.FRCL = 0x00;
hd63701y0.OCR1H = 0xFF;
hd63701y0.OCR1L = 0xFF;
hd63701y0.ICRH = 0x00;
hd63701y0.ICRL = 0x00;
hd63701y0.TCSR2 = 0x10;
hd63701y0.RMCR = 0xC0;
hd63701y0.TRCSR1 = 0x20;
hd63701y0.RDR = 0x00; // Recieve Data Reg, R
hd63701y0.TDR = 0x00; // Transmit Data Reg, W
hd63701y0.RP5CR = 0x78; // or 0xF8; Ram/Port5 Control Reg, R/W
hd63701y0.PORT5 = 0x00; // port 5, R/W
hd63701y0.P6DDR = 0x00; // port 6 ddr, W
hd63701y0.PORT6 = 0x00; // port 6, R/W
hd63701y0.PORT7 = 0x00; // port 7, R/W
hd63701y0.OCR2H = 0xFF;
hd63701y0.OCR2L = 0xFF;
hd63701y0.TCSR3 = 0x20;
hd63701y0.TCONR = 0xFF;
hd63701y0.T2CNT = 0x00;
hd63701y0.TRCSR2 = 0x28;
hd63701y0.TSTREG = 0x00;
hd63701y0.P5DDR = 0x00; // port 5 ddr, W
hd63701y0.P6CSR = 0x00;
}

static READ8_HANDLER( main_hd63701_internal_registers_r )
{
	UINT8 data = 0;
	logerror("main hd637B01Y0: %04x: read from 0x%02X: ", cpu_get_pc(space->cpu), offset);
	switch(offset)
	{
		case 0x00: // Port 1 DDR
		case 0x01: // Port 2 DDR
		case 0x04: // Port 3 DDR
		case 0x05: // Port 4 DDR
		case 0x13: // TxD Register
		case 0x16: // Port 6 DDR
		case 0x1C: // Time Constant Register
		case 0x20: // Port 5 DDR
			logerror("a write only register! returning 0\n");
			data = 0;
			break;
		case 0x02: // Port 1
			logerror("Port 1\n");
			data = hd63701y0.PORT1;
			break;
		case 0x03: // Port 2
			logerror("Port 2\n");
			data = hd63701y0.PORT1;
			break;
		case 0x06: // Port 3
			logerror("Port 3\n");
			data = hd63701y0.PORT3;
			break;
		case 0x07: // Port 4
			logerror("Port 4\n");
			data = hd63701y0.PORT4;
			break;
		case 0x08: // Timer Control/Status Register 1
			logerror("Timer Control/Status Register 1\n");
			data = hd63701y0.TCSR1;
			break;
		case 0x09: // Free Running Counter (MSB)
			logerror("Free Running Counter (MSB)\n");
			data = hd63701y0.FRCH;
			break;
		case 0x0A: // Free Running Counter (LSB)
			logerror("Free Running Counter (LSB)\n");
			data = hd63701y0.FRCL;
			break;
		// B C (D E)
		case 0x0F: // Timer Control/Status Register 2
			logerror("Timer Control/Status Register 2\n");
			data = hd63701y0.TCSR2;
			break;
		// 10 11 (12)

		case 0x14: // RAM/Port 5 Control Register
			logerror("RAM/Port 5 Control Register\n");
			data = hd63701y0.RP5CR;
			break;
		case 0x15: // Port 5
			logerror("Port 5\n");
			data = hd63701y0.PORT5;
			break;
		case 0x17: // Port 6
			logerror("Port 6\n");
			data = hd63701y0.PORT6;
			break;
		case 0x18: // Port 7
			logerror("Port 7\n");
			data = hd63701y0.PORT7;
			break;
		// 19 1a 1b 1c 1d 1e 1f
		case 0x21: // Port 6 Control/Status Register
			logerror("Port 6 Control/Status Register\n");
			data = hd63701y0.P6CSR;
			break;
		default:
			logerror("\n");
			data = 0;
			break;
	}
	logerror("returning %02X\n", data);
	return data;
}


static WRITE8_HANDLER( main_hd63701_internal_registers_w )
{
	logerror("main hd637B01Y0: %04x: write to 0x%02X: ", cpu_get_pc(space->cpu), offset);
	switch(offset)
	{
		case 0x00: // Port 1 DDR
			logerror("Port 1 DDR of %02X\n", data);
			hd63701y0.P1DDR = data;
			rvoicepc.port1 = (hd63701y0.PORT1 & hd63701y0.P1DDR);
			break;
		case 0x01: // Port 2 DDR
			logerror("Port 2 DDR of %02X\n", data);
			hd63701y0.P2DDR = data;
			rvoicepc.port2 = (hd63701y0.PORT2 & hd63701y0.P2DDR);
			break;
		case 0x02: // Port 1
			logerror("Port 1 of %02X\n", data);
			hd63701y0.PORT1 = data;
			rvoicepc.port1 = (hd63701y0.PORT1 & hd63701y0.P1DDR);
			break;
		case 0x03: // Port 2
			logerror("Port 2 of %02X\n", data);
			hd63701y0.PORT2 = data;
			rvoicepc.port2 = (hd63701y0.PORT2 & hd63701y0.P2DDR);
			break;
		case 0x04: // Port 3 DDR
			logerror("Port 3 DDR of %02X\n", data);
			hd63701y0.P3DDR = data;
			rvoicepc.port3 = (hd63701y0.PORT3 & hd63701y0.P3DDR);
			break;
		case 0x05: // Port 4 DDR
			logerror("Port 4 DDR of %02X\n", data);
			hd63701y0.P4DDR = data;
			rvoicepc.port4 = (hd63701y0.PORT4 & hd63701y0.P4DDR);
			break;
		case 0x06: // Port 3
			logerror("Port 3 of %02X\n", data);
			hd63701y0.PORT3 = data;
			rvoicepc.port3 = (hd63701y0.PORT3 & hd63701y0.P3DDR);
			break;
		case 0x07: // Port 4
			logerror("Port 4 of %02X\n", data);
			hd63701y0.PORT4 = data;
			rvoicepc.port4 = (hd63701y0.PORT4 & hd63701y0.P4DDR);
			break;
		case 0x08: // Timer Control/Status Register 1
			logerror("Timer Control/Status Register 1 of %02X\n", data);
			hd63701y0.TCSR1 = data;
			break;
		case 0x09: // Free Running Counter (MSB)
			logerror("Free Running Counter (MSB) of %02X\n", data);
			hd63701y0.FRCH = data;
			break;
		case 0x0A: // Free Running Counter (LSB)
			logerror("Free Running Counter (LSB) of %02X\n", data);
			hd63701y0.FRCL = data;
			break;
		// B C (D E)
		case 0x0F: // Timer Control/Status Register 2
			logerror("Timer Control/Status Register 2 of %02X\n", data);
			hd63701y0.TCSR2 = data;
			break;
		// 10 11 (12)
		case 0x13: // TxD Register
			logerror("TxD Register of %02X\n", data);
			hd63701y0.TDR = data;
			break;
		case 0x14: // RAM/Port 5 Control Register
			logerror("RAM/Port 5 Control Register of %02X\n", data);
			hd63701y0.RP5CR = data;
			break;
		case 0x15: // Port 5
			logerror("Port 5 of %02X\n", data);
			hd63701y0.PORT5 = data;
			rvoicepc.port5 = (hd63701y0.PORT5 & hd63701y0.P5DDR);
			break;
		case 0x16: // Port 6 DDR
			logerror("Port 6 DDR of %02X\n", data);
			hd63701y0.P6DDR = data;
			rvoicepc.port6 = (hd63701y0.PORT6 & hd63701y0.P6DDR);
			break;
		case 0x17: // Port 6
			logerror("Port 6 of %02X\n", data);
			hd63701y0.PORT6 = data;
			rvoicepc.port6 = (hd63701y0.PORT6 & hd63701y0.P6DDR);
			break;
		case 0x18: // Port 7
			logerror("Port 7 of %02X\n", data);
			hd63701y0.PORT7 = data;
			rvoicepc.port7 = data;
			break;
		// 19 1a 1b 1c 1d 1e 1f
		case 0x20: // Port 5 DDR
			logerror("Port 5 DDR of %02X\n", data);
			hd63701y0.P5DDR = data;
			rvoicepc.port5 = (hd63701y0.PORT5 & hd63701y0.P5DDR);
			break;
		case 0x21: // Port 6 Control/Status Register
			logerror("Port 6 Control/Status Register of %02X\n", data);
			hd63701y0.P6CSR = data;
			break;
		default:
			logerror("with data of %02X\n", data);
			break;
	}
}


/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(hd63701_main_mem, ADDRESS_SPACE_PROGRAM, 8)
    ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0027) AM_READWRITE(main_hd63701_internal_registers_r, main_hd63701_internal_registers_w) // INTERNAL REGS
	AM_RANGE(0x0040, 0x013f) AM_RAM // INTERNAL RAM (overlaps acia)
	AM_RANGE(0x0060, 0x007f) AM_DEVREADWRITE( "acia65c51", acia_6551_r, acia_6551_w) // ACIA 65C51
    AM_RANGE(0x2000, 0x7fff) AM_RAM // EXTERNAL SRAM
	AM_RANGE(0x8000, 0xffff) AM_ROM // 27512 EPROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(hd63701_main_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/
INPUT_PORTS_START( rvoicepc )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static WRITE8_DEVICE_HANDLER( null_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( dectalk_terminal_intf )
{
	DEVCB_HANDLER(null_kbd_put)
};

static MACHINE_DRIVER_START(rvoicepc)
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", HD63701, XTAL_7_3728MHz)
    MDRV_CPU_PROGRAM_MAP(hd63701_main_mem)
    MDRV_CPU_IO_MAP(hd63701_main_io)

    //MDRV_CPU_ADD("playercpu", HD63701, XTAL_7_3728MHz) // not dumped yet
    //MDRV_CPU_PROGRAM_MAP(hd63701_slave_mem)
    //MDRV_CPU_IO_MAP(hd63701_slave_io)
    MDRV_QUANTUM_TIME(HZ(60))
    MDRV_MACHINE_RESET(rvoicepc)

    MDRV_ACIA6551_ADD("acia65c51")

    /* video hardware */
    //MDRV_DEFAULT_LAYOUT(layout_dectalk) // hack to avoid screenless system crash

    /* sound hardware */
	MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,dectalk_terminal_intf)

MACHINE_DRIVER_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(rvoicepc)

    ROM_REGION(0x10000, "maincpu", 0)
    ROM_LOAD("rv_pc.bin", 0x08000, 0x08000, CRC(4001CD5F) SHA1(D973C6E19E493EEDD4F7216BC530DDB0B6C4921E))
    ROM_CONTINUE(0x8000, 0x8000) // first half of 27c512 rom is blank due to stupid address decoder circuit

ROM_END



/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                            FLAGS */
COMP( 1988?, rvoicepc,   0,          0,      rvoicepc,   rvoicepc, rvoicepc,      "Adaptive Communication Systems",        "Realvoice PC", GAME_NOT_WORKING | GAME_NO_SOUND)

