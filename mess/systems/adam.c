/***************************************************************************

  adam.c

  Driver file to handle emulation of the ColecoAdam.

  Marat Fayzullin (ColEm source)
  Marcel de Kogel (AdamEm source)
  Mike Balfour
  Ben Bruscella
  Sean Young
  Jose Moya


The Coleco ADAM is a Z80-based micro with all peripheral devices
attached to an internal serial serial bus (ADAMnet) managed by 6801
microcontrollers (processor, internal RAM, internal ROM, serial port).Each
device had its own 6801, and the ADAMnet was managed by a "master" 6801 on
the ADAM motherboard.  Each device was allotted a block of 21 bytes in Z80
address space; device control was accomplished by poking function codes into
the first byte of each device control block (hereafter DCB) after setup of
other DCB locations with such things as:  buffer address in Z80 space, # of
bytes to transfer, block # to access if it was a block device like a tape
or disk drive, etc.  The master 6801 would interpret this data, and pass
along internal ADAMnet requests to the desired peripheral, which would then
send/receive its data and return the status of the operation.  The status
codes were left in the same byte of the DCB as the function request, and
certain bits of the status byte would reflect done/working on it/error/
not present, and error codes were left in another DCB byte for things like
CRC error, write protected disk, missing block, etc.

ADAM's ROM operating system, EOS (Elementary OS), was constructed
similar to CP/M in that it provided both a filesystem (like BDOS) and raw
device interface (BIOS).  At the file level, sequential files could be
created, opened, read, written, appended, closed, and deleted.  Forward-
only random access was implemented (you could not move the R/W pointer
backward, except clear to the beginning!), and all files had to be
contiguous on disk/tape.  Directories could be initialized or searched
for a matching filename (no wildcards allowed).  At the device level,
individual devices could be read/written by block (for disks/tapes) or
character-by-character (for printer, keyboard, and a prototype serial
board which was never released).  Devices could be checked for their
ADAMnet status, and reset if necessary.  There was no function provided
to do low-level formatting of disks/tapes.

 At system startup, the EOS was loaded from ROM into the highest
8K of RAM, a function call made to initialize the ADAMnet, and then
any disks or tapes were checked for a boot medium; if found, block 0 of
the medium was loaded in, and a jump made to the start of the boot code.
The boot block would take over loading in the rest of the program.  If no
boot media were found, a jump would be made to a ROM word processor (called
SmartWriter).

 Coleco designed the ADAMnet to have up to 15 devices attached.
Before they went bankrupt, Coleco had released a 64K memory expander and
a 300-baud internal modem, but surprisingly neither of these was an
ADAMnet device.  Disassembly of the RAMdisk drivers in ADAM CP/M 2.2, and
of the ADAMlink terminal program revealed that these were simple port I/O
devices, banks of XRAM being accessed by a special memory switch port not
documented as part of the EOS.  The modem did not even use the interrupt
capabilities of the Z80--it was simply polled.  A combination serial/
parallel interface, each port of which *was* an ADAMnet device, reached the
prototype stage, as did a 5MB hard disk, but neither was ever released to
the public.  (One prototype serial/parallel board is still in existence,
but the microcontroller ROMs have not yet been succcessfully read.)  So
when Coleco finally bailed out of the computer business, a maximum ADAM
system consisted of a daisy wheel printer, a keyboard, 2 tape drives, and
2 disk drives (all ADAMnet devices), a 64K expander and a 300-baud modem
(which were not ADAMnet devices).

 Third-party vendors reverse-engineered the modem (which had a
2651 UART at its heart) and made a popular serial interface board.  It was
not an ADAMnet device, however, because nobody knew how to make a new ADAMnet
device (no design specs were ever released), and the 6801 microcontrollers
had unreadable mask ROMs.  Disk drives, however, were easily upgraded from
160K to as high as 1MB because, for some unknown reason, the disk controller
boards used a separate microprocessor and *socketed* EPROM (which was
promptly disassembled and reworked).  Hard drives were cobbled together from
a Kaypro-designed board and accessed as standard I/O port devices.  A parallel
interface card was similarly set up at its own I/O port.

  Devices (15 max):
    Device 0 = Master 6801 ADAMnet controller (uses the adam_pcb as DCB)
    Device 1 = Keyboard
    Device 2 = ADAM printer
    Device 3 = Copywriter (projected)
    Device 4 = Disk drive 1
    Device 5 = Disk drive 2
    Device 6 = Disk drive 3 (third party)
    Device 7 = Disk drive 4 (third party)
    Device 8 = Tape drive 1
    Device 9 = Tape drive 3 (projected)
    Device 10 = Unused
    Device 11 = Non-ADAMlink modem
    Device 12 = Hi-resolution monitor
    Device 13 = ADAM parallel interface (never released)
    Device 14 = ADAM serial interface (never released)
    Device 15 = Gateway
    Device 24 = Tape drive 2 (share DCB with Tape1)
    Device 25 = Tape drive 4 (projected, may have share DCB with Tape3)
    Device 26 = Expansion RAM disk drive (third party ID, not used by Coleco)
    
  Terminology:
    EOS = Elementary Operating System
    DCB = Device Control Block Table (21bytes each DCB, DCB+16=dev#, DCB+0=Status Byte) (0xFD7C)

           0     Status byte
         1-2     Buffer start address (lobyte, hibyte)
         3-4     Buffer length (lobyte, hibyte)
         5-8     Block number accessed (loword, hiword in lobyte, hibyte format)
           9     High nibble of device number
        10-15    Always zero (unknown purpose)
          16     Low nibble of device number
        17-18    Maximum block length (lobyte, hibyte)
          19     Device type (0 for block device, 1 for character device)
          20     Node type

        - Writing to byte0 requests the following operations:
            1     Return current status
            2     Soft reset
            3     Write
            4     Read


    FCB = File Control Block Table (32bytes, 2 max each application) (0xFCB0)
    OCB = Overlay Control Block Table
    adam_pcb = Processor Control Block Table, 4bytes (adam_pcb+3 = Number of valid DCBs) (0xFEC0 relocatable), current adam_pcb=[0xFD70]
            adam_pcb+0 = Status, 0=Request Status of Z80 -> must return 0x81..0x82 to sync Master 6801 clk with Z80 clk
            adam_pcb+1,adam_pcb+2 = address of adam_pcb start
            adam_pcb+3 = device #
            
            - Writing to byte0:
                1   Synchronize the Z80 clock (should return 0x81)
                2   Synchronize the Master 6801 clock (should return 0x82)
                3   Relocate adam_pcb

            - Status values:
                0x80 -> Success
                0x81 -> Z80 clock in sync
                0x82 -> Master 6801 clock in sync
                0x83 -> adam_pcb relocated
                0x9B -> Time Out
            
    DEV_ID = Device id
  
  
    The ColecoAdam I/O map is contolled by the MIOC (Memory Input Output Controller):

            20-3F (W) = Adamnet Writes
            20-3F (R) = Adamnet Reads

            42-42 (W) = Expansion RAM page selection, only useful if expansion greater than 64k
            
            40-40 (W) = Printer Data Out
            40-40 (R) = Printer (Returns 0x41)
            
            5E-5E (RW)= Modem Data I/O
            5F-5F (RW)= Modem Data Control Status
            
            60-7F (W) = Set Memory configuration
            60-7F (R) = Read Memory configuration

            80-9F (W) = Set both controllers to keypad mode
            80-9F (R) = Not Connected

            A0-BF (W) = Video Chip (TMS9928A), A0=0 -> Write Register 0 , A0=1 -> Write Register 1
            A0-BF (R) = Video Chip (TMS9928A), A0=0 -> Read Register 0 , A0=1 -> Read Register 1

            C0-DF (W) = Set both controllers to joystick mode 
            C0-DF (R) = Not Connected

            E0-FF (W) = Sound Chip (SN76496)
            E0-FF (R) = Read Controller data, A1=0 -> read controller 1, A1=1 -> read controller 2

TO DO:
    - Improve Keyboard "Simulation" (No ROM dumps available for the keyboard MC6801 AdamNet MCU)
    - Add Tape "Simulation" (No ROM dumps available for the Tape MC6801 AdamNet MCU)
    - Add Disc "Simulation" (No ROM dumps available for the Disc MC6801 AdamNet MCU)
    - Add Full ColecoAdam emulation (every MC6801) if ROM dumps become available.
        Do you have the ROM dumps?... let us know.

***************************************************************************/


