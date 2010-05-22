/***************************************************************************

    Atari Jaguar

    Nathan Woods
    based on MAME cojag driver by Aaron Giles


****************************************************************************

    Memory map (TBA)

    ========================================================================
    MAIN CPU
    ========================================================================

    ------------------------------------------------------------
    000000-3FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 0
    400000-7FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 1
    F00000-F000FF   R/W   xxxxxxxx xxxxxxxx   Tom Internal Registers
    F00400-F005FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table A
    F00600-F007FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table B
    F00800-F00D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer A
    F01000-F0159F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer B
    F01800-F01D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer currently selected
    F02000-F021FF   R/W   xxxxxxxx xxxxxxxx   GPU control registers
    F02200-F022FF   R/W   xxxxxxxx xxxxxxxx   Blitter registers
    F03000-F03FFF   R/W   xxxxxxxx xxxxxxxx   Local GPU RAM
    F08800-F08D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer A
    F09000-F0959F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer B
    F09800-F09D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer currently selected
    F0B000-F0BFFF   R/W   xxxxxxxx xxxxxxxx   32-bit access to local GPU RAM
    F10000-F13FFF   R/W   xxxxxxxx xxxxxxxx   Jerry
    F14000-F17FFF   R/W   xxxxxxxx xxxxxxxx   Joysticks and GPIO0-5
    F18000-F1AFFF   R/W   xxxxxxxx xxxxxxxx   Jerry DSP
    F1B000-F1CFFF   R/W   xxxxxxxx xxxxxxxx   Local DSP RAM
    F1D000-F1DFFF   R     xxxxxxxx xxxxxxxx   Wavetable ROM
    ------------------------------------------------------------

****************************************************************************

Protection Check

At power on, a checksum is performed on the cart to ensure it has been
certified by Atari. The actual checksum calculation is performed by the GPU,
the result being left in GPU RAM at address f03000. The GPU is instructed to
do the calculation when the bios sends a 1 to f02114 while it is in the
initialisation stage. The bios then loops, waiting for the GPU to finish the
calculation. When it does, it sets bit 15 of f02114 high. The bios then does
the compare of the checksum. The checksum algorithm is unknown, but the
final result must be 03d0dead. The bios checks for this particular result,
and if found, the cart is allowed to start. Otherwise, the background turns
red, and the console freezes.


Jaguar Logo

A real Jaguar will show the red Jaguar logo, the falling white Atari letters,
and the turning jaguar's head, accompanied by the sound of a flushing toilet.
The cart will then start. All Jaguar emulators (including this one) skip the
logo with the appropriate memory hack. The cart can also instruct the logo
be skipped by placing non-zero at location 800408. We do the same thing when
the cart is loaded (see the DEVICE_IMAGE_LOAD section below).


Start Address

The start address of a cart may be found at 800404. It is normally 802000.


***************************************************************************/


#include "emu.h"
#include "emuopts.h"
#include "cpu/m68000/m68000.h"
#include "cpu/mips/r3000.h"
#include "cpu/jaguar/jaguar.h"
#include "devices/cartslot.h"
#include "devices/snapquik.h"
#include "sound/dac.h"
#include "machine/eeprom.h"
#include "includes/jaguar.h"

/* 26.6MHz for Tom and Jerry, and half that for the CPU */
#define JAGUAR_CLOCK		26.6e6

static QUICKLOAD_LOAD( jaguar );
static DEVICE_IMAGE_LOAD( jaguar );

/*************************************
 *
 *  Global variables
 *
 *************************************/

UINT32 *jaguar_shared_ram;
UINT32 *jaguar_gpu_ram;
UINT32 *jaguar_gpu_clut;
UINT32 *jaguar_dsp_ram;
UINT32 *jaguar_wave_rom;
UINT8 cojag_is_r3000 = FALSE;



/*************************************
 *
 *  Local variables
 *
 *************************************/

static UINT32 joystick_data;
static UINT32 *rom_base;
static size_t rom_size;
static UINT32 *cart_base;
static size_t cart_size;
static UINT8 eeprom_bit_count;
static UINT8 protection_check = 0;	/* 0 = check hasn't started yet; 1= check in progress; 2 = check is finished. */


static IRQ_CALLBACK(jaguar_irq_callback)
{
	return (irqline == 6) ? 0x40 : -1;
}

/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( jaguar )
{
	cpu_set_irq_callback(devtag_get_device(machine, "maincpu"), jaguar_irq_callback);

	protection_check = 0;

	/* Set up pointers for Jaguar logo */
	memcpy(jaguar_shared_ram, rom_base, 0x10);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), REG_GENPC, rom_base[1]);

#if 0
	/* set up main CPU RAM/ROM banks */
	memory_set_bankptr(machine, 3, jaguar_gpu_ram);

	/* set up DSP RAM/ROM banks */
	memory_set_bankptr(machine, 10, jaguar_shared_ram);
	memory_set_bankptr(machine, 11, jaguar_gpu_clut);
	memory_set_bankptr(machine, 12, jaguar_gpu_ram);
	memory_set_bankptr(machine, 13, jaguar_dsp_ram);
	memory_set_bankptr(machine, 14, jaguar_shared_ram);
	memory_set_bankptr(machine, 15, cart_base);
	memory_set_bankptr(machine, 16, rom_base);
	memory_set_bankptr(machine, 17, jaguar_gpu_ram);
#endif

	/* clear any spinuntil stuff */
	jaguar_gpu_resume(machine);
	jaguar_dsp_resume(machine);

	/* halt the CPUs */
	jaguargpu_ctrl_w(devtag_get_device(machine, "gpu"), G_CTRL, 0, 0xffffffff);
	jaguardsp_ctrl_w(devtag_get_device(machine, "audiocpu"), D_CTRL, 0, 0xffffffff);

	joystick_data = 0xffffffff;
	eeprom_bit_count = 0;
}


/********************************************************************
*
*  EEPROM
*  ======
*
*   The EEPROM is accessed by a serial protocol using the registers
*   0xF14000 (read data), F14800 (increment clock, write data), F15000 (reset for next word)
*
********************************************************************/
static mame_file *jaguar_nvram_fopen( running_machine *machine, UINT32 openflags)
{
	running_device *image = devtag_get_device(machine, "cart");
	astring *fname;
	file_error filerr;
	mame_file *file;
	if (image_exists(image))
	{
		fname = astring_assemble_4( astring_alloc(), machine->gamedrv->name, PATH_SEPARATOR, image_basename_noext(image), ".nv");
		filerr = mame_fopen( SEARCHPATH_NVRAM, astring_c( fname), openflags, &file);
		astring_free( fname);
		return (filerr == FILERR_NONE) ? file : NULL;
	}
	else
		return NULL;
}

static void jaguar_nvram_load(running_machine *machine)
{
	mame_file *nvram_file = NULL;
	running_device *device;

	for (device = machine->devicelist.first(); device != NULL; device = device->next)
	{
		device_nvram_func nvram = (device_nvram_func)device->get_config_fct(DEVINFO_FCT_NVRAM);
		if (nvram != NULL)
		{
			if (nvram_file == NULL)
				nvram_file = jaguar_nvram_fopen(machine, OPEN_FLAG_READ);
			(*nvram)(device, nvram_file, 0);
		}
	}
	if (nvram_file != NULL)
		mame_fclose(nvram_file);
}