#include "driver.h"
#include "sound/sn76496.h"
#include "vidhrdw/tms9928a.h"
#include "includes/adam.h"
#include "cpu/m6800/m6800.h"
#include "devices/cartslot.h"
#include "devices/mflopimg.h"
#include "formats/adam_dsk.h"

static MEMORY_READ_START( adam_readmem )
	{ 0x00000, 0x01fff, MRA_BANK1 },
	{ 0x02000, 0x03fff, MRA_BANK2 },
	{ 0x04000, 0x05fff, MRA_BANK3 },
	{ 0x06000, 0x07fff, MRA_BANK4 },
	{ 0x08000, 0x0ffff, MRA_BANK5 },
MEMORY_END

static MEMORY_WRITE_START( adam_writemem )
	{ 0x00000, 0x01fff, MWA_BANK6 },
	{ 0x02000, 0x03fff, MWA_BANK7 },
	{ 0x04000, 0x05fff, MWA_BANK8 },
	{ 0x06000, 0x07fff, MWA_BANK9 },
	{ 0x08000, 0x0ffff, common_writes_w},
MEMORY_END

static PORT_READ_START ( adam_readport )
    { 0x20, 0x3F, adamnet_r },
    { 0x60, 0x7F, adam_memory_map_controller_r },
    { 0xA0, 0xBF, adam_video_r },
    { 0xE0, 0xFF, adam_paddle_r },
PORT_END

static PORT_WRITE_START ( adam_writeport )
    { 0x20, 0x3F, adamnet_w },
    { 0x60, 0x7F, adam_memory_map_controller_w },
    { 0x80, 0x9F, adam_paddle_toggle_off },
    { 0xA0, 0xBF, adam_video_w },
    { 0xC0, 0xDF, adam_paddle_toggle_on },
    { 0xE0, 0xFF, SN76496_0_w },
PORT_END

/*
I do now know the real memory map of the Master 6801...
and the 6801 ASM code is a replacement coded for this driver.
*/

static MEMORY_READ_START( master6801_readmem )
	{ 0x0000, 0x001f, m6803_internal_registers_r },
	{ 0x0020, 0x007f, MRA_NOP }, /* Unused */
	{ 0x0080, 0x00ff, MRA_RAM }, /* 6801 internal RAM */
	{ 0x0100, 0x3fff, MRA_ROM }, /* Replacement Master ROM code */
	{ 0x4000, 0xffff, master6801_ram_r }, /* RAM Memory shared with Z80 not banked*/
MEMORY_END

static MEMORY_WRITE_START( master6801_writemem )
	{ 0x0000, 0x001f, m6803_internal_registers_w },
	{ 0x0020, 0x007f, MWA_NOP }, /* Unused */
	{ 0x0080, 0x00ff, MWA_NOP }, /* 6801 internal RAM */
	{ 0x0100, 0x3fff, MWA_NOP }, /* Unused */
    { 0x4000, 0xffff, master6801_ram_w }, /* RAM Memory shared with Z80 not banked*/
MEMORY_END

INPUT_PORTS_START( adam )
    PORT_START  /* IN0 */

    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "0 (pad 1)", KEYCODE_0, IP_JOY_DEFAULT)
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "1 (pad 1)", KEYCODE_1, IP_JOY_DEFAULT)
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "2 (pad 1)", KEYCODE_2, IP_JOY_DEFAULT)
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, "3 (pad 1)", KEYCODE_3, IP_JOY_DEFAULT)
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_TILT, "4 (pad 1)", KEYCODE_4, IP_JOY_DEFAULT)
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_TILT, "5 (pad 1)", KEYCODE_5, IP_JOY_DEFAULT)
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_TILT, "6 (pad 1)", KEYCODE_6, IP_JOY_DEFAULT)
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_TILT, "7 (pad 1)", KEYCODE_7, IP_JOY_DEFAULT)

    PORT_START  /* IN1 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "8 (pad 1)", KEYCODE_8, IP_JOY_DEFAULT)
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "9 (pad 1)", KEYCODE_9, IP_JOY_DEFAULT)
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "# (pad 1)", KEYCODE_MINUS, IP_JOY_DEFAULT)
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, ". (pad 1)", KEYCODE_EQUALS, IP_JOY_DEFAULT)
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN2 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN3 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "0 (pad 2)", KEYCODE_0_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "1 (pad 2)", KEYCODE_1_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "2 (pad 2)", KEYCODE_2_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, "3 (pad 2)", KEYCODE_3_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_TILT, "4 (pad 2)", KEYCODE_4_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_TILT, "5 (pad 2)", KEYCODE_5_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_TILT, "6 (pad 2)", KEYCODE_6_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_TILT, "7 (pad 2)", KEYCODE_7_PAD, IP_JOY_DEFAULT )

    PORT_START  /* IN4 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "8 (pad 2)", KEYCODE_8_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "9 (pad 2)", KEYCODE_9_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "# (pad 2)", KEYCODE_MINUS_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, ". (pad 2)", KEYCODE_PLUS_PAD, IP_JOY_DEFAULT )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN5 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
    PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
    PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
    PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START /* IN6 */
    PORT_DIPNAME(0x0F, 0x00, "Controllers")
    PORT_DIPSETTING(0x00, "None" )
    PORT_DIPSETTING(0x01, "Driving Controller" )
    PORT_DIPSETTING(0x02, "Roller Controller" )
    PORT_DIPSETTING(0x04, "Super Action Controllers" )
    PORT_DIPSETTING(0x08, "Standard Controllers" )

    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3, "SAC Blue Button P1", KEYCODE_Z, IP_JOY_DEFAULT )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4, "SAC Purple Button P1", KEYCODE_X, IP_JOY_DEFAULT )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "SAC Blue Button P2", KEYCODE_Q, IP_JOY_DEFAULT )
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2, "SAC Purple Button P2", KEYCODE_W, IP_JOY_DEFAULT )        

    PORT_START	/* IN7, to emulate Extra Controls (Driving Controller, SAC P1 slider, Roller Controller X Axis) */
    PORT_ANALOGX( 0x0f, 0x00, IPT_TRACKBALL_X | IPF_CENTER, 20, 10, 0, 0, KEYCODE_L, KEYCODE_J, IP_JOY_NONE, IP_JOY_NONE )

    PORT_START	/* IN8, to emulate Extra Controls (SAC P2 slider, Roller Controller Y Axis) */
    PORT_ANALOGX( 0x0f, 0x00, IPT_TRACKBALL_Y | IPF_CENTER | IPF_PLAYER2, 20, 10, 0, 0, KEYCODE_I, KEYCODE_K, IP_JOY_NONE, IP_JOY_NONE )


/* Keyboard with 75 Keys */
	
	PORT_START /* IN9 0*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "WP/ESCAPE", KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_START /* IN10 1*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)

	PORT_START /* IN11 2*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)

	PORT_START /* IN12 3*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "] }", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\ |", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "^ ~", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "- `", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :", KEYCODE_COLON, IP_JOY_NONE)

	PORT_START /* IN13 4*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 _", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 &", KEYCODE_7, IP_JOY_NONE)

	PORT_START /* IN14 5*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 *", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "' \"", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "+ =", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ ?", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "[ {", KEYCODE_OPENBRACE, IP_JOY_NONE)
	
	PORT_START /* IN15 6*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SoftKey I", KEYCODE_F1, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SoftKey II", KEYCODE_F2, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SoftKey III", KEYCODE_F3, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "SoftKey IV", KEYCODE_F4, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SoftKey V", KEYCODE_F5, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SoftKey VI", KEYCODE_F6, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)

	PORT_START /* IN16 7*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "WILD CARD", KEYCODE_F7, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "UNDO", KEYCODE_F8, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "MOVE", KEYCODE_HOME, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "STORE", KEYCODE_END, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "INSERT", KEYCODE_INSERT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "PRINT", KEYCODE_PRTSCR, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_DEL, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "DELETE", KEYCODE_BACKSPACE, IP_JOY_NONE)

	PORT_START /* IN17 8*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "HOME", KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP ARROW", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN ARROW", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT ARROW", KEYCODE_LEFT, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT ARROW", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "LSHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "RSHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	
	PORT_START /* IN18 9*/
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "LCONTROL", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "RCONTROL", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "SHIFT LOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BIT (0xF8, IP_ACTIVE_LOW, IPT_UNKNOWN )
	
INPUT_PORTS_END

static struct SN76496interface sn76496_interface =
{
    1,  		/* 1 chip 		*/
    {3579545},  /* 3.579545 MHz */
    { 100 }
};

/***************************************************************************

  The interrupts come from the vdp. The vdp (tms9928a) interrupt can go up
  and down; the Coleco only uses nmi interrupts (which is just a pulse). They
  are edge-triggered: as soon as the vdp interrupt line goes up, an interrupt
  is generated. Nothing happens when the line stays up or goes down.

  To emulate this correctly, we set a callback in the tms9928a (they
  can occur mid-frame). At every frame we call the TMS9928A_interrupt
  because the vdp needs to know when the end-of-frame occurs, but we don't
  return an interrupt.

***************************************************************************/

static INTERRUPT_GEN( adam_interrupt )
{
    TMS9928A_interrupt();
    exploreKeyboard();
}

static void adam_vdp_interrupt (int state)
{
	static int last_state = 0;

    /* only if it goes up */
	if (state && !last_state)
	    {
	        cpu_set_nmi_line(0, PULSE_LINE);
	    }
	last_state = state;
}

void adam_paddle_callback (int param)
{
	int port7 = input_port_7_r (0);
	int port8 = input_port_8_r (0);

	if (port7 == 0)
		adam_joy_stat[0] = 0;
	else if (port7 & 0x08)
		adam_joy_stat[0] = -1;
	else
		adam_joy_stat[0] = 1;

	if (port8 == 0)
		adam_joy_stat[1] = 0;
	else if (port8 & 0x08)
		adam_joy_stat[1] = -1;
	else
		adam_joy_stat[1] = 1;

	if (adam_joy_stat[0] || adam_joy_stat[1])
		cpu_set_irq_line (0, 0, HOLD_LINE);
}

void set_memory_banks(void)
{
/*
Lineal virtual memory map:

0x00000, 0x07fff -> Lower Internal RAM
0x08000, 0x0ffff -> Upper Internal RAM
0x10000, 0x17fff -> Lower Expansion RAM
0x18000, 0x1ffff -> Upper Expansion RAM
0x20000, 0x27fff -> SmartWriter ROM
0x28000, 0x2ffff -> Cartridge
0x30000, 0x31fff -> OS7 ROM (ColecoVision ROM)
0x32000, 0x39fff -> EOS ROM
0x3A000, 0x41fff -> Used to Write Protect ROMs
*/
	UINT8 *BankBase;
	BankBase = &memory_region(REGION_CPU1)[0x00000];

	switch (adam_lower_memory)
	{
		case 0: /* SmartWriter ROM */
			if (adam_net_data & 0x02)
			{
				/* Read */
				cpu_setbank(1, BankBase+0x32000); /* No data here */
				cpu_setbank(2, BankBase+0x34000); /* No data here */
				cpu_setbank(3, BankBase+0x36000); /* No data here */
				cpu_setbank(4, BankBase+0x38000); /* EOS ROM */

				/* Write */
				cpu_setbank(6, BankBase+0x3A000); /* Write protecting ROM */
				cpu_setbank(7, BankBase+0x3A000); /* Write protecting ROM */
				cpu_setbank(8, BankBase+0x3A000); /* Write protecting ROM */
				cpu_setbank(9, BankBase+0x3A000); /* Write protecting ROM */
			}
			else
			{
				/* Read */
				cpu_setbank(1, BankBase+0x20000); /* SmartWriter ROM */
				cpu_setbank(2, BankBase+0x22000);
				cpu_setbank(3, BankBase+0x24000);
				cpu_setbank(4, BankBase+0x26000);
	            
				/* Write */
				cpu_setbank(6, BankBase+0x3A000); /* Write protecting ROM */
				cpu_setbank(7, BankBase+0x3A000); /* Write protecting ROM */
				cpu_setbank(8, BankBase+0x3A000); /* Write protecting ROM */
				cpu_setbank(9, BankBase+0x3A000); /* Write protecting ROM */
			}
			break;
		case 1: /* Internal RAM */
			/* Read */
			cpu_setbank(1, BankBase);
			cpu_setbank(2, BankBase+0x02000);
			cpu_setbank(3, BankBase+0x04000);
			cpu_setbank(4, BankBase+0x06000);
	        
			/* Write */
			cpu_setbank(6, BankBase);
			cpu_setbank(7, BankBase+0x02000);
			cpu_setbank(8, BankBase+0x04000);
			cpu_setbank(9, BankBase+0x06000);
			break;
		case 2: /* RAM Expansion */
			/* Read */
			cpu_setbank(1, BankBase+0x10000);
			cpu_setbank(2, BankBase+0x12000);
			cpu_setbank(3, BankBase+0x14000);
			cpu_setbank(4, BankBase+0x16000);

			/* Write */
			cpu_setbank(6, BankBase+0x10000);
			cpu_setbank(7, BankBase+0x12000);
			cpu_setbank(8, BankBase+0x14000);
			cpu_setbank(9, BankBase+0x16000);
			break;
		case 3: /* OS7 ROM (8k) + Internal RAM (24k) */
			/* Read */
			cpu_setbank(1, BankBase+0x30000);
			cpu_setbank(2, BankBase+0x02000);
			cpu_setbank(3, BankBase+0x04000);
			cpu_setbank(4, BankBase+0x06000);
	        
			/* Write */
			cpu_setbank(6, BankBase+0x3A000); /* Write protecting ROM */
			cpu_setbank(7, BankBase+0x02000);
			cpu_setbank(8, BankBase+0x04000);
			cpu_setbank(9, BankBase+0x06000);
	}

	switch (adam_upper_memory)
	{
		case 0: /* Internal RAM */
			/* Read */
			cpu_setbank(5, BankBase+0x08000);
			/*cpu_setbank(10, BankBase+0x08000);*/
			break;
		case 1: /* ROM Expansion */
			break;
		case 2: /* RAM Expansion */
			/* Read */
			cpu_setbank(5, BankBase+0x18000);
			/*cpu_setbank(10, BankBase+0x18000);*/
			break;
		case 3: /* Cartridge ROM */
			/* Read */
			cpu_setbank(5, BankBase+0x28000);
			/*cpu_setbank(10, BankBase+0x3A000); *//* Write protecting ROM */
			break;
	}
}

void resetPCB(void)
{
    int i;
    memory_region(REGION_CPU1)[adam_pcb] = 0x01;

	for (i = 0; i < 15; i++)
		memory_region(REGION_CPU1)[(adam_pcb+4+i*21)&0xFFFF]=i+1;
}

static MACHINE_INIT( adam )
{
	if (image_exists(image_from_devtype_and_index(IO_CARTSLOT, 0)))
	{
		/* ColecoVision Mode Reset (Cartridge Mounted) */
		adam_lower_memory = 3; /* OS7 + 24k RAM */
		adam_upper_memory = 3; /* Cartridge ROM */
	}
	else
	{
		/* Adam Mode Reset */
		adam_lower_memory = 0; /* SmartWriter ROM/EOS */
		adam_upper_memory = 0; /* Internal RAM */
	}

	adam_net_data = 0;
	set_memory_banks();
	adam_pcb=0xFEC0;
	clear_keyboard_buffer();

	memset(&memory_region(REGION_CPU1)[0x0000], 0xFF, 0x20000); /* Initializing RAM */
	timer_pulse(TIME_IN_MSEC(20), 0, adam_paddle_callback);
} 

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	adam_vdp_interrupt
};

static MACHINE_DRIVER_START( adam )
	/* Machine hardware */
	MDRV_CPU_ADD_TAG("Main", Z80, 3579545)       /* 3.579545 Mhz */
	MDRV_CPU_MEMORY(adam_readmem,adam_writemem)
	MDRV_CPU_PORTS(adam_readport,adam_writeport)

    /* Master M6801 AdamNet controller */
	//MDRV_CPU_ADD(M6800, 4000000)       /* 4.0 Mhz */
	//MDRV_CPU_MEMORY(master6801_readmem,master6801_writemem)

	MDRV_CPU_VBLANK_INT(adam_interrupt,1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( adam )

    /* video hardware */
	MDRV_TMS9928A( &tms9928a_interface )

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
os7.rom CRC(3AA93EF3)
eos.rom CRC(05A37A34)
wp.rom  CRC(58D86A2A)
*/
/*
Total Memory Size: 64k Internal RAM, 64k Expansion RAM, 32k SmartWriter ROM, 8k OS7, 32k Cartridge, 32k Extra

Lineal virtual memory map:

0x00000, 0x07fff -> Lower Internal RAM
0x08000, 0x0ffff -> Upper Internal RAM
0x10000, 0x17fff -> Lower Expansion RAM
0x18000, 0x1ffff -> Upper Expansion RAM
0x20000, 0x27fff -> SmartWriter ROM
0x28000, 0x2ffff -> Cartridge
0x30000, 0x31fff -> OS7 ROM (ColecoVision ROM)
0x32000, 0x39fff -> EOS ROM
0x3A000, 0x41fff -> Used to Write Protect ROMs
0x42000, 0x47fff -> Low unused EOS ROM
*/
ROM_START (adam)
    ROM_REGION( 0x42000, REGION_CPU1, 0)
    ROM_LOAD ("wp.rom", 0x20000, 0x8000, CRC(58d86a2a))
    ROM_LOAD ("os7.rom", 0x30000, 0x2000, CRC(3aa93ef3))
    ROM_LOAD ("eos.rom", 0x38000, 0x2000, CRC(05a37a34))
    //ROM_REGION( 0x10000, REGION_CPU2, 0)
    //ROM_LOAD ("master68.rom", 0x0100, 0x0E4, CRC(619a47b8)) /* Replacement 6801 Master ROM */
ROM_END

SYSTEM_CONFIG_START(adam)
	CONFIG_DEVICE_CARTSLOT_OPT( 1, "rom\0col\0bin\0", NULL, NULL, device_load_adam_cart, NULL, adam_cart_verify, NULL)
	CONFIG_DEVICE_FLOPPY( 4, adam )
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME    PARENT	COMPAT	MACHINE INPUT   INIT	CONFIG	COMPANY FULLNAME */
COMP( 1982, adam,   0,		coleco,	adam,   adam,   0,		adam,	"Adam", "ColecoAdam" )