static void jaguar_nvram_save(running_machine *machine)
{
	mame_file *nvram_file = NULL;
	running_device *device;

	for (device = machine->devicelist.first(); device != NULL; device = device->next)
	{
		device_nvram_func nvram = (device_nvram_func)device->get_config_fct(DEVINFO_FCT_NVRAM);
		if (nvram != NULL)
		{
			if (nvram_file == NULL)
				nvram_file = jaguar_nvram_fopen(machine, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
			(*nvram)(device, nvram_file, 1);
		}
	}

	if (nvram_file != NULL)
		mame_fclose(nvram_file);
}

static NVRAM_HANDLER( jaguar )
{
	if (read_or_write)	{
		jaguar_nvram_save(machine);
	}
	else
	{
		if (file)
			jaguar_nvram_load(machine);
	}
}

static WRITE32_HANDLER( jaguar_eeprom_w )
{
	running_device *eeprom = devtag_get_device(space->machine, "eeprom");
	eeprom_bit_count++;
	if (eeprom_bit_count != 9)		/* kill extra bit at end of address */
	{
		eeprom_write_bit(eeprom,data >> 31);
		eeprom_set_clock_line(eeprom,PULSE_LINE);
	}
}

static READ32_HANDLER( jaguar_eeprom_clk )
{
	running_device *eeprom = devtag_get_device(space->machine, "eeprom");
	eeprom_set_clock_line(eeprom,PULSE_LINE);	/* get next bit when reading */
	return 0;
}

static READ32_HANDLER( jaguar_eeprom_cs )
{
	running_device *eeprom = devtag_get_device(space->machine, "eeprom");
	eeprom_set_cs_line(eeprom,ASSERT_LINE);	/* must do at end of an operation */
	eeprom_set_cs_line(eeprom,CLEAR_LINE);		/* enable chip for next operation */
	eeprom_write_bit(eeprom,1);			/* write a start bit */
	eeprom_set_clock_line(eeprom,PULSE_LINE);
	eeprom_bit_count = 0;
	return 0;
}


/*************************************
 *
 *  32-bit access to the GPU
 *
 *************************************/

static READ32_HANDLER( gpuctrl_r )
{
	UINT32 result = jaguargpu_ctrl_r(devtag_get_device(space->machine, "gpu"), offset);
	if (protection_check != 1) return result;

	protection_check++;
	jaguar_gpu_ram[0] = 0x3d0dead;
	return 0x80000000;
}


static WRITE32_HANDLER( gpuctrl_w )
{
	if ((!protection_check) && (offset == 5) && (data == 1)) protection_check++;
	jaguargpu_ctrl_w(devtag_get_device(space->machine, "gpu"), offset, data, mem_mask);
}



/*************************************
 *
 *  32-bit access to the DSP
 *
 *************************************/

static READ32_HANDLER( dspctrl_r )
{
	return jaguardsp_ctrl_r(devtag_get_device(space->machine, "audiocpu"), offset);
}


static WRITE32_HANDLER( dspctrl_w )
{
	jaguardsp_ctrl_w(devtag_get_device(space->machine, "audiocpu"), offset, data, mem_mask);
}


/*************************************
 *
 *  Input ports
 *
 *  Information from "The Jaguar Underground Documentation"
 *  by Klaus and Nat!
 *
 *************************************/

static READ32_HANDLER( joystick_r )
{
	UINT16 joystick_result = 0xfffe;
	UINT16 joybuts_result = 0xffef;
	int i;
	static const char *const keynames[2][8] =
	{
		{ "JOY0", "JOY1", "JOY2", "JOY3", "JOY4", "JOY5", "JOY6", "JOY7" },
		{ "BUTTONS0", "BUTTONS1", "BUTTONS2", "BUTTONS3", "BUTTONS4", "BUTTONS5", "BUTTONS6", "BUTTONS7" }
	};

	/*
     *   16        12        8         4         0
     *   +---------+---------+---------^---------+
     *   |  pad 1  |  pad 0  |      unused       |
     *   +---------+---------+-------------------+
     *     15...12   11...8          7...0
     *
     *   Reading this register gives you the output of the selected columns
     *   of the pads.
     *   The buttons pressed will appear as cleared bits.
     *   See the description of the column addressing to map the bits
     *   to the buttons.
     */

	for (i = 0; i < 8; i++)
	{
		if ((joystick_data & (0x10000 << i)) == 0)
		{
			joystick_result &= input_port_read(space->machine, keynames[0][i]);
			joybuts_result &= input_port_read(space->machine, keynames[1][i]);
		}
	}

	joystick_result |= eeprom_read_bit(devtag_get_device(space->machine, "eeprom"));
	joybuts_result |= (input_port_read(space->machine, "CONFIG") & 0x10);

	return (joystick_result << 16) | joybuts_result;
}

static WRITE32_HANDLER( joystick_w )
{
	/*
     *   16        12         8         4         0
     *   +-+-------^------+--+---------+---------+
     *   |r|    unused    |mu|  col 1  |  col 0  |
     *   +-+--------------+--+---------+---------+
     *    15                8   7...4     3...0
     *
     *   col 0:   column control of joypad 0
     *
     *      Here you select which column of the joypad to poll.
     *      The columns are:
     *
     *                Joystick       Joybut
     *      col_bit|11 10  9  8     1    0
     *      -------+--+--+--+--    ---+------
     *         0   | R  L  D  U     A  PAUSE       (RLDU = Joypad directions)
     *         1   | 1  4  7  *     B
     *         2   | 2  5  8  0     C
     *         3   | 3  6  9  #   OPTION
     *
     *      You select a column my clearing the appropriate bit and setting
     *      all the other "column" bits.
     *
     *
     *   col1:    column control of joypad 1
     *
     *      This is pretty much the same as for joypad EXCEPT that the
     *      column addressing is reversed (strange!!)
     *
     *                Joystick      Joybut
     *      col_bit|15 14 13 12     3    2
     *      -------+--+--+--+--    ---+------
     *         4   | 3  6  9  #   OPTION
     *         5   | 2  5  8  0     C
     *         6   | 1  4  7  *     B
     *         7   | R  L  D  U     A  PAUSE     (RLDU = Joypad directions)
     *
     *   mute (mu):   sound control
     *
     *      You can turn off the sound by clearing this bit.
     *
     *   read enable (r):
     *
     *      Set this bit to read from the joysticks, clear it to write
     *      to them.
     */
	COMBINE_DATA(&joystick_data);
}




/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/


static ADDRESS_MAP_START( jaguar_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0xffffff)
	AM_RANGE(0x000000, 0x1fffff) AM_RAM AM_BASE(&jaguar_shared_ram) AM_MIRROR(0x200000) AM_SHARE("share1") AM_REGION("maincpu", 0)
	AM_RANGE(0x800000, 0xdfffff) AM_ROM AM_BASE(&cart_base) AM_SIZE(&cart_size) AM_SHARE("share15") AM_REGION("maincpu", 0x800000)
	AM_RANGE(0xe00000, 0xe1ffff) AM_ROM AM_BASE(&rom_base) AM_SIZE(&rom_size) AM_SHARE("share16") AM_REGION("maincpu", 0xe00000)
	AM_RANGE(0xf00000, 0xf003ff) AM_READWRITE(jaguar_tom_regs32_r, jaguar_tom_regs32_w)
	AM_RANGE(0xf00400, 0xf005ff) AM_MIRROR(0x000200) AM_RAM AM_BASE(&jaguar_gpu_clut) AM_SHARE("share2")
	AM_RANGE(0xf02100, 0xf021ff) AM_MIRROR(0x008000) AM_READWRITE(gpuctrl_r, gpuctrl_w)
	AM_RANGE(0xf02200, 0xf022ff) AM_MIRROR(0x008000) AM_READWRITE(jaguar_blitter_r, jaguar_blitter_w)
	AM_RANGE(0xf03000, 0xf03fff) AM_MIRROR(0x008000) AM_RAM AM_BASE(&jaguar_gpu_ram) AM_SHARE("share3")
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
	AM_RANGE(0xf14000, 0xf14003) AM_READWRITE(joystick_r, joystick_w)
	AM_RANGE(0xf14800, 0xf14803) AM_READWRITE(jaguar_eeprom_clk,jaguar_eeprom_w)	// GPI00
	AM_RANGE(0xf15000, 0xf15003) AM_READ(jaguar_eeprom_cs)				// GPI01
	AM_RANGE(0xf1a100, 0xf1a13f) AM_READWRITE(dspctrl_r, dspctrl_w)
	AM_RANGE(0xf1a140, 0xf1a17f) AM_READWRITE(jaguar_serial_r, jaguar_serial_w)
	AM_RANGE(0xf1b000, 0xf1cfff) AM_RAM AM_BASE(&jaguar_dsp_ram) AM_SHARE("share4")
	AM_RANGE(0xf1d000, 0xf1dfff) AM_ROM AM_REGION("maincpu", 0xf1d000)
ADDRESS_MAP_END

/*************************************
 *
 *  GPU/DSP memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( gpu_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0xffffff)
	AM_RANGE(0x000000, 0x1fffff) AM_MIRROR(0x200000) AM_RAM AM_SHARE("share1") AM_REGION("maincpu", 0)
	AM_RANGE(0x800000, 0xdfffff) AM_ROM AM_SHARE("share15") AM_REGION("maincpu", 0x800000)
	AM_RANGE(0xe00000, 0xe1ffff) AM_ROM AM_SHARE("share16") AM_REGION("maincpu", 0xe00000)
	AM_RANGE(0xf00000, 0xf003ff) AM_READWRITE(jaguar_tom_regs32_r, jaguar_tom_regs32_w)
	AM_RANGE(0xf00400, 0xf005ff) AM_MIRROR(0x000200) AM_RAM AM_SHARE("share2")
	AM_RANGE(0xf02100, 0xf021ff) AM_MIRROR(0x008000) AM_READWRITE(gpuctrl_r, gpuctrl_w)
	AM_RANGE(0xf02200, 0xf022ff) AM_MIRROR(0x008000) AM_READWRITE(jaguar_blitter_r, jaguar_blitter_w)
	AM_RANGE(0xf03000, 0xf03fff) AM_MIRROR(0x008000) AM_RAM AM_SHARE("share3")
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
	AM_RANGE(0xf14000, 0xf14003) AM_READWRITE(joystick_r, joystick_w)
	AM_RANGE(0xf1a100, 0xf1a13f) AM_READWRITE(dspctrl_r, dspctrl_w)
	AM_RANGE(0xf1a140, 0xf1a17f) AM_READWRITE(jaguar_serial_r, jaguar_serial_w)
	AM_RANGE(0xf1b000, 0xf1cfff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0xf1d000, 0xf1dfff) AM_ROM AM_REGION("maincpu", 0xf1d000)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( jaguar )
	PORT_START("JOY0")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY1")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 1") PORT_CODE(KEYCODE_1) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 4") PORT_CODE(KEYCODE_4) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 7") PORT_CODE(KEYCODE_7) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad *") PORT_CODE(KEYCODE_K) PORT_PLAYER(1)
	PORT_BIT( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY2")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 2") PORT_CODE(KEYCODE_2) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 5") PORT_CODE(KEYCODE_5) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 8") PORT_CODE(KEYCODE_8) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 0") PORT_CODE(KEYCODE_0) PORT_PLAYER(1)
	PORT_BIT( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY3")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 3") PORT_CODE(KEYCODE_3) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 6") PORT_CODE(KEYCODE_6) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad 9") PORT_CODE(KEYCODE_9) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P1 Keypad #") PORT_CODE(KEYCODE_L) PORT_PLAYER(1)
	PORT_BIT( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY4")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 3") PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 6") PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 9") PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad #") PORT_PLAYER(2)
	PORT_BIT( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY5")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 2") PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 5") PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 8") PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 0") PORT_PLAYER(2)
	PORT_BIT( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY6")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 1") PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 4") PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad 7") PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("P2 Keypad *") PORT_PLAYER(2)
	PORT_BIT( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY7")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("P1 Pause") PORT_CODE(KEYCODE_I) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 A") PORT_PLAYER(1)
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS1")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 B") PORT_PLAYER(1)
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS2")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 C") PORT_PLAYER(1)
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS3")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("P1 Option") PORT_CODE(KEYCODE_O) PORT_PLAYER(1)
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS4")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("P2 Option") PORT_PLAYER(2)
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS5")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P2 C") PORT_PLAYER(2)
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS6")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 B") PORT_PLAYER(2)
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS7")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("P2 Pause") PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 A") PORT_PLAYER(2)
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("CONFIG")
//  PORT_CONFNAME( 0x02, 0x02, "Show Logo")
//  PORT_CONFSETTING(    0x00, "Yes")
//  PORT_CONFSETTING(    0x02, "No")
	PORT_CONFNAME( 0x10, 0x10, "TV System")
	PORT_CONFSETTING(    0x00, "PAL")
	PORT_CONFSETTING(    0x10, "NTSC")
INPUT_PORTS_END


/*************************************
 *
 *  Machine driver
 *
 *************************************/


static const jaguar_cpu_config gpu_config =
{
	jaguar_gpu_cpu_int
};


static const jaguar_cpu_config dsp_config =
{
	jaguar_dsp_cpu_int
};

static MACHINE_DRIVER_START( jaguar )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68EC020, JAGUAR_CLOCK/2)
	MDRV_CPU_PROGRAM_MAP(jaguar_map)

	MDRV_CPU_ADD("gpu", JAGUARGPU, JAGUAR_CLOCK)
	MDRV_CPU_CONFIG(gpu_config)
	MDRV_CPU_PROGRAM_MAP(gpu_map)

	MDRV_CPU_ADD("audiocpu", JAGUARDSP, JAGUAR_CLOCK)
	MDRV_CPU_CONFIG(dsp_config)
	MDRV_CPU_PROGRAM_MAP(gpu_map)

	MDRV_MACHINE_RESET(jaguar)
	MDRV_NVRAM_HANDLER(jaguar)

	MDRV_TIMER_ADD("serial_timer", jaguar_serial_callback)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_RAW_PARAMS(COJAG_PIXEL_CLOCK/2, 456, 42, 402, 262, 17, 257)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)

	MDRV_VIDEO_START(cojag)
	MDRV_VIDEO_UPDATE(cojag)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("dac1", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MDRV_SOUND_ADD("dac2", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)

	/* quickload */
	MDRV_QUICKLOAD_ADD("quickload", jaguar, "bin", 0)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("jag,abs,rom,j64,j01")
	MDRV_CARTSLOT_LOAD(jaguar)

	MDRV_EEPROM_93C46_ADD("eeprom")
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( jaguar )
	ROM_REGION( 0x1000000, "maincpu", 0 )  /* 4MB for RAM at 0 */
	ROM_LOAD16_WORD( "jagboot.rom", 0xe00000, 0x020000, CRC(fb731aaa) SHA1(f8991b0c385f4e5002fa2a7e2f5e61e8c5213356) )
	ROM_CART_LOAD("cart", 0x800000, 0x600000, ROM_NOMIRROR)
	ROM_LOAD16_WORD("jagwave.rom", 0xf1d000, 0x1000, CRC(7a25ee5b) SHA1(58117e11fd6478c521fbd3fdbe157f39567552f0) )
ROM_END

ROM_START( jaguarcd )
	ROM_REGION( 0x1000000, "maincpu", 0 )
	ROM_SYSTEM_BIOS( 0, "default", "Jaguar CD" )
	ROMX_LOAD( "jag_cd.bin", 0xe00000, 0x040000, CRC(687068d5) SHA1(73883e7a6e9b132452436f7ab1aeaeb0776428e5), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "dev", "Jaguar Developer CD" )
	ROMX_LOAD( "jagdevcd.bin", 0xe00000, 0x040000, CRC(55a0669c) SHA1(d61b7b5912118f114ef00cf44966a5ef62e455a5), ROM_BIOS(2) )
	ROM_CART_LOAD("cart", 0x800000, 0x600000, ROM_NOMIRROR)
	ROM_LOAD16_WORD("jagwave.rom", 0xf1d000, 0x1000, CRC(7a25ee5b) SHA1(58117e11fd6478c521fbd3fdbe157f39567552f0) )
ROM_END

/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( jaguar )
{
	state_save_register_global(machine, joystick_data);
}

static QUICKLOAD_LOAD( jaguar )
{
	int i,j;
	offs_t quickload_begin = 0x4000;
	memset(jaguar_shared_ram, 0, 0x200000);
	quickload_size = MIN(quickload_size, 0x200000 - quickload_begin);
	image_fread(image, &memory_region(image->machine, "maincpu")[quickload_begin], quickload_size);

	/* Fix endian-ness */
	for (i = quickload_begin / 4; i < quickload_size / 4; i++)
	{
		j = jaguar_shared_ram[i];
		jaguar_shared_ram[i] = ((j & 0xff) << 24) | ((j & 0xff00) << 8) | ((j & 0xff0000) >> 8) | ((j & 0xff000000) >> 24);
	}

	/* Transfer control to image */
	cpu_set_reg(devtag_get_device(image->machine, "maincpu"), REG_GENPC, quickload_begin);
	return INIT_PASS;
}

static DEVICE_IMAGE_LOAD( jaguar )
{
	int i,j;
	image_fread(image, cart_base, image_length(image));
	memset(jaguar_shared_ram, 0, 0x200000);
	memcpy(jaguar_shared_ram, rom_base, 0x10);

	/* Fix endian-ness */
	if (!mame_stricmp(image_filetype(image), "j01"))
	{
		for (i = 0; i < image_length(image) / 4; i++)
		{
			j = cart_base[i];
			cart_base[i] = ((j & 0xff) << 16) | ((j & 0xff00) << 16) | ((j & 0xff0000) >> 16) | ((j & 0xff000000) >> 16);
		}
	}
	else
	for (i = 0; i < image_length(image) / 4; i++)
	{
		j = cart_base[i];
		cart_base[i] = ((j & 0xff) << 24) | ((j & 0xff00) << 8) | ((j & 0xff0000) >> 8) | ((j & 0xff000000) >> 24);
	}

	/* Skip the logo */
	cart_base[0x102] = 1;

	/* Transfer control to the bios */
	cpu_set_reg(devtag_get_device(image->machine, "maincpu"), REG_GENPC, rom_base[1]);
	return INIT_PASS;
}

/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

/*    YEAR   NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      COMPANY    FULLNAME */
CONS( 1993,  jaguar,   0,        0,      jaguar,   jaguar,   jaguar,   "Atari",   "Atari Jaguar", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING)
CONS( 1995,  jaguarcd, jaguar,   0,      jaguar,   jaguar,   jaguar,   "Atari",   "Atari Jaguar CD", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING)